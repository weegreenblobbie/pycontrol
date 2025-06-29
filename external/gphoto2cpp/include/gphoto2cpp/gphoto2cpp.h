#pragma once

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <ctime>
#include <cctype>

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
}


namespace gphoto2cpp
{
    using cam_list_ptr = std::shared_ptr<GP2::CameraList>;
    using context_ptr = std::shared_ptr<GP2::GPContext>;
    using camera_ptr = std::shared_ptr<GP2::Camera>;
    using port_info_list_ptr = std::shared_ptr<GP2::GPPortInfoList>;
    enum class LogLevel_t : unsigned int {debug, error};

    std::vector<std::string> auto_detect();
    context_ptr              get_context();
    camera_ptr               make_camera();
    cam_list_ptr             make_camera_list();
    port_info_list_ptr       make_port_info_list();
    camera_ptr               open_camera(const std::string & port);

    bool                     read_config(const camera_ptr & camera);
    bool                     read_property(
                                 const camera_ptr & camera,
                                 const std::string & property,
                                 std::string & output
                             );

    std::vector<std::string> read_choices(
                                 const camera_ptr & camera,
                                 const std::string & property);

    bool                     write_property(
                                 camera_ptr & camera,
                                 const std::string & property,
                                 const std::string & value
                             );

    bool                     write_config(camera_ptr & camera);

    void                     reset_cache(const camera_ptr & camera);

    bool                     trigger(const camera_ptr & camera);

