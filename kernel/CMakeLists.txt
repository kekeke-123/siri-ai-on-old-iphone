cmake_minimum_required(VERSION 3.20)
project(siriplus-kernel LANGUAGES C CXX)

# The upstream runtime is fetched at build time (scripts/fetch-runtime.sh) and
# patched in place. It is not vendored, so this tree stays small and the
# attribution lives in third_party/NOTICE.md.
set(RUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/runtime" CACHE PATH "runtime source tree")

if(NOT EXISTS "${RUNTIME_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "runtime tree missing at ${RUNTIME_DIR}; run scripts/fetch-runtime.sh first")
endif()

# Build only what the kernel needs. No server, no tools, no tests.
set(LLAMA_BUILD_COMMON     OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES   OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TOOLS      OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TESTS      OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_SERVER     OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_APP        OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_MTMD       ON  CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS      OFF CACHE BOOL "" FORCE)
set(GGML_OPENMP            OFF CACHE BOOL "" FORCE)
set(GGML_RPC               OFF CACHE BOOL "" FORCE)

add_subdirectory("${RUNTIME_DIR}" "${CMAKE_BINARY_DIR}/runtime" EXCLUDE_FROM_ALL)

add_library(spk STATIC
    src/spk_engine.cpp
    src/spk_session.cpp
    src/spk_vision.cpp
    src/spk_translate.cpp
    src/spk_asr.cpp
    src/spk_guard.cpp
    src/spk_sampler.cpp
)

target_include_directories(spk
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/spk"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${RUNTIME_DIR}/include"
        "${RUNTIME_DIR}/ggml/include"
        "${RUNTIME_DIR}/tools/mtmd"
)

target_compile_features(spk PRIVATE cxx_std_17)
target_compile_options(spk PRIVATE -fvisibility=hidden -fno-exceptions)

target_link_libraries(spk PUBLIC llama mtmd)

# The public ABI is C, so the static library carries no C++ symbols.
set_target_properties(spk PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

# Headless smoke test, built only on request so the iOS archive stays clean.
option(SPK_BUILD_SELFTEST "Build the spk_selftest CLI" OFF)
if(SPK_BUILD_SELFTEST)
    add_executable(spk_selftest tools/spk_selftest.cpp)
    target_compile_features(spk_selftest PRIVATE cxx_std_17)
    target_link_libraries(spk_selftest PRIVATE spk)
endif()
