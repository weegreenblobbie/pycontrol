#pragma once

#include <interface/gphoto2cpp.h>

namespace pycontrol
{


class GPhoto2Cpp : public interface::GPhoto2Cpp
{
public:
    std::vector<std::string>
    auto_detect() override;

    gphoto2cpp::camera_ptr
    open_camera(const std::string & port) override;

    std::set<std::string>
    read_choices(
        const gphoto2cpp::camera_ptr & camera,
        const std::string & property) override;

    bool
    read_config(const gphoto2cpp::camera_ptr & camera) override;

    bool
    read_property(
        const gphoto2cpp::camera_ptr & camera,
        const std::string & property,
        std::string & output) override;

    void
    reset_cache(const gphoto2cpp::camera_ptr & camera) override;

    bool
    trigger(const gphoto2cpp::camera_ptr & camera) override;

    bool
    write_config(gphoto2cpp::camera_ptr & camera) override;

    bool
    write_property(
            gphoto2cpp::camera_ptr & camera,
            const std::string & property,
            const std::string & value) override;
};


} /*  namespace pycontrol */
