#include <common/io.h>
#include <common/types.h>
#include <camera_control/Camera.h>
#include <camera_control/GPhoto2Cpp.h>

using namespace pycontrol;

int main(int argc, char ** argv)
{
    GPhoto2Cpp gp2cpp;

    const auto & all_detected_ports = gp2cpp.auto_detect();

    if (all_detected_ports.empty())
    {
        INFO_LOG << "No cameras detected" << std::endl;
        return 0;
    }

    for (const auto & port : all_detected_ports)
    {
        INFO_LOG << "Opening camera on port: " << port << std::endl;

        auto gp2_camera = gp2cpp.open_camera(port);
        if (not gp2_camera)
        {
            ERROR_LOG << "gphoto2cpp::open_camera() failed, ignoring" << std::endl;
            continue;
        }

        if (not gp2cpp.read_config(gp2_camera))
        {
            ERROR_LOG
                << "gphoto2cpp::read_config(camera) failed"
                << std::endl;
            continue;
        }

        std::string serial;
        if (not gp2cpp.read_property(gp2_camera, "serialnumber", serial))
        {
            ERROR_LOG
                << "gphoto2cpp::read_property(\"serialnumber\") failed"
                << std::endl;
            continue;
        }

        auto cam = Camera(gp2cpp, gp2_camera, port, serial, "camera.config");

        cam.read_config();

        // Increase loop to 2 for debugging issues.
        for (int i = 0; i < 1; ++i)
        {

            INFO_LOG
                << "Camera\n"
                << "    Serial:        " << cam.info().serial << "\n"
                << "    Description:   " << cam.info().desc << "\n"
                << "    imagequality:  " << cam.info().quality << "\n"
                << "    shutterspeed:  " << cam.info().shutter << "\n"
                << "    capturetarget: " << cam.info().capture_target << "\n\n";

            INFO_LOG << "Shutter speeds:\n";

            for (const auto & shutterspeed : cam.read_choices("shutterspeed"))
            {
                INFO_LOG << "    " << shutterspeed << std::endl;
            }

            INFO_LOG << "Capture targets:\n";

            for (const auto & ct : cam.read_choices("capturetarget"))
            {
                INFO_LOG << "    " << ct << std::endl;
            }

            INFO_LOG << "Image quality:\n";

            for (const auto & iq : cam.read_choices("imagequality"))
            {
                INFO_LOG << "    " << iq << std::endl;
            }

            std::cout << std::endl;

            // Trigger the camera.

            INFO_LOG << "Trying 'Memory Card'\n";
            cam.set_capture_target("Memory Card");
            cam.write_config();

            for (int i = 0; i < 10; ++i)
            {

                if (result::success == cam.trigger())
                {
                    INFO_LOG << "trigger success!" << std::endl;
                }
                else
                {
                    ERROR_LOG << "trigger failed!" << std::endl;
                }

            }


        }
    }

    return 0;
}
