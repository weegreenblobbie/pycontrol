#include <chrono>

#include <gphoto2cpp/gphoto2cpp.h>
#include <camera_control/Camera.h>
#include <camera_control/CameraControl.h>


struct Timer
{
    // #include <chrono>
    Timer(const std::string & message):
        _msg(message),
        _start(std::chrono::system_clock::now())
    {}

    ~Timer()
    {
        if (_dump_on_destruct)
        {
            _dump();
        }
    }

    void _dump() const
    {
        auto duration = std::chrono::duration<double>(std::chrono::system_clock::now() - _start);
        std::cout << _msg << " took " << duration.count() << " seconds\n";
    }

    operator double() const
    {
        _dump_on_destruct = false;
        auto duration = std::chrono::duration<double>(std::chrono::system_clock::now() - _start);
        return duration.count();
    }

private:

    std::string _msg;
    std::chrono::time_point<std::chrono::system_clock> _start;
    mutable bool _dump_on_destruct{true};
};


int main(int argc, char ** argv)
{
//~    auto cc = pycontrol::CameraControl();
//~    ABORT_IF(cc.init("config_camera_control"), "init() failed", 1);

//~    for (int i = 0; i < 10; ++i)
//~    {
//~        ABORT_IF(cc.dispatch(), "failed", 1);
//~    }

    auto timer = Timer("detecting cameras");

    for (int i = 0; i < 30; ++i)
    {
        auto detected = gphoto2cpp::auto_detect();
        for (const auto & port : detected)
        {
            auto camera = gphoto2cpp::open_camera(port);
            if (camera)
            {
                std::cout << "detected camera with serial number: "
                          << gphoto2cpp::read_property(camera, "serialnumber")
                          << "\n";
            }
        }
    }

    double total_seconds = timer;

    std::cout << "detecting cameras took " << total_seconds/ 30.0 << " per call\n";

    return 0;
}
