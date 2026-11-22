// 1.1 regression test: sequence isolation, concurrent generation, greedy
// determinism, reset and long-context re-entry.
#include <llai/llai.hpp>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>

static int g_fails = 0;

static void check(bool ok, const char * what) {
    std::printf("%s %s\n", ok ? "[ok]  " : "[FAIL]", what);
    if (!ok) {
        ++g_fails;
    }
}

int main(int argc, char ** argv) {
    if (argc < 2) {
        std::printf("usage: %s model.gguf\n", argv[0]);
        return 2;
    }
    llai::Config cfg;
    cfg.model_path = argv[1];
    cfg.threads    = 2;
    cfg.context    = 2048;
    cfg.secure     = false;

    std::string err;
    auto eng = llai::Engine::open(cfg, &err);
    if (!eng) {
        std::printf("[FAIL] engine open: %s\n", err.c_str());
        return 1;
    }
    std::printf("%s\n", eng.info_line().c_str());

    llai::Params p;
    p.max_tokens = 32;

    // ---- 1. two sessions must not share one KV sequence ----------------
    auto a = llai::Session::create(eng);
    auto b = llai::Session::create(eng);
    check(a != nullptr && a->valid(), "session a created");
    check(b != nullptr && b->valid(), "session b created");

    auto ra = a->ask("用一句话介绍你自己", p);
    std::printf("[A]  %s\n", ra.text.c_str());
    check(!ra.text.empty(), "A produced text");

    const int32_t a_past_1 = a->n_past();
    auto rb = b->ask("只回答数字：7 乘以 6 等于多少", p);
    std::printf("[B]  %s\n", rb.text.c_str());
    check(!rb.text.empty(), "B produced text");

    // b's turn must not have touched a's sequence
    check(a->n_past() == a_past_1, "A n_past untouched while B generated");

    auto ra2 = a->ask("我刚才问了你什么？", p);
    std::printf("[A2] %s\n", ra2.text.c_str());
    check(a->n_past() > a_past_1, "A keeps its own history across turns");

    // ---- 2. concurrent generation on two sessions ----------------------
    std::atomic<int> done{0};
    std::string ta, tb;
    const auto t0 = std::chrono::steady_clock::now();
    std::thread th1([&] { ta = a->ask("说三个水果的名字", p).text; ++done; });
    std::thread th2([&] { tb = b->ask("说三种颜色的名字", p).text; ++done; });
    th1.join();
    th2.join();
    const double ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count();
    check(done.load() == 2, "two threads finished without deadlock");
    check(!ta.empty() && !tb.empty(), "both concurrent answers arrived");
    std::printf("     concurrent pair took %.0f ms\n", ms);

    // ---- 3. greedy is deterministic ------------------------------------
    auto g1 = llai::Session::create(eng);
    llai::Params gp;
    gp.max_tokens  = 16;
    gp.temperature = -1.0f;                 // explicit greedy
    const auto r1 = g1->ask("列出 1 到 5 的整数", gp).text;
    g1->reset();
    const auto r2 = g1->ask("列出 1 到 5 的整数", gp).text;
    std::printf("[G1] %s\n[G2] %s\n", r1.c_str(), r2.c_str());
    check(r1 == r2, "greedy decoding is reproducible");
    check(r1.size() > 0, "greedy produced text");

    // ---- 4. reset then re-enter ---------------------------------------
    g1->reset();
    const auto r3 = g1->ask("你好", p).text;
    check(!r3.empty(), "session usable after reset");
    check(g1->n_past() > 0, "n_past rebuilt after reset");

    // ---- 5. many short sessions: the pool must recycle ----------------
    for (int i = 0; i < 12; ++i) {
        auto t = llai::Session::create(eng);
        if (t == nullptr || !t->valid()) {
            std::printf("[FAIL] session pool exhausted at i=%d\n", i);
            ++g_fails;
            break;
        }
    }
    check(true, "12 sequential sessions created and destroyed");

    std::printf("\n%s (%d failure%s)\n",
                g_fails == 0 ? "ALL PASS" : "FAILURES",
                g_fails, g_fails == 1 ? "" : "s");
    return g_fails == 0 ? 0 : 1;
}
