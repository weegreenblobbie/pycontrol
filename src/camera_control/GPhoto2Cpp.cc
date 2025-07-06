#include <gphoto2cpp/gphoto2cpp.h>

#include <camera_control/GPhoto2Cpp.h>

namespace pycontrol
{

std::vector<std::string>
GPhoto2Cpp::auto_detect()
{
    return gphoto2cpp::auto_detect();
}

bool
GPhoto2Cpp::list_files(
    const gphoto2cpp::camera_ptr & camera,
    std::vector<std::string> & out)
{
    return gphoto2cpp::list_files(camera, out);
}

gphoto2cpp::camera_ptr
GPhoto2Cpp::open_camera(const std::string & port)
{
    return gphoto2cpp::open_camera(port);
}

std::vector<std::string>
GPhoto2Cpp::read_choices(
    const gphoto2cpp::camera_ptr & camera,
    const std::string & property)
{
    return gphoto2cpp::read_choices(camera, property);
}

bool
GPhoto2Cpp::read_config(const gphoto2cpp::camera_ptr & camera)
{
    return gphoto2cpp::read_config(camera);
}

bool
GPhoto2Cpp::read_property(
    const gphoto2cpp::camera_ptr & camera,
    const std::string & property,
    std::string & output)
{
    return gphoto2cpp::read_property(camera, property, output);
}

void
GPhoto2Cpp::reset_cache(const gphoto2cpp::camera_ptr & camera)
{
    return gphoto2cpp::reset_cache(camera);
}

bool
GPhoto2Cpp::trigger(const gphoto2cpp::camera_ptr & camera)
{
    return gphoto2cpp::trigger(camera);
}

bool
GPhoto2Cpp::write_config(gphoto2cpp::camera_ptr & camera)
{
    return gphoto2cpp::write_config(camera);
}

bool
GPhoto2Cpp::write_property(
    gphoto2cpp::camera_ptr & camera,
    const std::string & property,
    const std::string & value)
{
    return gphoto2cpp::write_property(camera, property, value);
}

bool
GPhoto2Cpp::wait_for_event(
    const gphoto2cpp::camera_ptr & camera,
    const int timeout_ms,
    gphoto2cpp::Event & out)
{
    return gphoto2cpp::wait_for_event(camera, timeout_ms, out);
}


} /*  namespace pycontrol */
