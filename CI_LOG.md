# build failure
run: 44908039288 @ 87d16674884ce68314052757ba2a16b3e78605c0

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
Updating files:  44% (1851/4163)Updating files:  45% (1874/4163)Updating files:  46% (1915/4163)Updating files:  47% (1957/4163)Updating files:  48% (1999/4163)Updating files:  49% (2040/4163)Updating files:  50% (2082/4163)Updating files:  51% (2124/4163)Updating files:  52% (2165/4163)Updating files:  53% (2207/4163)Updating files:  54% (2249/4163)Updating files:  55% (2290/4163)Updating files:  55% (2315/4163)Updating files:  56% (2332/4163)Updating files:  56% (2344/4163)Updating files:  57% (2373/4163)Updating files:  58% (2415/4163)Updating files:  59% (2457/4163)Updating files:  60% (2498/4163)Updating files:  61% (2540/4163)Updating files:  62% (2582/4163)Updating files:  63% (2623/4163)Updating files:  64% (2665/4163)Updating files:  65% (2706/4163)Updating files:  66% (2748/4163)Updating files:  67% (2790/4163)Updating files:  68% (2831/4163)Updating files:  69% (2873/4163)Updating files:  70% (2915/4163)Updating files:  71% (2956/4163)Updating files:  72% (2998/4163)Updating files:  73% (3039/4163)Updating files:  74% (3081/4163)Updating files:  75% (3123/4163)Updating files:  76% (3164/4163)Updating files:  77% (3206/4163)Updating files:  78% (3248/4163)Updating files:  79% (3289/4163)Updating files:  80% (3331/4163)Updating files:  81% (3373/4163)Updating files:  82% (3414/4163)Updating files:  83% (3456/4163)Updating files:  84% (3497/4163)Updating files:  85% (3539/4163)Updating files:  86% (3581/4163)Updating files:  87% (3622/4163)Updating files:  88% (3664/4163)Updating files:  88% (3697/4163)Updating files:  89% (3706/4163)Updating files:  90% (3747/4163)Updating files:  91% (3789/4163)Updating files:  92% (3830/4163)Updating files:  93% (3872/4163)Updating files:  94% (3914/4163)Updating files:  95% (3955/4163)Updating files:  96% (3997/4163)Updating files:  97% (4039/4163)Updating files:  98% (4080/4163)Updating files:  99% (4122/4163)Updating files: 100% (4163/4163)Updating files: 100% (4163/4163), done.
== applying kernel patches ==
  0001-spk-gguf-integrity.patch
  0002-spk-mmap-guard.patch
  0003-spk-thread-qos.patch
  0004-kernel-qwen35.patch
error: kernel/src/spk_sampler.cpp: No such file or directory
error: kernel/src/spk_internal.h: No such file or directory
error: kernel/src/spk_engine.cpp: No such file or directory
error: kernel/src/spk_session.cpp: No such file or directory
error: kernel/src/spk_translate.cpp: No such file or directory
error: kernel/CMakeLists.txt: No such file or directory
```
