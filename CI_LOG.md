# build failure
run: 44898293842 @ 3cccdd9a87a19c5626859ab1766fd73f6b7fd823

```
Xcode 16.4
Build version 16F6
[32m==>[0m [1mDownloading bottle manifests[0m
✔︎ Bottle Manifest xcodegen (2.46.0)
[34m==>[0m [1mWould install 1 formula:[0m
xcodegen
[32m==>[0m [1mFetching downloads for: [32mxcodegen[39m[0m
✔︎ Bottle xcodegen (2.46.0)
[34m==>[0m [1mPouring xcodegen--2.46.0.sonoma.bottle.1.tar.gz[0m
🍺  /usr/local/Cellar/xcodegen/2.46.0: 38 files, 7.8MB
== fetching runtime v1.0.0 ==
Cloning into '/Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/third_party/runtime'...
warning: refs/tags/v1.0.0 a6d1a74dbebce3a16f4f2ceddbf38c21a08d8674 is not a commit!
Updating files:  33% (1407/4163)Updating files:  34% (1416/4163)Updating files:  35% (1458/4163)Updating files:  36% (1499/4163)Updating files:  37% (1541/4163)Updating files:  38% (1582/4163)Updating files:  39% (1624/4163)Updating files:  40% (1666/4163)Updating files:  41% (1707/4163)Updating files:  42% (1749/4163)Updating files:  43% (1791/4163)Updating files:  44% (1832/4163)Updating files:  45% (1874/4163)Updating files:  46% (1915/4163)Updating files:  47% (1957/4163)Updating files:  48% (1999/4163)Updating files:  49% (2040/4163)Updating files:  50% (2082/4163)Updating files:  51% (2124/4163)Updating files:  52% (2165/4163)Updating files:  53% (2207/4163)Updating files:  54% (2249/4163)Updating files:  55% (2290/4163)Updating files:  55% (2303/4163)Updating files:  56% (2332/4163)Updating files:  57% (2373/4163)Updating files:  58% (2415/4163)Updating files:  59% (2457/4163)Updating files:  60% (2498/4163)Updating files:  61% (2540/4163)Updating files:  62% (2582/4163)Updating files:  63% (2623/4163)Updating files:  64% (2665/4163)Updating files:  65% (2706/4163)Updating files:  65% (2723/4163)Updating files:  66% (2748/4163)Updating files:  67% (2790/4163)Updating files:  68% (2831/4163)Updating files:  69% (2873/4163)Updating files:  70% (2915/4163)Updating files:  71% (2956/4163)Updating files:  72% (2998/4163)Updating files:  73% (3039/4163)Updating files:  74% (3081/4163)Updating files:  75% (3123/4163)Updating files:  76% (3164/4163)Updating files:  77% (3206/4163)Updating files:  78% (3248/4163)Updating files:  79% (3289/4163)Updating files:  80% (3331/4163)Updating files:  81% (3373/4163)Updating files:  82% (3414/4163)Updating files:  83% (3456/4163)Updating files:  84% (3497/4163)Updating files:  85% (3539/4163)Updating files:  86% (3581/4163)Updating files:  87% (3622/4163)Updating files:  88% (3664/4163)Updating files:  89% (3706/4163)Updating files:  90% (3747/4163)Updating files:  91% (3789/4163)Updating files:  92% (3830/4163)Updating files:  93% (3872/4163)Updating files:  94% (3914/4163)Updating files:  95% (3955/4163)Updating files:  96% (3997/4163)Updating files:  97% (4039/4163)Updating files:  98% (4080/4163)Updating files:  99% (4122/4163)Updating files: 100% (4163/4163)Updating files: 100% (4163/4163), done.
== applying kernel patches ==
  0001-spk-gguf-integrity.patch
  0002-spk-mmap-guard.patch
  0003-spk-thread-qos.patch
== runtime ready at /Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/third_party/runtime ==
-- The C compiler identification is AppleClang 17.0.0.17000013
-- The CXX compiler identification is AppleClang 17.0.0.17000013
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- llama.cpp version: 1.0.0-dev
CMAKE_BUILD_TYPE=Release
-- Found Git: /usr/local/bin/git (found version "2.55.0")
-- The ASM compiler identification is AppleClang
-- Found assembler: /usr/bin/cc
-- Setting GGML_NATIVE_DEFAULT to OFF
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD - Success
-- Found Threads: TRUE
-- Warning: ccache not found - consider installing it for faster compilation or disable this warning with GGML_CCACHE=OFF
-- CMAKE_SYSTEM_PROCESSOR: 
-- GGML_SYSTEM_ARCH: ARM
-- Including CPU backend
-- Accelerate framework found
-- ARM detected
-- Performing Test GGML_COMPILER_SUPPORTS_FP16_FORMAT_I3E
-- Performing Test GGML_COMPILER_SUPPORTS_FP16_FORMAT_I3E - Failed
-- Checking for ARM features using flags:
-- Performing Test HAVE_DOTPROD
-- Performing Test HAVE_DOTPROD - Failed
-- Performing Test HAVE_SVE
-- Performing Test HAVE_SVE - Failed
-- Performing Test HAVE_MATMUL_INT8
-- Performing Test HAVE_MATMUL_INT8 - Failed
-- Performing Test HAVE_FMA
-- Performing Test HAVE_FMA - Success
-- Performing Test HAVE_FP16_VECTOR_ARITHMETIC
-- Performing Test HAVE_FP16_VECTOR_ARITHMETIC - Failed
-- Performing Test HAVE_SME
-- Performing Test HAVE_SME - Failed
-- Adding CPU backend variant ggml-cpu:  
-- Looking for dgemm_
-- Looking for dgemm_ - found
-- Found BLAS: /Applications/Xcode_16.4.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS18.5.sdk/System/Library/Frameworks/Accelerate.framework
-- BLAS found, Libraries: /Applications/Xcode_16.4.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS18.5.sdk/System/Library/Frameworks/Accelerate.framework
-- BLAS found, Includes: 
-- Including BLAS backend
-- Metal framework found
-- Including METAL backend
-- ggml version: 1.0.0
-- ggml commit:  b1b770d-dirty
-- Disabling MTMD_VIDEO because LLAMA_SUBPROCESS is OFF
-- Configuring done (7.4s)
-- Generating done (0.2s)
-- Build files have been written to: /Users/runner/work/siri-ai-on-old-iphone/siri-ai-on-old-iphone/build/kernel
[  0%] Building CXX object runtime/vendor/hash/CMakeFiles/vendor-hash.dir/hash.cpp.o
[  2%] Building C object runtime/ggml/src/CMakeFiles/ggml-base.dir/ggml.c.o
[  2%] Building C object runtime/vendor/hash/CMakeFiles/vendor-hash.dir/xxhash/xxhash.c.o
[  2%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-base.dir/ggml.cpp.o
[  2%] Building CXX object runtime/vendor/hash/CMakeFiles/vendor-hash.dir/sha1/sha1.c.o
[  3%] Building C object runtime/vendor/hash/CMakeFiles/vendor-hash.dir/sha256/sha256.c.o
[  4%] Building C object runtime/ggml/src/CMakeFiles/ggml-base.dir/ggml-alloc.c.o
[  4%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-base.dir/ggml-backend.cpp.o
[  5%] Linking CXX static library libvendor-hash.a
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
[ 10%] Building CXX object runtime/ggml/src/ggml-blas/CMakeFiles/ggml-blas.dir/ggml-blas.cpp.o
[ 10%] Building C object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/ggml-cpu.c.o
[ 10%] Generate embedded Metal library for binbcast
[ 11%] Generate embedded Metal library for conv
[ 12%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/ggml-cpu.cpp.o
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
[ 20%] Generate embedded Metal library for fa_vec_q5_0
[ 20%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/repack.cpp.o
[ 20%] Generate embedded Metal library for fa_vec_q5_1
[ 21%] Generate embedded Metal library for fa_vec_q8_0
[ 22%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/hbm.cpp.o
[ 23%] Generate embedded Metal library for gated_delta_net
[ 23%] Building C object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/quants.c.o
[ 24%] Generate embedded Metal library for misc
[ 25%] Generate embedded Metal library for mul_mm
[ 26%] Generate embedded Metal library for mul_mv
[ 26%] Generate embedded Metal library for mul_mv_mma
[ 26%] Generate embedded Metal library for norm
[ 26%] Generate embedded Metal library for pool
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
[ 26%] Generate embedded Metal library for quantize
[ 27%] Generate embedded Metal library for reduce
[ 27%] Generate embedded Metal library for rope
2 warnings generated.
[ 28%] Linking CXX static library libggml-blas.a
[ 29%] Generate embedded Metal library for softmax
[ 29%] Built target ggml-blas
[ 30%] Generate embedded Metal library for solve_tri
[ 31%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/traits.cpp.o
[ 32%] Generate embedded Metal library for ssm
[ 32%] Generate embedded Metal library for tri
[ 33%] Generate embedded Metal library for unary
[ 33%] Generate embedded Metal library for upscale
[ 33%] Building CXX object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal.cpp.o
[ 33%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/amx/amx.cpp.o
[ 34%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/amx/mmq.cpp.o
[ 35%] Building C object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-device.m.o
[ 35%] Building CXX object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-device.cpp.o
[ 35%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/tiled/tiled.cpp.o
[ 36%] Building CXX object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-common.cpp.o
[ 37%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/tiled/tiled-kernel.cpp.o
[ 38%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/binary-ops.cpp.o
[ 39%] Building C object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-context.m.o
[ 39%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/unary-ops.cpp.o
[ 39%] Building CXX object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-fusion.cpp.o
[ 40%] Building CXX object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-ops.cpp.o
[ 41%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/vec.cpp.o
[ 41%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/ops.cpp.o
[ 41%] Building CXX object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/ggml-metal-tuning.cpp.o
[ 42%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/llamafile/sgemm.cpp.o
[ 43%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_aux.s.o
[ 43%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_f16.s.o
[ 44%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_f32.s.o
[ 44%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_q4_0.s.o
[ 45%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_q4_1.s.o
[ 45%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_q5_0.s.o
[ 46%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_q5_1.s.o
[ 46%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_q8_0.s.o
[ 47%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_tensor.s.o
[ 48%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_f16.s.o
[ 48%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_f32.s.o
[ 49%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_q4_0.s.o
[ 49%] Building C object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/arch/arm/quants.c.o
[ 49%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_q4_1.s.o
[ 50%] Building ASM object runtime/ggml/src/ggml-metal/CMakeFiles/ggml-metal.dir/autogenerated/ggml-metal-embed-fa_vec_q5_0.s.o
[ 51%] Building CXX object runtime/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/arch/arm/repack.cpp.o
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
[ 64%] Building CXX object runtime/src/CMakeFiles/llama.dir/Unity/unity_9_cxx.cxx.o
[ 66%] Building CXX object runtime/src/CMakeFiles/llama.dir/Unity/unity_10_cxx.cxx.o
[ 66%] Building CXX object runtime/src/CMakeFiles/llama.dir/Unity/unity_7_cxx.cxx.o
[ 66%] Building CXX object runtime/src/CMakeFiles/llama.dir/Unity/unity_8_cxx.cxx.o
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
[ 89%] Building CXX object runtime/tools/mtmd/CMakeFiles/mtmd.dir/Unity/unity_2_cxx.cxx.o
[ 89%] Building CXX object runtime/tools/mtmd/CMakeFiles/mtmd.dir/Unity/unity_1_cxx.cxx.o
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
[ 97%] Building CXX object CMakeFiles/spk.dir/src/spk_vision.cpp.o
[ 98%] Building CXX object CMakeFiles/spk.dir/src/spk_engine.cpp.o
[ 98%] Building CXX object CMakeFiles/spk.dir/src/spk_translate.cpp.o
[ 98%] Building CXX object CMakeFiles/spk.dir/src/spk_session.cpp.o
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
/Users/runner/work/_temp/8d9ebf3c-f2c9-46a6-b618-4026f97c13a6.sh: line 2: ./scripts/gen-xcconfig.sh: Permission denied
```
