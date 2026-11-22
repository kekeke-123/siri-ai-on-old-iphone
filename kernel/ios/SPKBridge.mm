//
//  SPKBridge.mm
//  Siri AI on old iPhone
//
//  ObjC++ shim between Swift and the LLAI Kernel C API.
//  One engine, one long lived session, one serial inference queue.
//
#import "SPKBridge.h"
#include "spk/spk.h"

#include <dispatch/dispatch.h>
#include <string>

struct SPKSink {
    SPKTokenBlock block;
    std::string * acc;
};

static void spk_sink_cb(const char * piece, bool is_final, void * ud) {
    SPKSink * sink = (SPKSink *) ud;
    if (sink == nullptr) {
        return;
    }
    if (piece != nullptr && sink->acc != nullptr) {
        sink->acc->append(piece);
    }
    if (sink->block == nil) {
        return;
    }
    NSString * str = piece != nullptr ? [NSString stringWithUTF8String:piece] : nil;
    SPKTokenBlock block = sink->block;
    dispatch_async(dispatch_get_main_queue(), ^{
        block(str, is_final ? YES : NO);
    });
}

static NSError * spk_error(spk_status st, NSString * message) {
    return [NSError errorWithDomain:@"ai.siriplus.spk"
                               code:(NSInteger) st
                           userInfo:@{ NSLocalizedDescriptionKey: message ?: @"kernel error" }];
}

@interface SPKAssistant () {
    spk_engine *     _engine;
    spk_session *    _session;
    dispatch_queue_t _queue;
}
- (BOOL)prepareSession:(NSError **)error;
@end

@implementation SPKAssistant

+ (instancetype)instance {
    return [self shared];
}

+ (SPKAssistant *)shared {
    static SPKAssistant * inst;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ inst = [[SPKAssistant alloc] init]; });
    return inst;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _queue = dispatch_queue_create("ai.siriplus.spk.inference", DISPATCH_QUEUE_SERIAL);
    }
    return self;
}

- (void)dealloc {
    if (_session != nullptr) { spk_session_destroy(_session); _session = nullptr; }
    if (_engine  != nullptr) { spk_engine_destroy(_engine);   _engine  = nullptr; }
}

- (BOOL)ready       { return _engine != nullptr; }
- (BOOL)visionReady { return _engine != nullptr && spk_supports_vision(_engine); }

- (NSString *)lastError {
    if (_engine == nullptr) return nil;
    const char * e = spk_engine_last_error(_engine);
    return (e != nullptr && e[0] != '\0') ? [NSString stringWithUTF8String:e] : nil;
}

- (NSString *)loadModelAtPath:(NSString *)path
                   mmprojPath:(NSString *)mmprojPath {
    if (path.length == 0) return @"model path is empty";

    if (_session != nullptr) { spk_session_destroy(_session); _session = nullptr; }
    if (_engine  != nullptr) { spk_engine_destroy(_engine);   _engine  = nullptr; }

    spk_engine_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.model_path   = path.UTF8String;
    cfg.mmproj_path  = mmprojPath.length > 0 ? mmprojPath.UTF8String : nullptr;
    cfg.n_threads    = 0;     // auto: performance cores
    cfg.n_ctx        = 0;     // auto from physical RAM
    cfg.n_batch      = 256;
    cfg.n_ubatch     = 128;
    // Metal stays on by default; the fix lives in the context params (f16 K/V,
    // flash attention off, K/V kept on the CPU). The switch stays so a user can
    // drop to CPU if a future model still trips the GPU path.
    NSUserDefaults * defaults = [NSUserDefaults standardUserDefaults];
    if ([defaults objectForKey:@"spk.metal.enabled"] == nil) {
        cfg.use_metal = true;
    } else {
        cfg.use_metal = [defaults boolForKey:@"spk.metal.enabled"];
    }
    cfg.use_mmap     = true;
    cfg.secure_mode  = true;  // escape markup + wipe KV on destroy
    cfg.max_model_gb = 0.0;   // auto guard: 45% of RAM

    char errbuf[256] = {0};
    spk_engine * engine = nullptr;
    spk_status st = spk_engine_create(&cfg, &engine, errbuf, sizeof(errbuf));
    if (st != SPK_OK) {
        return errbuf[0] != '\0' ? [NSString stringWithUTF8String:errbuf] : @"model load failed";
    }
    _engine = engine;

    if (mmprojPath.length > 0) {
        st = spk_engine_add_model(_engine, SPK_MODEL_VISION, path.UTF8String, mmprojPath.UTF8String);
        if (st != SPK_OK) {
            return [self lastError] ?: @"vision projector rejected";
        }
    }
    return nil;
}

