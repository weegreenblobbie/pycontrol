#pragma once

#include <string.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace GP2 {
#include <gphoto2/gphoto2-abilities-list.h>
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-context.h>
#include <gphoto2/gphoto2-port-info-list.h>
#include <gphoto2/gphoto2-port-result.h>
#include <gphoto2/gphoto2-widget.h>

constexpr auto ERROR_UNKNOWN_PORT = GP_ERROR_UNKNOWN_PORT;
constexpr auto OK = GP_OK;
}

namespace gphoto2cpp
{
    using cam_list_ptr = std::shared_ptr<GP2::CameraList>;
    using context_ptr = std::shared_ptr<GP2::GPContext>;
    using camera_ptr = std::shared_ptr<GP2::Camera>;
    using widget_ptr = std::shared_ptr<GP2::CameraWidget>;
    using port_info_list_ptr = std::shared_ptr<GP2::GPPortInfoList>;

    std::vector<std::string> auto_detect();
    context_ptr              get_context();
    camera_ptr               make_camera();
    cam_list_ptr             make_camera_list();
    port_info_list_ptr       make_port_info_list();
    widget_ptr               make_widget(GP2::CameraWidget * ptr);
    camera_ptr               open_camera(const std::string & port);
    std::string              read_property(
                                 const camera_ptr & camera,
                                 const std::string & property,
                                 const std::string & default_="__ERROR__"
                             );
}


//-----------------------------------------------------------------------------
// Inline implementation.
//-----------------------------------------------------------------------------

#define GPHOTO2CPP_ERROR_LOG std::cerr << __FILE__ << "(" << __LINE__ << "): ERROR: "
#define GPHOTO2CPP_CHECK_PTR( expr, message, return_code ) \
    do { \
        if (not (expr)) { \
            GPHOTO2CPP_ERROR_LOG << #expr << ": " << message << std::endl; \
            return return_code; \
        } \
    } while (0)
#define GPHOTO2CPP_SAFE_CALL( expr, return_code) \
    do { \
        if ((expr) < GP2::OK) \
        { \
            GPHOTO2CPP_ERROR_LOG << #expr << ": gphoto2 call failed, aborting" << std::endl; \
            return return_code; \
        } \
    } while (0)

