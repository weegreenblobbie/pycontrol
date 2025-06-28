#include <camera_control/Camera.h>
#include <camera_control/CameraControl.h>
#include <camera_control/GPhoto2Cpp.h>
#include <common/str_utils.h>
#include <common/UdpSocket.h>


#include <cactus_rt/rt.h>


#include <chrono>
#include <thread>


using namespace pycontrol;


class Thread : public cactus_rt::CyclicThread
{
public:

    using Config = cactus_rt::CyclicThreadConfig;

    Thread(
        const std::string & name,
        const cactus_rt::CyclicThreadConfig & config,
        CameraControl & cc)
    :
        CyclicThread(name.c_str(), config),
        _cam_control(cc)
    {}

protected:

    CameraControl & _cam_control;

    // Main working loop that's dispatched every config.period_ns.
    LoopControl Loop(int64_t /*now*/) noexcept final
    {
        ABORT_ON_FAILURE(
            _cam_control.dispatch(),
            "FATAL: CameraControl.dispatch(), aborting thread!",
            LoopControl::Stop
        );
        return LoopControl::Continue;
    }
};


//-----------------------------------------------------------------------------
// Reads the input config files and update settings found theirin.  All settings
// are key value pair strings.  Here's a complete example with default values:
//
//     # comments start with '#' and the rest of the line is ignored.
//     udp_ip            239.192.168.1  # The UDP IP address the telemetry is sent to.
//     command_port      10017          # Reads command messages on this port.
//     telem_port        10018          # Writes telemetry messages on this port.
//     period            50             # 20 Hz or 50 ms dispatch period.
//     camera_aliases    filename       # A file to persistently map camera serial numbers to short names.
//
//-----------------------------------------------------------------------------

struct cc_config_t
{
    std::string   udp_ip;
    std::uint16_t command_port;
    std::uint16_t telem_port;
    milliseconds  control_period;
    kv_pair_vec   camera_to_ids;
};

result
read_camera_control_config(const std::string & filename, cc_config_t & out)
{
    //-------------------------------------------------------------------------
    // Read in the config file.
    //-------------------------------------------------------------------------
    kv_pair_vec config_pairs;
    ABORT_IF(read_config(filename, config_pairs), "failed", result::failure);

    milliseconds period = 0;
    std::string udp_ip = "";
    auto command_port = std::uint16_t {0};
    auto telem_port = std::uint16_t {0};
    kv_pair_vec cam_to_ids;

    for (const auto & pair : config_pairs)
    {
        if (pair.key == "udp_ip")
        {
            udp_ip = pair.value;
        }
        else
        if (pair.key == "command_port")
        {
            ABORT_ON_FAILURE(
                as_type<std::uint16_t>(pair.value, command_port),
                "as_type<std::uint16_t>(" << pair.value <<") failed",
                result::failure
            );
        }
        else
        if (pair.key == "telem_port")
        {
            ABORT_ON_FAILURE(
                as_type<std::uint16_t>(pair.value, telem_port),
                "as_type<std::uint16_t>(" << pair.value <<") failed",
                result::failure
            );
        }
        else
        if (pair.key == "period")
        {
            ABORT_ON_FAILURE(
                as_type<milliseconds>(pair.value, period),
                "as_type<std::uint64_t>(" << pair.value <<") failed",
                result::failure
            );
        }
        else
        if (pair.key == "camera_aliases")
        {
            ABORT_ON_FAILURE(read_config(pair.value, cam_to_ids), "Failed to read camera_aliases", result::failure);
        }
    }

    ABORT_IF_NOT(udp_ip.starts_with("239."), "Please use a local multicast IP address", result::failure);
    ABORT_IF(command_port < 1024, "command_port too low, pick a higher port", result::failure);
    ABORT_IF(telem_port < 1024, "telem_port too low, pick a higher port", result::failure);
    ABORT_IF(period < 10, "100+ Hz is probably too fast", result::failure);

    out = cc_config_t {
        .udp_ip         = udp_ip,
        .command_port   = command_port,
        .telem_port     = telem_port,
        .control_period = period,
        .camera_to_ids  = cam_to_ids
    };

    return result::success;
}


int main(int argc, char ** argv)
{
    // Deugging libgphoto2.
    //gphoto2cpp::set_log_level(gphoto2cpp::LogLevel_t::debug);

    cc_config_t cfg;
    ABORT_ON_FAILURE(
        read_camera_control_config("config/camera_control.config", cfg),
        "failure",
        1
    );

    INFO_LOG << "init():         udp_ip: " << cfg.udp_ip << "\n";
    INFO_LOG << "init():   command_port: " << cfg.command_port << "\n";
    INFO_LOG << "init():     telem_port: " << cfg.telem_port << "\n";
    INFO_LOG << "init(): control_period: " << cfg.control_period << " ms\n";

    UdpSocket command_socket;

    ABORT_ON_FAILURE(command_socket.init(cfg.udp_ip, cfg.command_port), "failure", 1);
    ABORT_ON_FAILURE(command_socket.bind(), "failure", 1);

    UdpSocket telem_socket;
    ABORT_ON_FAILURE(telem_socket.init(cfg.udp_ip, cfg.command_port), "failure", 1);

    GPhoto2Cpp gp2cpp;

    CameraControl cc(command_socket, telem_socket, gp2cpp, cfg.camera_to_ids);

    // Construct the Runtime config.
    Thread::Config config;

    config.period_ns = cfg.control_period * 1'000'000;
    config.cpu_affinity = std::vector<std::size_t>{2};
    config.SetFifoScheduler(80);
    config.tracer_config.trace_overrun = false;

    auto thread = std::make_shared<Thread>(
        "camera_control_bin",
        config,
        cc
    );

    cactus_rt::App app;
    app.RegisterThread(thread);
    app.Start();

    // This function blocks until SIGINT or SIGTERM are received.
    cactus_rt::WaitForAndHandleTerminationSignal();

    app.RequestStop();
    app.Join();

    return 0;
}