- (NSString *)attachModelAtPath:(NSString *)path
                            role:(NSInteger)role
                      mmprojPath:(NSString *)mmprojPath {
    if (_engine == nullptr) return @"load the primary model first";
    if (path.length == 0)  return @"empty path";
    spk_status st = spk_engine_add_model(_engine, (spk_model_role) role, path.UTF8String,
                                         mmprojPath.length > 0 ? mmprojPath.UTF8String : nullptr);
    if (st != SPK_OK) {
        return [self lastError] ?: @"kernel rejected the model";
    }
    return nil;
}

- (BOOL)prepareSession:(NSError **)error {
    if (_engine == nullptr) {
        if (error) *error = spk_error(SPK_ERR_CONTEXT, @"model not loaded");
        return NO;
    }
    if (_session == nullptr) {
        spk_status st = spk_session_create(_engine, &_session);
        if (st != SPK_OK) {
            if (error) *error = spk_error(st, [self lastError]);
            return NO;
        }
        spk_session_warmup(_session);   // snapshot the system prompt KV once
    }
    return YES;
}

- (void)ask:(NSString *)prompt
      images:(NSArray<NSString *> *)images
     onToken:(SPKTokenBlock)onToken
  completion:(SPKCompletionBlock)completion {
    SPKCompletionBlock done = [completion copy];
    NSString *         text = [prompt copy];
    NSArray *          imgs = [images copy];
    SPKTokenBlock      tcb  = [onToken copy];

    dispatch_async(_queue, ^{
        NSError * err = nil;
        if (![self prepareSession:&err]) {
            dispatch_async(dispatch_get_main_queue(), ^{ done(err); });
            return;
        }

        NSMutableArray<NSString *> * paths = [NSMutableArray array];
        for (NSString * p in imgs) {
            if ([p isKindOfClass:[NSString class]]) [paths addObject:p];
        }

        const char ** cpaths = nullptr;
        if (paths.count > 0) {
            cpaths = (const char **) calloc(paths.count + 1, sizeof(char *));
            for (NSUInteger i = 0; i < paths.count; ++i) cpaths[i] = paths[i].UTF8String;
            cpaths[paths.count] = nullptr;
        }

        spk_status st = spk_session_append(_session, "user", text.UTF8String, cpaths);
        if (cpaths != nullptr) free(cpaths);

        if (st != SPK_OK) {
            NSError * e = spk_error(st, [self lastError]);
            dispatch_async(dispatch_get_main_queue(), ^{ done(e); });
            return;
        }

        std::string answer;
        SPKSink sink = { tcb, &answer };

        spk_gen_params gp;
        memset(&gp, 0, sizeof(gp));
        // Every knob left at 0 means "the kernel's default for this
        // architecture". The host no longer overrides Qwen's tuned profile
        // with generic numbers.
        gp.max_tokens     = 256;

        st = spk_session_generate(_session, &gp, spk_sink_cb, &sink);

        if (!answer.empty()) {
            spk_session_append(_session, "assistant", answer.c_str(), nullptr);
        }

        NSError * final = nil;
        if (st != SPK_OK && st != SPK_ERR_CANCELLED) {
            final = spk_error(st, [self lastError]);
        }
        dispatch_async(dispatch_get_main_queue(), ^{ done(final); });
    });
}