    bool                     set_log_level(LogLevel_t lvl);
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

/*
 * Case insensitive map.  This is used so users of this library don't have to
 * get a property case string exactly correct.  Instead, use this map to look up
 * the proper case that the camera uses.
 */
class CaseInsensitiveMap
{
public:
    // Type alias for the map's iterator for convenience.
    using iterator = std::unordered_map<std::string, std::string>::iterator;
    using const_iterator = std::unordered_map<std::string, std::string>::const_iterator;
    void insert(const std::string& value)
    {
        std::string key = value;
        std::transform(key.begin(), key.end(), key.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        _map[key] = value;
    }
    iterator find(const std::string& key) { return _map.find(key); }
    iterator begin() { return _map.begin(); }
    iterator end() { return _map.end(); }

    const_iterator find(const std::string& key) const { return _map.find(key); }
    const_iterator begin() const { return _map.begin(); }
    const_iterator end() const { return _map.end(); }

    bool empty() const { return _map.empty(); }

private:
    std::unordered_map<std::string, std::string> _map;
};

/*
 * Camera widget caches
 */
using child_widget_ptr = GP2::CameraWidget *;
using widget_ptr = std::shared_ptr<GP2::CameraWidget>;

using root_widget_ptr = widget_ptr;

root_widget_ptr make_root_widget(GP2::CameraWidget * ptr);

using camera_to_root     = std::map<camera_ptr, root_widget_ptr>;
using property_map       = std::unordered_map<std::string, child_widget_ptr>;
using camera_to_property = std::map<camera_ptr, property_map>;
using choice_set         = CaseInsensitiveMap;
using choice_map         = std::unordered_map<std::string, choice_set>;
using camera_to_choice   = std::map<camera_ptr, choice_map>;

inline
camera_to_root &
_get_camera_to_root()
{
    static auto _camera_to_root = camera_to_root();
    return _camera_to_root;
}

inline
camera_to_property &
_get_camera_to_property()
{
    static auto _camera_to_property = camera_to_property();
    return _camera_to_property;
}

inline
camera_to_choice &
_get_camera_to_choice()
{
    static auto _camera_to_choice = camera_to_choice();
    return _camera_to_choice;
}


inline
void
reset_cache(const camera_ptr & camera)
{
    auto & cam_to_root = _get_camera_to_root();
    const auto itor1 = cam_to_root.find(camera);
    if (itor1 != cam_to_root.end())
    {
        // Cache hit!  Must free the root widgit and clear the property map.
        cam_to_root.erase(itor1);

        // Discard the property cache.
        auto & cam_to_prop = _get_camera_to_property();
        auto itor2 = cam_to_prop.find(camera);
        if (itor2 != cam_to_prop.end())
        {
            cam_to_prop.erase(itor2);
        }

        // Discard the choice cache.
        auto & cam_to_choice = _get_camera_to_choice();
        auto itor3 = cam_to_choice.find(camera);
        if (itor3 != cam_to_choice.end())
        {
            cam_to_choice.erase(itor3);
        }
    }
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
root_widget_ptr
make_root_widget(GP2::CameraWidget * ptr)
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
    const root_widget_ptr & widget,
    const std::string & property,
    child_widget_ptr * child)
{
    int ret;
    ret = GP2::gp_widget_get_child_by_name(widget.get(), property.c_str(), child);
    if (ret < GP2::OK)
    {
        ret = gp_widget_get_child_by_label(widget.get(), property.c_str(), child);
    }
    return ret;
}


inline
bool
read_config(const camera_ptr & camera)
{
    reset_cache(camera);

    // Read the full camera configuration.
    GP2::CameraWidget * raw_root {nullptr};

    GPHOTO2CPP_SAFE_CALL(
        GP2::gp_camera_get_config(
            camera.get(),
            &raw_root,
            get_context().get()
        ),
        false
    );

    // Wrap in std::shared_ptr and store in the cache.
    _get_camera_to_root()[camera] = make_root_widget(raw_root);

    return true;
}


inline
bool
read_property(
    const camera_ptr & camera,
    const std::string & property,
    std::string & output)
{
    auto & cam_to_root = _get_camera_to_root();
    auto itor1 = cam_to_root.find(camera);
    if (itor1 == cam_to_root.end())
    {
        GPHOTO2CPP_ERROR_LOG << "must call read_config() first!" << std::endl;
        return false;
    }
    auto & root = itor1->second;
    auto & cam_to_prop = _get_camera_to_property()[camera];
    auto itor2 = cam_to_prop.find(property);
    if (itor2 == cam_to_prop.end())
    {
        // Cache miss, fetch the child widget from libgphoto2.
        child_widget_ptr child {nullptr};

        GPHOTO2CPP_SAFE_CALL(
            _lookup_widget(
                root,
                property,
                &child
            ),
            false
        );
        cam_to_prop[property] = child;
    }

    auto child = cam_to_prop[property];

    GP2::CameraWidgetType widget_type;
    GPHOTO2CPP_SAFE_CALL(
        gp_widget_get_type(child, &widget_type),
        false
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
                GP2::gp_widget_get_value(child, &value),
                false
            );
            GPHOTO2CPP_CHECK_PTR(
                value,
                "gp_widget_get_value('" << property << "') failed",
                false
            );
            if (property == "serialnumber")
            {
                // Strip off leading zeros.
                while (*value == '0') ++value;
            }
            output = std::string(value);
            return true;
        }
        // int types
        case GP2::GP_WIDGET_DATE: // fall through
        case GP2::GP_WIDGET_TOGGLE:
        {
            int value {0};
            GPHOTO2CPP_SAFE_CALL(
                GP2::gp_widget_get_value(child, &value),
                false
            );
            output = std::to_string(value);
            return true;
        }
        // float types
        case GP2::GP_WIDGET_RANGE:
        {
            float value {0};
            GPHOTO2CPP_SAFE_CALL(
                GP2::gp_widget_get_value(child, &value),
                false
            );

            if (property == "availableshots")
            {
                output = std::to_string(static_cast<std::uint32_t>(value));
                return true;
            }
            output = std::to_string(value);
            return true;
        }

        default:
        {
            GPHOTO2CPP_ERROR_LOG << "widget has bad type " << widget_type << "\n";
            return false;
        }
    }

    return false;
}


