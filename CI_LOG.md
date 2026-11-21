# build failure

```
[  9%] Generate embedded Metal library for wkv
[ 10%] Generate embedded Metal library for argsort
[ 10%] Building CXX object runtime/ggml/src/ggml-blas/CMakeFiles/ggml-blas.dir/ggml-blas.cpp.o
[ 10%] Building C object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/ggml-cpu.c.o
[ 10%] Generate embedded Metal library for binbcast
[ 11%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/ggml-cpu.cpp.o
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
/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/third_party/runtime/ggml/src/ggml-cpu/ggml-cpu.c:3258:17: error: use of undeclared identifier 'nullptr'
 3258 |     return v == nullptr || v[0] == '0';
      |                 ^
1 error generated.
make[2]: *** [runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/ggml-cpu.c.o] Error 1
make[2]: *** Waiting for unfinished jobs....
[ 18%] Generate embedded Metal library for fa_vec_f32
[ 19%] Generate embedded Metal library for fa_vec_q4_0
[ 19%] Generate embedded Metal library for fa_vec_q4_1
[ 20%] Generate embedded Metal library for fa_vec_q5_0
[ 20%] Generate embedded Metal library for fa_vec_q5_1
[ 21%] Generate embedded Metal library for fa_vec_q8_0
[ 22%] Generate embedded Metal library for gated_delta_net
[ 23%] Generate embedded Metal library for misc
[ 24%] Generate embedded Metal library for mul_mm
make[1]: *** [runtime/ggml/src/CMakeFiles/ggml-cpu.dir/all] Error 2
make[1]: *** Waiting for unfinished jobs....
[ 25%] Generate embedded Metal library for mul_mv
[ 25%] Generate embedded Metal library for mul_mv_mma
[ 25%] Generate embedded Metal library for norm
[ 25%] Generate embedded Metal library for pool
[ 25%] Generate embedded Metal library for quantize
[ 26%] Generate embedded Metal library for reduce
[ 26%] Generate embedded Metal library for rope
[ 27%] Generate embedded Metal library for softmax
[ 28%] Generate embedded Metal library for solve_tri
[ 29%] Generate embedded Metal library for ssm
[ 29%] Generate embedded Metal library for tri
[ 30%] Generate embedded Metal library for unary
[ 30%] Generate embedded Metal library for upscale
[ 30%] Building CXX object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal.cpp.o
[ 31%] Building C object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-device.m.o
[ 31%] Building CXX object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-device.cpp.o
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
2 warnings generated.
[ 32%] Linking CXX static library libggml-blas.a
[ 32%] Built target ggml-blas
[ 33%] Building CXX object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-common.cpp.o
[ 34%] Building C object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-context.m.o
[ 34%] Building CXX object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-fusion.cpp.o
[ 35%] Building CXX object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-ops.cpp.o
[ 35%] Building CXX object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-tuning.cpp.o
[ 36%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_aux.s.o
[ 36%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_f16.s.o
[ 37%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_f32.s.o
[ 37%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_q4_0.s.o
[ 38%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_q4_1.s.o
[ 38%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_q5_0.s.o
[ 39%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_q5_1.s.o
[ 39%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_q8_0.s.o
[ 40%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_tensor.s.o
[ 41%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_f16.s.o
[ 41%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_f32.s.o
[ 42%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_q4_0.s.o
[ 42%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_q4_1.s.o
[ 43%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_q5_0.s.o
[ 43%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_q5_1.s.o
[ 44%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_q8_0.s.o
[ 44%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-mul_mv.s.o
[ 45%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-mul_mv_mma.s.o
[ 45%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-mul_mm.s.o
[ 46%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-quantize.s.o
[ 46%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-softmax.s.o
[ 47%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-norm.s.o
[ 48%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-unary.s.o
[ 48%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-binbcast.s.o
[ 49%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-reduce.s.o
[ 49%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-tri.s.o
[ 50%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-ssm.s.o
[ 50%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-wkv.s.o
[ 51%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-gated_delta_net.s.o
[ 51%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-solve_tri.s.o
[ 52%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-rope.s.o
[ 52%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-conv.s.o
[ 53%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-upscale.s.o
[ 54%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-argsort.s.o
[ 54%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-pool.s.o
[ 55%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-misc.s.o
[ 55%] Linking CXX static library libggml-metal.a
[ 55%] Built target ggml-metal
make: *** [all] Error 2
```
