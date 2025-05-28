#include <cactus_rt/rt.h>

#include <chrono>
#include <thread>

#include <gphoto2cpp/gphoto2cpp.h>
#include <camera_control/Camera.h>
#include <camera_control/CameraControl.h>

namespace pycontrol
{

class Thread : public cactus_rt::CyclicThread
{
public:

    using Config = cactus_rt::CyclicThreadConfig;

    Thread(
        const std::string & name,
        const cactus_rt::CyclicThreadConfig & config)
    :
        CyclicThread(name.c_str(), config)
    {}

    void init(std::shared_ptr<CameraControl> & cc)
    {
        _cam_control = cc;
    }

protected:

    std::shared_ptr<CameraControl> _cam_control {nullptr};

    // Main working loop that's dispatched every config.period_ns.
    LoopControl Loop(int64_t /*now*/) noexcept final
    {
        ABORT_IF_NOT(
            _cam_control,
            "_cam_control is null!",
            LoopControl::Stop
        );
        ABORT_IF(
            _cam_control->dispatch(),
            "CameraControl.dispatch() failed",
            LoopControl::Stop
        );
        return LoopControl::Continue;
    }
};
}



int main(int argc, char ** argv)
{
    // Construct CameraControl.
    auto cc = std::make_shared<pycontrol::CameraControl>();
    ABORT_IF(cc->init("config/camera_control.config"), "CameraControl.init() failed", 1);

    // Construct the Runtime config.
    pycontrol::Thread::Config config;

    config.period_ns = cc->control_period() * 1'000'000;
    config.cpu_affinity = std::vector<std::size_t>{2};
    config.SetFifoScheduler(80);
    config.tracer_config.trace_overrun = false;

    cactus_rt::ThreadTracerConfig tconfig;
    tconfig.trace_overrun = false;

    auto thread = std::make_shared<pycontrol::Thread>("camera_control_bin", config);
    thread->init(cc);

    cactus_rt::App app;
    app.RegisterThread(thread);
    app.Start();

    // This function blocks until SIGINT or SIGTERM are received.
    cactus_rt::WaitForAndHandleTerminationSignal();

    app.RequestStop();
    app.Join();

    return 0;
}