inline
const CaseInsensitiveMap &
_read_choices(const camera_ptr & camera, const std::string & property)
{
    auto & cam_to_choices = _get_camera_to_choice();
    auto itor1 = cam_to_choices.find(camera);
    // Cache miss.
    if (itor1 == cam_to_choices.end())
    {
        cam_to_choices[camera] = choice_map();
        itor1 = cam_to_choices.find(camera);
    }

    auto & ch_map = itor1->second;
    auto itor2 = ch_map.find(property);
    // Cache miss.
    if (itor2 == ch_map.end())
    {
        ch_map[property] = choice_set();
        itor2 = ch_map.find(property);
    }

    auto & ch_set = itor2->second;

    // Cache miss.
    if (ch_set.empty())
    {
        // Call read_property() so the child widget pointer is fetched if it was
        // missing in the cache.
        std::string temp;
        if (not read_property(camera, property, temp))
        {
            GPHOTO2CPP_ERROR_LOG
                << "Failed to read_property(\"" << property << "\")"
                << std::endl;
            return ch_set;
        }

        auto & prop_to_child = _get_camera_to_property()[camera];
        auto & child = prop_to_child[property];

        // Grab the widget type and read the choices if it's a Window or Radio
        // widget.
        GP2::CameraWidgetType widget_type;
        if (GP2::OK !=  gp_widget_get_type(child, &widget_type))
        {
            GPHOTO2CPP_ERROR_LOG
                << "gp_widget_get_type(\"" << property << "\") failed"
                << std::endl;
            return ch_set;
        }

        switch(widget_type)
        {
            case GP2::GP_WIDGET_MENU: // Fall through.
            case GP2::GP_WIDGET_RADIO:
            {
                const int num_choices = GP2::gp_widget_count_choices(child);
                if (num_choices < GP2::OK)
                {
                    GPHOTO2CPP_ERROR_LOG << "gp_widget_count_choices() returned "
                                         << num_choices
                                         << std::endl;
                    break;
                }

                for (int idx = 0; idx < num_choices; ++idx)
                {
                    const char * choice;
                    auto ret = GP2::gp_widget_get_choice(child, idx, &choice);
                    if (ret < GP2::OK)
                    {
                        continue;
                    }
                    ch_set.insert(std::string(choice));
                }
                break;
            }
            default:
            {
                GPHOTO2CPP_ERROR_LOG
                    << "Property \"" << property << "\" is not a Menu or Radio "
                    << "widget, no choices to read."
                    << std::endl;
                break;
            }
        }
    }

    return ch_set;
}


inline
std::vector<std::string>
read_choices(const camera_ptr & camera, const std::string & property)
{
    auto & cam_to_choices = _get_camera_to_choice();
    auto itor = cam_to_choices.find(camera);
    if (itor == cam_to_choices.end())
    {
        _read_choices(camera, property);
        itor = cam_to_choices.find(camera);
    }
    const auto & ch_map = itor->second;

    auto out = std::vector<std::string>();
    auto itor2 = ch_map.find(property);
    if (itor2 != ch_map.end())
    {
        for (const auto & choice : itor2->second)
        {
            out.push_back(choice.first);
        }
    }

    return out;
}


