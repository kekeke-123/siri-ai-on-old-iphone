//
//  SPKBridge.h
//  Siri AI on old iPhone
//
//  The only Objective-C surface the app uses. Swift never talks to the C API
//  directly, which keeps the kernel ABI free to change.
//
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^SPKTokenBlock)(NSString * _Nullable piece, BOOL isFinal);
typedef void (^SPKCompletionBlock)(NSError * _Nullable error);

@interface SPKAssistant : NSObject

@property (class, nonatomic, readonly) SPKAssistant * shared;
+ (instancetype)instance;

@property (nonatomic, readonly) BOOL ready;
@property (nonatomic, readonly) BOOL visionReady;

- (nullable NSString *)loadModelAtPath:(NSString *)path
                            mmprojPath:(nullable NSString *)mmprojPath;

- (nullable NSString *)attachModelAtPath:(NSString *)path
                                     role:(NSInteger)role
                               mmprojPath:(nullable NSString *)mmprojPath;

- (void)ask:(NSString *)prompt
      images:(nullable NSArray<NSString *> *)images
     onToken:(SPKTokenBlock)onToken
  completion:(SPKCompletionBlock)completion;

- (void)translate:(NSString *)text
         fromLang:(nullable NSString *)fromLang
           toLang:(NSString *)toLang
          onToken:(SPKTokenBlock)onToken
       completion:(SPKCompletionBlock)completion;

- (void)describeImageAtPath:(NSString *)path
                    question:(nullable NSString *)question
                     onToken:(SPKTokenBlock)onToken
                  completion:(SPKCompletionBlock)completion;

- (void)resetConversation;
- (void)cancel;
- (void)handleMemoryWarning;
- (void)updateThermalState:(NSInteger)state;

- (NSDictionary<NSString *, id> *)info;
- (NSDictionary<NSString *, id> *)metrics;
- (nullable NSString *)lastError;

@end

NS_ASSUME_NONNULL_END
