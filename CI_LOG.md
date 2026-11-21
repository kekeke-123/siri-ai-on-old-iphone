# build failure
run: 44897628414 @ 88f33286e4675ebe88da7966c957272d9693511a

```
[  1%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-base.dir/ggml.cpp.o
[  1%] Building CXX object runtime/vendor/hash/CMakeFiles/vendor-hash.dir/hash.cpp.o
[  2%] Building C object runtime/vendor/hash/CMakeFiles/vendor-hash.dir/xxhash/xxhash.c.o
[  2%] Building CXX object runtime/vendor/hash/CMakeFiles/vendor-hash.dir/sha1/sha1.c.o
[  3%] Building C object runtime/vendor/hash/CMakeFiles/vendor-hash.dir/sha256/sha256.c.o
[  4%] Building C object runtime/ggml/src/CMakeFiles/ggml-base.dir/ggml-alloc.c.o
[  5%] Linking CXX static library libvendor-hash.a
[  5%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-base.dir/ggml-backend.cpp.o
[  6%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-base.dir/ggml-backend-meta.cpp.o
[  6%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-base.dir/ggml-opt.cpp.o
[  6%] Built target vendor-hash
[  7%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-base.dir/ggml-threading.cpp.o
[  7%] Building C object runtime/ggml/src/CMakeFiles/ggml-base.dir/ggml-quants.c.o
[  8%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-base.dir/gguf.cpp.o
[  9%] Linking CXX static library libggml-base.a
[  9%] Built target ggml-base
[  9%] Generate embedded Metal library for wkv
[ 10%] Generate embedded Metal library for argsort
[ 10%] Building C object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/ggml-cpu.c.o
[ 10%] Building CXX object runtime/ggml/src/ggml-blas/CMakeFiles/ggml-blas.dir/ggml-blas.cpp.o
[ 11%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/ggml-cpu.cpp.o
[ 11%] Generate embedded Metal library for binbcast
[ 12%] Generate embedded Metal library for conv
[ 13%] Generate embedded Metal library for fa_aux
[ 13%] Generate embedded Metal library for fa_f16
[ 14%] Generate embedded Metal library for fa_f32
[ 15%] Generate embedded Metal library for fa_q4_0
[ 15%] Generate embedded Metal library for fa_q4_1
[ 16%] Generate embedded Metal library for fa_q5_0
[ 16%] Generate embedded Metal library for fa_q5_1
[ 17%] Generate embedded Metal library for fa_q8_0
[ 17%] Generate embedded Metal library for fa_tensor
[ 18%] Generate embedded Metal library for fa_vec_f16
[ 18%] Generate embedded Metal library for fa_vec_f32
[ 19%] Generate embedded Metal library for fa_vec_q4_0
[ 19%] Generate embedded Metal library for fa_vec_q4_1
[ 19%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/repack.cpp.o
[ 20%] Generate embedded Metal library for fa_vec_q5_0
[ 20%] Generate embedded Metal library for fa_vec_q5_1
[ 21%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/hbm.cpp.o
[ 22%] Generate embedded Metal library for fa_vec_q8_0
[ 22%] Building C object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/quants.c.o
[ 23%] Generate embedded Metal library for gated_delta_net
[ 24%] Generate embedded Metal library for misc
[ 25%] Generate embedded Metal library for mul_mm
[ 26%] Generate embedded Metal library for mul_mv
/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/third_party/runtime/ggml/src/ggml-blas/ggml-blas.cpp:142:13: warning: 'cblas_sgemm' is only available on iOS 16.4 or newer [-Wunguarded-availability-new]
  142 |             cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans,
      |             ^~~~~~~~~~~
/Applications/Xcode_16.4.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS18.5.sdk/System/Library/Frameworks/Accelerate.framework/Frameworks/vecLib.framework/Headers/cblas_new.h:891:6: note: 'cblas_sgemm' has been marked as being introduced in iOS 16.4 here, but the deployment target is iOS 15.0.0
  891 | void cblas_sgemm(const enum CBLAS_ORDER ORDER,
      |      ^
/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/third_party/runtime/ggml/src/ggml-blas/ggml-blas.cpp:142:13: note: enclose 'cblas_sgemm' in a __builtin_available check to silence this warning
  142 |             cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans,
      |             ^~~~~~~~~~~
  143 |                         ne1, ne01, ne10,
  144 |                         1.0f,   y, ne10,
  145 |                                 x, ne00,
  146 |                         0.0f,   d, ne01);
      |                                          
/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/third_party/runtime/ggml/src/ggml-blas/ggml-blas.cpp:207:5: warning: 'cblas_sgemm' is only available on iOS 16.4 or newer [-Wunguarded-availability-new]
  207 |     cblas_sgemm(CblasRowMajor, transposeA, CblasNoTrans, m, n, k, 1.0, a, lda, b, n, 0.0, c, n);
      |     ^~~~~~~~~~~
/Applications/Xcode_16.4.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS18.5.sdk/System/Library/Frameworks/Accelerate.framework/Frameworks/vecLib.framework/Headers/cblas_new.h:891:6: note: 'cblas_sgemm' has been marked as being introduced in iOS 16.4 here, but the deployment target is iOS 15.0.0
  891 | void cblas_sgemm(const enum CBLAS_ORDER ORDER,
      |      ^
/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/third_party/runtime/ggml/src/ggml-blas/ggml-blas.cpp:207:5: note: enclose 'cblas_sgemm' in a __builtin_available check to silence this warning
  207 |     cblas_sgemm(CblasRowMajor, transposeA, CblasNoTrans, m, n, k, 1.0, a, lda, b, n, 0.0, c, n);
      |     ^~~~~~~~~~~                                                                                 
[ 26%] Generate embedded Metal library for mul_mv_mma
[ 26%] Generate embedded Metal library for norm
[ 26%] Generate embedded Metal library for pool
2 warnings generated.
[ 27%] Linking CXX static library libggml-blas.a
[ 27%] Generate embedded Metal library for quantize
[ 27%] Built target ggml-blas
[ 28%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/traits.cpp.o
[ 29%] Generate embedded Metal library for reduce
[ 29%] Generate embedded Metal library for rope
[ 30%] Generate embedded Metal library for softmax
[ 31%] Generate embedded Metal library for solve_tri
[ 32%] Generate embedded Metal library for ssm
[ 32%] Generate embedded Metal library for tri
[ 33%] Generate embedded Metal library for unary
[ 33%] Generate embedded Metal library for upscale
[ 33%] Building CXX object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal.cpp.o
[ 33%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/amx/amx.cpp.o
[ 34%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/amx/mmq.cpp.o
[ 34%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/tiled/tiled.cpp.o
[ 35%] Building C object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-device.m.o
[ 36%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/tiled/tiled-kernel.cpp.o
[ 37%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/binary-ops.cpp.o
[ 37%] Building CXX object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-device.cpp.o
[ 37%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/unary-ops.cpp.o
[ 38%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/vec.cpp.o
[ 39%] Building CXX object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-common.cpp.o
[ 40%] Building C object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-context.m.o
[ 40%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/ops.cpp.o
[ 41%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/llamafile/sgemm.cpp.o
[ 41%] Building C object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/arch/arm/quants.c.o
[ 41%] Building CXX object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-fusion.cpp.o
[ 42%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/arch/arm/repack.cpp.o
[ 43%] Building CXX object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-ops.cpp.o
[ 43%] Building CXX object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-tuning.cpp.o
[ 44%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_aux.s.o
[ 44%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_f16.s.o
[ 45%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_f32.s.o
[ 45%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_q4_0.s.o
[ 46%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_q4_1.s.o
[ 46%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_q5_0.s.o
[ 47%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_q5_1.s.o
[ 47%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_q8_0.s.o
[ 48%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_tensor.s.o
[ 49%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_f16.s.o
[ 49%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_f32.s.o
[ 50%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_q4_0.s.o
[ 50%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_q4_1.s.o
[ 51%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_q5_0.s.o
[ 51%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_q5_1.s.o
[ 52%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_q8_0.s.o
[ 52%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-mul_mv.s.o
[ 53%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-mul_mv_mma.s.o
[ 53%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-mul_mm.s.o
[ 54%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-quantize.s.o
[ 54%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-softmax.s.o
[ 55%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-norm.s.o
[ 56%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-unary.s.o
[ 56%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-binbcast.s.o
[ 57%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-reduce.s.o
[ 57%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-tri.s.o
[ 58%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-ssm.s.o
[ 58%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-wkv.s.o
[ 59%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-gated_delta_net.s.o
[ 59%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-solve_tri.s.o
[ 60%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-rope.s.o
[ 60%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-conv.s.o
[ 61%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-upscale.s.o
[ 62%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-argsort.s.o
[ 62%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-pool.s.o
[ 63%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-misc.s.o
[ 63%] Linking CXX static library libggml-metal.a
[ 63%] Built target ggml-metal
[ 63%] Linking CXX static library libggml-cpu.a
[ 63%] Built target ggml-cpu
[ 63%] Building CXX object runtime/ggml/src/CMakeFiles/ggml.dir/ggml-backend-dl.cpp.o
[ 64%] Building CXX object runtime/ggml/src/CMakeFiles/ggml.dir/ggml-backend-reg.cpp.o
[ 64%] Linking CXX static library libggml.a
[ 64%] Built target ggml
[ 65%] Building CXX object runtime/src/CMakeFiles/llama.dir/Unity/unity_9_cxx.cxx.o
[ 65%] Building CXX object runtime/src/CMakeFiles/llama.dir/Unity/unity_10_cxx.cxx.o
[ 66%] Building CXX object runtime/src/CMakeFiles/llama.dir/Unity/unity_8_cxx.cxx.o
[ 66%] Building CXX object runtime/src/CMakeFiles/llama.dir/Unity/unity_7_cxx.cxx.o
[ 67%] Building CXX object runtime/src/CMakeFiles/llama.dir/Unity/unity_6_cxx.cxx.o
[ 67%] Building CXX object runtime/src/CMakeFiles/llama.dir/Unity/unity_5_cxx.cxx.o
[ 68%] Building CXX object runtime/src/CMakeFiles/llama.dir/Unity/unity_4_cxx.cxx.o
[ 68%] Building CXX object runtime/src/CMakeFiles/llama.dir/Unity/unity_3_cxx.cxx.o
[ 69%] Building CXX object runtime/src/CMakeFiles/llama.dir/Unity/unity_2_cxx.cxx.o
[ 70%] Building CXX object runtime/src/CMakeFiles/llama.dir/Unity/unity_1_cxx.cxx.o
[ 70%] Building CXX object runtime/src/CMakeFiles/llama.dir/Unity/unity_0_cxx.cxx.o
[ 71%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama.cpp.o
[ 71%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-adapter.cpp.o
[ 72%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-arch.cpp.o
[ 72%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-batch.cpp.o
[ 73%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-chat.cpp.o
[ 73%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-context.cpp.o
[ 74%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-cparams.cpp.o
[ 74%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-grammar.cpp.o
[ 75%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-graph.cpp.o
[ 75%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-hparams.cpp.o
[ 76%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-impl.cpp.o
[ 77%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-io.cpp.o
[ 77%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-kv-cache.cpp.o
[ 78%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-kv-cache-iswa.cpp.o
[ 78%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-kv-cache-dsa.cpp.o
[ 79%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-kv-cache-dsa-iswa.cpp.o
[ 79%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-kv-cache-msa.cpp.o
[ 80%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-kv-cache-dsv4.cpp.o
[ 80%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-memory.cpp.o
[ 81%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-moe-cache.cpp.o
[ 81%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-memory-hybrid.cpp.o
[ 82%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-memory-hybrid-iswa.cpp.o
[ 83%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-memory-hybrid-idx.cpp.o
[ 83%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-memory-recurrent.cpp.o
[ 84%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-mmap.cpp.o
[ 84%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-model-loader.cpp.o
[ 85%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-model-saver.cpp.o
[ 85%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-model.cpp.o
[ 86%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-quant.cpp.o
[ 86%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-sampler.cpp.o
[ 87%] Building CXX object runtime/src/CMakeFiles/llama.dir/llama-vocab.cpp.o
[ 87%] Building CXX object runtime/src/CMakeFiles/llama.dir/unicode-data.cpp.o
[ 88%] Building CXX object runtime/src/CMakeFiles/llama.dir/unicode.cpp.o
[ 88%] Linking CXX static library libllama.a
[ 88%] Built target llama
[ 88%] Building CXX object runtime/tools/mtmd/CMakeFiles/mtmd.dir/Unity/unity_1_cxx.cxx.o
[ 89%] Building CXX object runtime/tools/mtmd/CMakeFiles/mtmd.dir/Unity/unity_2_cxx.cxx.o
[ 90%] Building CXX object runtime/tools/mtmd/CMakeFiles/mtmd.dir/Unity/unity_3_cxx.cxx.o
[ 91%] Building CXX object runtime/tools/mtmd/CMakeFiles/mtmd.dir/Unity/unity_0_cxx.cxx.o
[ 91%] Building CXX object runtime/tools/mtmd/CMakeFiles/mtmd.dir/mtmd.cpp.o
[ 92%] Building CXX object runtime/tools/mtmd/CMakeFiles/mtmd.dir/mtmd-audio.cpp.o
[ 92%] Building CXX object runtime/tools/mtmd/CMakeFiles/mtmd.dir/mtmd-image.cpp.o
[ 93%] Building CXX object runtime/tools/mtmd/CMakeFiles/mtmd.dir/mtmd-helper.cpp.o
[ 93%] Building CXX object runtime/tools/mtmd/CMakeFiles/mtmd.dir/mtmd-helper-gen.cpp.o
[ 94%] Building CXX object runtime/tools/mtmd/CMakeFiles/mtmd.dir/mtmd-model.cpp.o
[ 94%] Building CXX object runtime/tools/mtmd/CMakeFiles/mtmd.dir/mtmd-subcontext.cpp.o
[ 95%] Building CXX object runtime/tools/mtmd/CMakeFiles/mtmd.dir/mtmd-graph.cpp.o
[ 95%] Linking CXX static library libmtmd.a
[ 95%] Built target mtmd
[ 98%] Building CXX object CMakeFiles/spk.dir/src/spk_vision.cpp.o
[ 98%] Building CXX object CMakeFiles/spk.dir/src/spk_session.cpp.o
[ 98%] Building CXX object CMakeFiles/spk.dir/src/spk_translate.cpp.o
[ 98%] Building CXX object CMakeFiles/spk.dir/src/spk_engine.cpp.o
[ 98%] Building CXX object CMakeFiles/spk.dir/src/spk_asr.cpp.o
[ 99%] Building CXX object CMakeFiles/spk.dir/src/spk_guard.cpp.o
[ 99%] Building CXX object CMakeFiles/spk.dir/src/spk_sampler.cpp.o
[100%] Linking CXX static library libspk.a
[100%] Built target spk
== static libraries ==
/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel/libspk.a
/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel/runtime/ggml/src/ggml-blas/libggml-blas.a
/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel/runtime/ggml/src/ggml-metal/libggml-metal.a
/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel/runtime/ggml/src/libggml-base.a
/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel/runtime/ggml/src/libggml-cpu.a
/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel/runtime/ggml/src/libggml.a
/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel/runtime/src/libllama.a
/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel/runtime/tools/mtmd/libmtmd.a
/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel/runtime/vendor/hash/libvendor-hash.a
== wrote /Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/app/SPKVendor.xcconfig ==
// generated by scripts/gen-xcconfig.sh - do not edit by hand
HEADER_SEARCH_PATHS = $(inherited) \
    "/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/kernel/include" \
    "/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/kernel/include/spk" \
    "/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/kernel/ios" \
    "/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/third_party/runtime/include" \
    "/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/third_party/runtime/ggml/include" \
    "/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/third_party/runtime/tools/mtmd"

OTHER_LDFLAGS = $(inherited) -lc++ -lz -framework Metal -framework MetalKit \
    -framework Accelerate -framework Foundation -framework UIKit \
    -framework AVFoundation -framework Speech \
    -framework ActivityKit -framework WidgetKit -framework AppIntents
OTHER_LDFLAGS = $(inherited) "/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel/libspk.a"
OTHER_LDFLAGS = $(inherited) "/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel/runtime/ggml/src/ggml-blas/libggml-blas.a"
OTHER_LDFLAGS = $(inherited) "/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel/runtime/ggml/src/ggml-metal/libggml-metal.a"
OTHER_LDFLAGS = $(inherited) "/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel/runtime/ggml/src/libggml-base.a"
OTHER_LDFLAGS = $(inherited) "/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel/runtime/ggml/src/libggml-cpu.a"
OTHER_LDFLAGS = $(inherited) "/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel/runtime/ggml/src/libggml.a"
OTHER_LDFLAGS = $(inherited) "/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel/runtime/src/libllama.a"
OTHER_LDFLAGS = $(inherited) "/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel/runtime/tools/mtmd/libmtmd.a"
OTHER_LDFLAGS = $(inherited) "/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel/runtime/vendor/hash/libvendor-hash.a"
```