inline
bool
write_property(camera_ptr & camera, const std::string & property, const std::string & value)
{
    auto & cam_to_root = _get_camera_to_root();
    auto itor1 = cam_to_root.find(camera);
    if (itor1 == cam_to_root.end())
    {
        GPHOTO2CPP_ERROR_LOG << "must call read_config() first!" << std::endl;
        return false;
    }

    auto & root = itor1->second;
    auto & cam_to_prop = _get_camera_to_property()[camera];
    auto itor2 = cam_to_prop.find(property);
    if (itor2 == cam_to_prop.end())
    {
        // Cache miss, fetch the child widget from libgphoto2.
        child_widget_ptr child {nullptr};

        GPHOTO2CPP_SAFE_CALL(
            _lookup_widget(
                root,
                property,
                &child
            ),
            false
        );
        cam_to_prop[property] = child;
    }

    auto child = cam_to_prop[property];

    GP2::CameraWidgetType widget_type;
    GPHOTO2CPP_SAFE_CALL(
        gp_widget_get_type(child, &widget_type),
        false
    );

    // Before writing a new value, clear the changed flag so we only read a
    // changed status if the value we wrote changes the child.
    GPHOTO2CPP_SAFE_CALL(
        GP2::gp_widget_set_changed(child, 0),
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

            // Propagate changed status to root widget.
            if (GP2::gp_widget_changed(child))
            {
                GPHOTO2CPP_SAFE_CALL(
                    GP2::gp_widget_set_changed(root.get(), 1),
                    false
                );
            }

            return true;
        }

        // Float.
        case GP2::GP_WIDGET_RANGE:
        {
            float value_f {0.0f};

            if (not str_as_type<float>(value, value_f))
            {
                GPHOTO2CPP_ERROR_LOG << "value not a float: '"
                                     << value
                                     << "'"
                                     << std::endl;
                return false;
            };

            auto ret = GP2::gp_widget_set_value(child, &value_f);

            if (ret < GP2::OK)
            {
                float min_ {0.0f};
                float max_ {0.0f};
                float step_ {0.0f};

                GPHOTO2CPP_SAFE_CALL(
                    GP2::gp_widget_get_range(child, &min_, &max_, &step_),
                    false
                );

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

                return false;
            }

            // Propagate changed status to root widget.
            if (GP2::gp_widget_changed(child))
            {
                GPHOTO2CPP_SAFE_CALL(
                    GP2::gp_widget_set_changed(root.get(), 1),
                    false
                );
            }

            return true;
        }

        // Boolean.
        case GP2::GP_WIDGET_TOGGLE:
        {
            static const std::set<std::string> false_bools = {
                "off", "no", "false", "0",
            };
            static const std::set<std::string> true_bools = {
                "on",  "yes", "true", "1",
            };

            constexpr int INVALID_BOOL = 999;

            int bool_value = INVALID_BOOL;

            if (false_bools.contains(value))
            {
                bool_value = 0;
            }
            else if (true_bools.contains(value))
            {
                bool_value = 1;
            }

            // No match.
            if (bool_value == INVALID_BOOL)
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

            // Propagate changed status to root widget.
            if (GP2::gp_widget_changed(child))
            {
                GPHOTO2CPP_SAFE_CALL(
                    GP2::gp_widget_set_changed(root.get(), 1),
                    false
                );
            }

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

            // Propagate changed status to root widget.
            if (GP2::gp_widget_changed(child))
            {
                GPHOTO2CPP_SAFE_CALL(
                    GP2::gp_widget_set_changed(root.get(), 1),
                    false
                );
            }

            return true;
        }

        case GP2::GP_WIDGET_MENU: // Fall through.
        case GP2::GP_WIDGET_RADIO:
        {
            const auto & map = _read_choices(camera, property);
            const auto & itor = map.find(value);
            if (itor == map.end())
            {
                GPHOTO2CPP_ERROR_LOG
                    << property << " value '" << value << "' is invalid. "
                    << "Please use one of the followng:\n";

                for (const auto & choice : read_choices(camera, property))
                {
                    std::cerr << "    " << choice << "\n";
                }
                return false;
            }

            GPHOTO2CPP_SAFE_CALL(
                GP2::gp_widget_set_value(child, itor->second.c_str()),
                false
            );

            // Propagate changed status to root widget.
            if (GP2::gp_widget_changed(child))
            {
                GPHOTO2CPP_SAFE_CALL(
                    GP2::gp_widget_set_changed(root.get(), 1),
                    false
                );
            }

            return true;
        }

        default:
        {
            GPHOTO2CPP_ERROR_LOG
                << property << " has widget type '" << to_string(widget_type)
                << "' does not support updates.";

            return false;
        }
    }

    return false;
}

inline
bool
write_config(camera_ptr & camera)
{
    auto root = _get_camera_to_root()[camera];

    // Quick return if noting to write.
    if (not GP2::gp_widget_changed(root.get()))
    {
        return true;
    }

    GPHOTO2CPP_SAFE_CALL(
        GP2::gp_camera_set_config(
            camera.get(),
            root.get(),
            get_context().get()
        ),
        false
    );

    // Clear the changed status on the root widget.
    GPHOTO2CPP_SAFE_CALL(
        GP2::gp_widget_set_changed(root.get(), false),
        false
    );

    return true;
}


inline
bool
trigger(const camera_ptr & camera)
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