- (void)translate:(NSString *)text
         fromLang:(NSString *)fromLang
           toLang:(NSString *)toLang
          onToken:(SPKTokenBlock)onToken
       completion:(SPKCompletionBlock)completion {
    SPKCompletionBlock done = [completion copy];
    SPKTokenBlock      tcb  = [onToken copy];

    dispatch_async(_queue, ^{
        if (_engine == nullptr) {
            dispatch_async(dispatch_get_main_queue(), ^{
                done(spk_error(SPK_ERR_CONTEXT, @"model not loaded"));
            });
            return;
        }
        std::string answer;
        SPKSink sink = { tcb, &answer };

        spk_gen_params gp;
        memset(&gp, 0, sizeof(gp));
        gp.max_tokens  = 512;
        gp.top_k       = 1;
        gp.temperature = 0.1f;

        spk_status st = spk_translate(_engine, text.UTF8String,
                                      fromLang.length ? fromLang.UTF8String : nullptr,
                                      toLang.UTF8String, &gp, spk_sink_cb, &sink);
        dispatch_async(dispatch_get_main_queue(), ^{
            done(st == SPK_OK ? nil : spk_error(st, [self lastError]));
        });
    });
}

- (void)describeImageAtPath:(NSString *)path
                    question:(NSString *)question
                     onToken:(SPKTokenBlock)onToken
                  completion:(SPKCompletionBlock)completion {
    SPKCompletionBlock done = [completion copy];
    SPKTokenBlock      tcb  = [onToken copy];

    dispatch_async(_queue, ^{
        if (_engine == nullptr) {
            dispatch_async(dispatch_get_main_queue(), ^{
                done(spk_error(SPK_ERR_VISION_UNAVAILABLE, @"vision model not loaded"));
            });
            return;
        }
        std::string answer;
        SPKSink sink = { tcb, &answer };

        spk_gen_params gp;
        memset(&gp, 0, sizeof(gp));
        gp.max_tokens  = 256;
        gp.top_k       = 40;
        gp.top_p       = 0.9f;
        gp.temperature = 0.6f;

        spk_status st = spk_describe_image(_engine, path.UTF8String,
                                           question.length ? question.UTF8String : nullptr,
                                           &gp, spk_sink_cb, &sink);
        dispatch_async(dispatch_get_main_queue(), ^{
            done(st == SPK_OK ? nil : spk_error(st, [self lastError]));
        });
    });
}

- (void)resetConversation {
    dispatch_async(_queue, ^{
        if (_session != nullptr) spk_session_reset(_session);
    });
}

- (void)cancel {
    if (_engine != nullptr) spk_engine_cancel(_engine);
}

- (void)handleMemoryWarning {
    if (_engine != nullptr) spk_engine_handle_memory_warning(_engine);
}

- (void)updateThermalState:(NSInteger)state {
    if (_engine != nullptr) spk_engine_set_thermal_state(_engine, (spk_thermal_state) state);
}

- (NSDictionary *)info {
    if (_engine == nullptr) return @{};
    spk_info inf;
    if (spk_engine_info(_engine, &inf) != SPK_OK) return @{};
    return @{
        @"desc":      [NSString stringWithUTF8String:inf.model_desc],
        @"params":    @(inf.params),
        @"bytes":     @(inf.model_bytes),
        @"nCtx":      @(inf.n_ctx),
        @"nCtxTrain": @(inf.n_ctx_train),
        @"threads":   @(inf.n_threads),
        @"layers":    @(inf.n_layer),
        @"embd":      @(inf.n_embd),
        @"vision":    @(inf.vision),
        @"metal":     @(inf.metal),
    };
}

- (NSDictionary *)metrics {
    if (_engine == nullptr) return @{};
    spk_metrics m;
    if (spk_engine_metrics(_engine, &m) != SPK_OK) return @{};
    return @{
        @"ttft":          @(m.ttft_ms),
        @"prefill":       @(m.prefill_tok_s),
        @"decode":        @(m.decode_tok_s),
        @"prefillTokens": @(m.n_prefill_tokens),
        @"decodeTokens":  @(m.n_decode_tokens),
        @"rss":           @(m.peak_rss_mb),
        @"thermal":       @(m.thermal_state),
    };
}

@end
