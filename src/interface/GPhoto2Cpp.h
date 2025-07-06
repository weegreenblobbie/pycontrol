#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>


// Forward types from gphoto2cpp.h
namespace GP2
{
    // libgphoto2's Camera definition.
    typedef struct _Camera Camera;
}

namespace gphoto2cpp
{
    using camera_ptr = std::shared_ptr<GP2::Camera>;
    struct Event;
}


namespace pycontrol
{
namespace interface
{


class GPhoto2Cpp
{
public:
    virtual ~GPhoto2Cpp() = default;

    virtual
    std::vector<std::string>
    auto_detect() = 0;

    virtual
    bool
    list_files(
        const gphoto2cpp::camera_ptr & camera,
        std::vector<std::string> & out) = 0;

    virtual
    gphoto2cpp::camera_ptr
    open_camera(const std::string & port) = 0;

    virtual
    std::vector<std::string>
    read_choices(
        const gphoto2cpp::camera_ptr & camera,
        const std::string & property) = 0;

    virtual
    bool
    read_config(const gphoto2cpp::camera_ptr & camera) = 0;

    virtual
    bool
    read_property(
        const gphoto2cpp::camera_ptr & camera,
        const std::string & property,
        std::string & output) = 0;

    virtual
    void
    reset_cache(const gphoto2cpp::camera_ptr & camera) = 0;

    virtual
    bool
    trigger(const gphoto2cpp::camera_ptr & camera) = 0;

    virtual
    bool
    write_config(gphoto2cpp::camera_ptr & camera) = 0;

    virtual
    bool
    write_property(
            gphoto2cpp::camera_ptr & camera,
            const std::string & property,
            const std::string & value) = 0;

    virtual
    bool
    wait_for_event(
        const gphoto2cpp::camera_ptr & camera,
        const int timeout,
        gphoto2cpp::Event & out) = 0;

};


} /* namespace interface */
} /* namespace pycontrol */
