#include <gphoto2cpp/gphoto2cpp.h>
#include <camera_control/Camera.h>

namespace pycontrol
{

Camera::info_vec
Camera::detect_cameras()
{
    auto detected = gphoto2cpp::auto_detect();

    return {};
}


}
