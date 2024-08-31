#include <cactus_rt/rt.h>

#include <chrono>
#include <iostream>

class PyControlThread : public cactus_rt::CyclicThread
{
    std::uint64_t _loop_counter {0};

public:

    using Config = cactus_rt::CyclicThreadConfig;

    PyControlThread(
        const char * name,
        const cactus_rt::CyclicThreadConfig & config)
    :
        CyclicThread(name, config)
    {}

    std::int64_t GetLoopCounter() const
    {
        return _loop_counter;
    }

protected:

    // Main working loop that's dispatched every config.period_ns.
    LoopControl Loop(int64_t /*now*/) noexcept final
    {
        if (_loop_counter % 20 == 0)
        {
            LOG_INFO(Logger(), "Loop {}", _loop_counter);
        }
        ++_loop_counter;
        return LoopControl::Continue;
    }
};


int main()
{
    PyControlThread::Config config;
    // 20 Hz
    config.period_ns = 50'000'000;
    config.cpu_affinity = std::vector<size_t>{2};
    config.SetFifoScheduler(80);

    auto thread = std::make_shared<PyControlThread>("thread-pycontrol", config);

    cactus_rt::App app;
    //app.StartTraceSession("build/data.perfetto");
    app.RegisterThread(thread);

    constexpr unsigned int time = 60;
    std::cout << "Testing RT loop for " << time << " seconds.\n";

    app.Start();
    std::this_thread::sleep_for(std::chrono::seconds(time));
    app.RequestStop();
    app.Join();

    std::cout << "Number of loops executed: " << thread->GetLoopCounter() << "\n";
    return 0;
}