namespace gphoto2cpp
{


inline
std::vector<std::string>
auto_detect()
{
    std::vector<std::string> out;
    auto ctx = get_context();
    GPHOTO2CPP_CHECK_PTR(ctx, "get_context() failed", out);
    auto camera_list = make_camera_list();
    GPHOTO2CPP_CHECK_PTR(camera_list, "make_camera_list() failed", out);

    GPHOTO2CPP_SAFE_CALL(GP2::gp_camera_autodetect(camera_list.get(), ctx.get()), out);
    GPHOTO2CPP_SAFE_CALL(GP2::gp_list_count(camera_list.get()), out);

    const auto num_detected = GP2::gp_list_count(camera_list.get());

    for (int idx = 0; idx < num_detected; ++idx)
    {
        const char * port {nullptr};
        GPHOTO2CPP_SAFE_CALL(GP2::gp_list_get_value(camera_list.get(), idx, &port), out);
        out.push_back(port);
    }

    return out;
}


inline
std::shared_ptr<GP2::GPContext>
get_context()
{
    static auto _ctx = std::shared_ptr<GP2::GPContext>(
        GP2::gp_context_new(),
        [](GP2::GPContext * p){gp_context_unref(p);}
    );
    GPHOTO2CPP_CHECK_PTR(_ctx, "failed", nullptr);
    return _ctx;
}

inline
camera_ptr
make_camera()
{
    GP2::Camera * local {nullptr};
    GPHOTO2CPP_SAFE_CALL(gp_camera_new(&local), nullptr);
    return camera_ptr(local, [](GP2::Camera * p){GP2::gp_camera_free(p);});
}


inline
cam_list_ptr
make_camera_list()
{
    GP2::CameraList * local {nullptr};
    GPHOTO2CPP_SAFE_CALL(GP2::gp_list_new(&local), nullptr);
    return cam_list_ptr(local, [](GP2::CameraList * p){GP2::gp_list_free(p);});
}


inline
port_info_list_ptr
make_port_info_list()
{
    GP2::GPPortInfoList * local {nullptr};
    GPHOTO2CPP_SAFE_CALL(gp_port_info_list_new(&local), nullptr);
    return std::shared_ptr<GP2::GPPortInfoList>(
        local,
        [](GP2::GPPortInfoList * p){GP2::gp_port_info_list_free(p);}
    );
}


inline
widget_ptr
make_widget(GP2::CameraWidget * ptr)
{
    return widget_ptr(ptr, [](GP2::CameraWidget * p){GP2::gp_widget_free(p);});
}


inline
camera_ptr
open_camera(const std::string & port)
{
    // Lookup the port info object for the provided port.
    // If found, make a camera object and connect over the given port.
    auto port_info_list_ptr = make_port_info_list();
    GPHOTO2CPP_SAFE_CALL(GP2::gp_port_info_list_load(port_info_list_ptr.get()), nullptr);
    const int port_index = GP2::gp_port_info_list_lookup_path(port_info_list_ptr.get(), port.c_str());
    if (port_index == GP2::ERROR_UNKNOWN_PORT or port_index < GP2::OK)
    {
        GPHOTO2CPP_ERROR_LOG
            << ": failed to lookup port info for port '" << port
            << "', aborting" << std::endl;
        return nullptr;
    }
    GP2::GPPortInfo port_info;
    GPHOTO2CPP_SAFE_CALL(
        GP2::gp_port_info_list_get_info(
            port_info_list_ptr.get(),
            port_index,
            &port_info
        ),
        nullptr
    );

    auto camera = make_camera();

    GPHOTO2CPP_SAFE_CALL(
        GP2::gp_camera_set_port_info(camera.get(), port_info),
        nullptr
    );

    auto result = GP2::gp_camera_init(camera.get(), get_context().get());
    if (result < GP2::OK)
    {
        GPHOTO2CPP_ERROR_LOG << "Failed to connect to port " << port << "\n"
                             << "Try unmounted it.\n";
        return nullptr;
    }

    return camera;
}


inline
int
_lookup_widget(
    GP2::CameraWidget * widget,
    const char * key,
    GP2::CameraWidget ** child)
{
    int ret;
    ret = GP2::gp_widget_get_child_by_name(widget, key, child);
    if (ret < GP2::OK)
    {
        ret = gp_widget_get_child_by_label(widget, key, child);
    }
    return ret;
}


inline
std::string
read_property(const camera_ptr & camera, const std::string & property, const std::string & default_)
{
    GP2::CameraWidget * raw_widget {nullptr};

    GPHOTO2CPP_SAFE_CALL(
        GP2::gp_camera_get_single_config(
            camera.get(),
            property.c_str(),
            &raw_widget,
            get_context().get()
        ),
        default_
    );

    // Wrap in std::shared_ptr.
    auto widget = make_widget(raw_widget);

    GP2::CameraWidget * raw_child {nullptr};

    GPHOTO2CPP_SAFE_CALL(
        _lookup_widget(
            raw_widget,
            property.c_str(),
            &raw_child
        ),
        default_
    );

    GP2::CameraWidgetType widget_type;
    GPHOTO2CPP_SAFE_CALL(
        gp_widget_get_type(raw_child, &widget_type),
        default_
    );

    switch(widget_type)
    {
        // char * types.
        case GP2::GP_WIDGET_MENU:  // fall through
        case GP2::GP_WIDGET_TEXT:  // fall through
        case GP2::GP_WIDGET_RADIO:
        {
            char * value {nullptr};
            GPHOTO2CPP_SAFE_CALL(
                GP2::gp_widget_get_value(raw_child, &value),
                default_
            );
            if (property == "serialnumber")
            {
                // Strip off leading zeros.
                while (*value == '0') ++value;
            }
            return std::string(value);
        }
        // int types
        case GP2::GP_WIDGET_DATE: // fall through
        case GP2::GP_WIDGET_TOGGLE:
        {
            int value {0};
            GPHOTO2CPP_SAFE_CALL(
                GP2::gp_widget_get_value(raw_child, &value),
                default_
            );
            return std::to_string(value);
        }
        // float types
        case GP2::GP_WIDGET_RANGE:
        {
            float value {0};
            GPHOTO2CPP_SAFE_CALL(
                GP2::gp_widget_get_value(raw_child, &value),
                default_
            );

            if (property == "availableshots")
            {
                return std::to_string(static_cast<std::uint32_t>(value));
            }
            return std::to_string(value);
        }

        default:
        {
            GPHOTO2CPP_ERROR_LOG << "widget has bad type " << widget_type << "\n";
            return default_;
        }
    }

    return default_;
}


}
