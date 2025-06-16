#pragma once

//#include <string.h>

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <ctime> // For syncing the camera time to system time.

#include <gphoto2/gphoto2-port-log.h>

namespace GP2 {
#include <gphoto2/gphoto2-abilities-list.h>
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-context.h>
#include <gphoto2/gphoto2-port-info-list.h>
#include <gphoto2/gphoto2-port-result.h>
#include <gphoto2/gphoto2-result.h>
#include <gphoto2/gphoto2-widget.h>

constexpr auto ERROR_UNKNOWN_PORT = GP_ERROR_UNKNOWN_PORT;
constexpr auto OK = GP_OK;
constexpr auto ERROR_BAD_PARAMETERS = GP_ERROR_BAD_PARAMETERS;
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

    bool                     write_property(
                                 camera_ptr & camera,
                                 const std::string & property,
                                 const std::string & value
                             );

    bool                     trigger(camera_ptr & camera);

    enum class LogLevel_t : unsigned int {debug, error};

    bool set_log_level(LogLevel_t lvl);
}


//-----------------------------------------------------------------------------
// Inline implementation.
//-----------------------------------------------------------------------------

#define GPHOTO2CPP_DEBUG_LOG std::cerr << __FILE__ << "(" << __LINE__ << "): DEBUG: "
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
        auto res = (expr); \
        if (res < GP2::OK) \
        { \
            static int error_count_##__LINE__ = 0; \
            if (error_count_##__LINE__ < 5) \
            { \
                GPHOTO2CPP_ERROR_LOG << #expr << ": call failed with " << GP2::gp_result_as_string(res) << ", aborting" << std::endl; \
                ++error_count_##__LINE__; \
            } \
            else if (error_count_##__LINE__ == 5) \
            { \
               GPHOTO2CPP_ERROR_LOG << "Suppressing further error messages for '" << #expr << "' at this location (" << __FILE__ << ":" << __LINE__ << ")." << std::endl; \
               ++error_count_##__LINE__; \
            } \
            return return_code; \
        } \
    } while (0)

namespace gphoto2cpp
{

using widget_map = std::map<std::string, widget_ptr>;
using camera_widget_map = std::map<camera_ptr, widget_map>;

inline
std::shared_ptr<camera_widget_map>
get_widget_cache()
{
    static auto _cam_widget_map = std::make_shared<camera_widget_map>();
    GPHOTO2CPP_CHECK_PTR(_cam_widget_map, "failed", nullptr);
    return _cam_widget_map;
}


template <typename T>
bool
str_as_type(const std::string & value, T &output)
{
    std::stringstream ss(value);
    if (not (ss >> output))
    {
        GPHOTO2CPP_ERROR_LOG << "str type conversion failed on '"
                             << value
                             << "'"
                             << std::endl;
        return false;
    }

    return true;
}

inline
std::string
to_string(const GP2::CameraWidgetType widget_type)
{
    switch(widget_type)
    {
        case GP2::GP_WIDGET_WINDOW: return "GP_WIDGET_WINDOW";
        case GP2::GP_WIDGET_SECTION: return "GP_WIDGET_SECTION";
        case GP2::GP_WIDGET_TEXT: return "GP_WIDGET_TEXT";
        case GP2::GP_WIDGET_RANGE: return "GP_WIDGET_RANGE";
        case GP2::GP_WIDGET_TOGGLE: return "GP_WIDGET_TOGGLE";
        case GP2::GP_WIDGET_RADIO: return "GP_WIDGET_RADIO";
        case GP2::GP_WIDGET_MENU: return "GP_WIDGET_MENU";
        case GP2::GP_WIDGET_BUTTON: return "GP_WIDGET_BUTTON";
        case GP2::GP_WIDGET_DATE: return "GP_WIDGET_DATE";
    }
    return "UNKNOWN_GP2_CAMERA_WIDGET_TYPE";
}


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
            GPHOTO2CPP_CHECK_PTR(
                value,
                "gp_widget_get_value('" << property << "') failed",
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


inline
bool
write_property(camera_ptr & camera, const std::string & property, const std::string & value)
{
    auto cache_ptr = get_widget_cache();
    GPHOTO2CPP_CHECK_PTR(cache_ptr, "failed", false);

    auto cache = *cache_ptr;

    auto widget_map_itor = cache.find(camera);
    if (widget_map_itor == cache.end())
    {
        cache[camera] = widget_map{};

    }

    auto widget = widget_ptr {nullptr};

    auto widget_map = cache[camera];
    auto widget_itor = widget_map.find(property);
    if (widget_itor == widget_map.end())
    {
        GP2::CameraWidget * raw_widget {nullptr};

        GPHOTO2CPP_SAFE_CALL(
            GP2::gp_camera_get_single_config(
                camera.get(),
                property.c_str(),
                &raw_widget,
                get_context().get()
            ),
            false
        );

        // Wrap in std::shared_ptr for auto cleanup.
        widget = make_widget(raw_widget);

        cache[camera][property] = widget;
    }
    else
    {
        widget = widget_itor->second;
    }

    GP2::CameraWidget * child {nullptr};

    GPHOTO2CPP_SAFE_CALL(
        _lookup_widget(
            widget.get(),
            property.c_str(),
            &child
        ),
        false
    );

    GP2::CameraWidgetType widget_type;
    GPHOTO2CPP_SAFE_CALL(
        GP2::gp_widget_get_type(child, &widget_type),
        false
    );

    // Modeled after:
    //     https://github.com/gphoto/gphoto2/blob/dfd3d4328d31a28436746191c3d4fa6b258ab797/gphoto2/actions.c#L1845
    //
    switch (widget_type)
    {
        // String.
        case GP2::GP_WIDGET_TEXT:
        {
            GPHOTO2CPP_SAFE_CALL(
                GP2::gp_widget_set_value(child, value.c_str()),
                false
            );
            return true;
        }

        // Float.
        case GP2::GP_WIDGET_RANGE:
        {
            float min_ {0.0f};
            float max_ {0.0f};
            float step_ {0.0f};

            GPHOTO2CPP_SAFE_CALL(
                GP2::gp_widget_get_range(child, &min_, &max_, &step_),
                false
            );

            float value_f {0.0f};

            if (not str_as_type<float>(value, value_f))
            {
                GPHOTO2CPP_ERROR_LOG << "value not a float: '"
                                     << value
                                     << "'"
                                     << std::endl;
                return false;
            };

            // Range Check
            if(value_f < min_ or value_f > max_)
            {
                GPHOTO2CPP_ERROR_LOG << "value out of bounds: "
                                     << min_ << " < "
                                     << value_f << " < "
                                     << max_
                                     << " is false"
                                     << std::endl;
                return false;
            }

            return true;
        }

        // Integer.
        case GP2::GP_WIDGET_TOGGLE:
        {
            static const std::set<std::string> false_bools = {
                "off", "no", "false", "0",
            };
            static const std::set<std::string> true_bools = {
                "on",  "yes","true", "1",
            };
            int bool_value = 999;

            if (false_bools.contains(value))
            {
                bool_value = 0;
            }
            else if (true_bools.contains(value))
            {
                bool_value = 1;
            }

            // No match.
            if (bool_value == 999)
            {
                GPHOTO2CPP_ERROR_LOG << "value '"
                                     << value
                                     << "' is not a boolean"
                                     << std::endl;
                return false;
            }
            GPHOTO2CPP_SAFE_CALL(
                GP2::gp_widget_set_value(child, &bool_value),
                false
            );

            return true;
        }

        // Date.
        case GP2::GP_WIDGET_DATE:
        {
            if(value != "now")
            {
                GPHOTO2CPP_ERROR_LOG << "value '"
                                     << value
                                     << "' != now, only now is supported."
                                     << std::endl;
               return false;
            }

            auto right_now = std::time(nullptr);

            GPHOTO2CPP_SAFE_CALL(
                GP2::gp_widget_set_value(child, &right_now),
                false
            );

            return true;
        }

        case GP2::GP_WIDGET_MENU: // Fall through.
        case GP2::GP_WIDGET_RADIO:
        {
            const int num_choices = GP2::gp_widget_count_choices(child);
            if (num_choices < GP2::OK)
            {
                GPHOTO2CPP_ERROR_LOG << "gp_widget_count_choices() returned "
                                     << num_choices
                                     << std::endl;
                return false;
            }

//~            GPHOTO2CPP_DEBUG_LOG << property << " has num_choices: " << num_choices << std::endl;

            bool choice_set = false;

            // Try to set the value if it matches a valid choice.
            for (int idx = 0; idx < num_choices; ++idx)
            {
                const char * choice;
                auto ret = GP2::gp_widget_get_choice(child, idx, &choice);
                if (ret < GP2::OK)
                {
                    continue;
                }

//~                GPHOTO2CPP_DEBUG_LOG << "    " << idx << ": " << choice << std::endl;

                if (value == choice)
                {
                    GPHOTO2CPP_SAFE_CALL(
                        GP2::gp_widget_set_value(child, value.c_str()),
                        false
                    );
                    choice_set = true;
                    break; // out of for loop.
                }
            }

            if (choice_set)
            {
//~                GPHOTO2CPP_DEBUG_LOG << "choice_set: " << property << " to " << value << std::endl;
                return true;
            }

            GPHOTO2CPP_DEBUG_LOG << "Trying to set " << property << " to " << value
                << " with gp_widget_set_value()" << std::endl;

            // Try to set the string directly.
            GPHOTO2CPP_SAFE_CALL(
                GP2::gp_widget_set_value(child, value.c_str()),
                false
            );

            return true;
        }

        default:
        {
            GPHOTO2CPP_ERROR_LOG << "Widget Type '" << to_string(widget_type)
                                 << "' does not support writes.";
            return false;
        }
    } // switch

    GPHOTO2CPP_DEBUG_LOG << "'" << property << "' widget type: " << to_string(widget_type) << std::endl;

    if (child == widget.get())
    {
        GPHOTO2CPP_SAFE_CALL(
            GP2::gp_camera_set_single_config(
                camera.get(),
                property.c_str(),
                child,
                get_context().get()
            ),
            false
        );
    }
    else
    {
        GPHOTO2CPP_SAFE_CALL(
            GP2::gp_camera_set_config(
                camera.get(),
                widget.get(),
                get_context().get()
            ),
            false
        );
    }

    return true;
}


inline
bool
trigger(camera_ptr & camera)
{
    GPHOTO2CPP_SAFE_CALL(
        GP2::gp_camera_trigger_capture(
            camera.get(),
            get_context().get()
        ),
        false
    );

    return true;
}


inline
void _debug_print(
    GPLogLevel level,
    const char * domain,
    const char * str,
    void * data)
{
    std::cerr << "GP2 DEBUG: " << str << std::endl;
}

inline
void _error_print(
    GPLogLevel level,
    const char * domain,
    const char * str,
    void * data)
{
    std::cerr << "GP2 ERROR: " << str << std::endl;
}

inline
bool set_log_level(LogLevel_t lvl)
{
    switch(lvl)
    {
        case LogLevel_t::debug:
        {
            GPHOTO2CPP_SAFE_CALL(
                gp_log_add_func(GP_LOG_DEBUG, _debug_print, NULL),
                false
            );
            break;
        }
        case LogLevel_t::error:
        {
            GPHOTO2CPP_SAFE_CALL(
                gp_log_add_func(GP_LOG_ERROR, _error_print, NULL),
                false
            );
            break;
        }
    }

    return true;
}


}
