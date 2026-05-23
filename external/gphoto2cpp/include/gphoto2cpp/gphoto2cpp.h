#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <cctype>
#include <cmath>
#include <csetjmp>
#include <ctime>

extern "C" {
#include <jpeglib.h>
}

#include <gphoto2/gphoto2-port-log.h>

namespace GP2 {
#include <gphoto2/gphoto2-abilities-list.h>
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-context.h>
#include <gphoto2/gphoto2-port-info-list.h>
#include <gphoto2/gphoto2-port-result.h>
#include <gphoto2/gphoto2-result.h>
#include <gphoto2/gphoto2-widget.h>

constexpr auto OK                        = GP_OK;
constexpr auto ERROR_BAD_PARAMETERS      = GP_ERROR_BAD_PARAMETERS;
constexpr auto ERROR_CAMERA_BUSY         = GP_ERROR_CAMERA_BUSY;
constexpr auto ERROR_CAMERA_ERROR        = GP_ERROR_CAMERA_ERROR;
constexpr auto ERROR_CANCEL              = GP_ERROR_CANCEL;
constexpr auto ERROR_CORRUPTED_DATA      = GP_ERROR_CORRUPTED_DATA;
constexpr auto ERROR_DIRECTORY_EXISTS    = GP_ERROR_DIRECTORY_EXISTS;
constexpr auto ERROR_DIRECTORY_NOT_FOUND = GP_ERROR_DIRECTORY_NOT_FOUND;
constexpr auto ERROR_FILE_EXISTS         = GP_ERROR_FILE_EXISTS;
constexpr auto ERROR_FILE_NOT_FOUND      = GP_ERROR_FILE_NOT_FOUND;
constexpr auto ERROR_MODEL_NOT_FOUND     = GP_ERROR_MODEL_NOT_FOUND;
constexpr auto ERROR_NO_SPACE            = GP_ERROR_NO_SPACE;
constexpr auto ERROR_OS_FAILURE          = GP_ERROR_OS_FAILURE;
constexpr auto ERROR_PATH_NOT_ABSOLUTE   = GP_ERROR_PATH_NOT_ABSOLUTE;
constexpr auto ERROR_UNKNOWN_PORT        = GP_ERROR_UNKNOWN_PORT;
}


namespace gphoto2cpp
{
    using cam_list_ptr = std::shared_ptr<GP2::CameraList>;
    using context_ptr = std::shared_ptr<GP2::GPContext>;
    using camera_ptr = std::shared_ptr<GP2::Camera>;
    using camera_file_ptr = std::shared_ptr<GP2::CameraFile>;
    using port_info_list_ptr = std::shared_ptr<GP2::GPPortInfoList>;
    enum class LogLevel_t : unsigned int {debug, error};

    std::vector<std::string> auto_detect();
    context_ptr              get_context();
    camera_ptr               make_camera();
    camera_file_ptr          make_camera_file();
    cam_list_ptr             make_camera_list();
    port_info_list_ptr       make_port_info_list();
    camera_ptr               open_camera(const std::string & port);

    bool                     list_files(
                                 const camera_ptr & camera,
                                 std::vector<std::string> & out);

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

    struct Event
    {
        GP2::CameraEventType  type;
        std::shared_ptr<void> data;
    };

    bool                     wait_for_event(
                                const camera_ptr & camera,
                                const int timeout_ms,
                                Event & out);

    bool                     set_log_level(LogLevel_t lvl);

    struct FileCapture
    {
    private:
        GP2::CameraFilePath _path         {.name = "unused", .folder = "unused"};
        camera_ptr          _camera       {nullptr};
        camera_file_ptr     _file_ptr     {nullptr};
        const char *        _data         {nullptr};
        unsigned long int   _size         {0};
        unsigned int        _num_channels {0}; /* 1 = monochrome or 3 for RGB */

    public:
        explicit FileCapture(const camera_ptr & ptr);

        bool capture();
        bool decompress_jpeg(
            std::vector<unsigned char> & output,
            unsigned int & num_channels
        );

        bool delete_last_capture();
    };
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
    iterator find(const std::string& key)
    {
        std::string lower = key;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        return _map.find(lower);
    }
    iterator begin() { return _map.begin(); }
    iterator end() { return _map.end(); }

    const_iterator find(const std::string& key) const
    {
        std::string lower = key;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        return _map.find(lower);
    }
    const_iterator begin() const { return _map.begin(); }
    const_iterator end() const { return _map.end(); }

    bool empty() const { return _map.empty(); }

private:
    std::unordered_map<std::string, std::string> _map;
};


// forwards
std::string _normailize_shutterspeed(const std::string &);

// Maps normailzed shutter speeds to raw camera shutter speed strings:
//    normalized   raw
//    "1/1600"  -> "0.002s"
//    "2.5"     -> "25/10"
//
class ShutterSpeedCache
{
private:
    struct shutter_speed_pair_t
    {
        const std::string norm;
        const std::string raw;
    };

    std::vector<shutter_speed_pair_t> _cache;

public:

    bool empty() const { return _cache.empty(); }

    // Appends the raw camera string to the cache, paired with the computed
    // normalized string.
    void
    push_back(const std::string& raw)
    {
        _cache.push_back({_normailize_shutterspeed(raw), raw});
    }

    // Returns a vector of normailzied shutter speeds, in the order they were
    // originally added.
    std::vector<std::string>
    read_choices() const
    {
        std::vector<std::string> choices;
        choices.reserve(_cache.size());
        for (const auto& item : _cache)
        {
            choices.push_back(item.norm);
        }
        return choices;
    }

    // Given a normalized shutter speed, return the original raw string it's
    // paired with.
    std::optional<std::string>
    get_raw(const std::string & norm) const
    {
        for (const auto & item : _cache)
        {
            if (norm == item.norm)
            {
                return item.raw;
            }
        }
        // Not found!
        return std::nullopt;
    }
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
using shutterspeed_map   = std::unordered_map<camera_ptr, ShutterSpeedCache>;


// Maps: camera_ptr -> (Normalized String -> Raw Camera String)
inline
shutterspeed_map &
_get_shutterspeed_reverse_map()
{
    static shutterspeed_map instance;
    return instance;
}

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
bool
_list_all_folders_recursively(
    const camera_ptr & camera,
    const std::string & folder,
    std::vector<std::string> & all_folders)
{
    if (all_folders.empty())
    {
        all_folders.push_back(folder);
    }
    auto folder_list_ptr = make_camera_list();
    GPHOTO2CPP_CHECK_PTR(folder_list_ptr, "make_camera_list() failed", false);

    // List subfolders in the current folder.
    if (GP2::gp_camera_folder_list_folders(
            camera.get(),
            folder.c_str(),
            folder_list_ptr.get(),
            get_context().get()) >= GP2::OK)
    {
        const int folder_count = GP2::gp_list_count(folder_list_ptr.get());

        for (int i = 0; i < folder_count; ++i)
        {
            const char* subfolder_name;
            GPHOTO2CPP_SAFE_CALL(
                GP2::gp_list_get_name(
                    folder_list_ptr.get(),
                    i,
                    &subfolder_name
                ),
                false
            );

            // Construct the full path for the subfolder
            std::string full_path = folder;
            if (full_path != "/")
            {
                full_path += "/";
            }
            full_path += subfolder_name;

            all_folders.push_back(full_path);

            // Recurse into the subfolder
            if (not _list_all_folders_recursively(camera, full_path, all_folders))
            {
                return false;
            }
        }
    }

    return true;
}

inline
bool
list_files(const camera_ptr & camera, std::vector<std::string> & out)
{
    out.clear();

    std::vector<std::string> all_folders;
    if (not _list_all_folders_recursively(camera, "/", all_folders))
    {
        return false;
    }

    for (const auto & folder : all_folders)
    {
        auto list_ptr = make_camera_list();
        GPHOTO2CPP_CHECK_PTR(list_ptr, "make_camera_list() failed", false);

        GPHOTO2CPP_SAFE_CALL(
            GP2::gp_camera_folder_list_files(
                camera.get(),
                folder.c_str(),
                list_ptr.get(),
                get_context().get()
            ),
            false
        );
        const int file_count = GP2::gp_list_count(list_ptr.get());

        if (file_count < GP2::OK)
        {
            GPHOTO2CPP_ERROR_LOG << "gp_list_count() failed" << std::endl;
            return false;
        }

        for (int i = 0; i < file_count; ++i)
        {
            const char* filename;
            GPHOTO2CPP_SAFE_CALL(
                GP2::gp_list_get_name(list_ptr.get(), i, &filename),
                false
            );
            out.push_back(folder + "/" + std::string(filename));
        }
    }

    return true;
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
    return camera_ptr(local, [](GP2::Camera * p){GP2::gp_camera_unref(p);});
}

inline
camera_file_ptr
make_camera_file()
{
    GP2::CameraFile * local {nullptr};
    GPHOTO2CPP_SAFE_CALL(GP2::gp_file_new(&local), nullptr);
    return camera_file_ptr(local, [](GP2::CameraFile * p){GP2::gp_file_unref(p);});
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
    GP2::CameraWidget * widget,
    const char * name,
    GP2::CameraWidget ** result)
{
    // From gphoto2/actions.c(1618) _find_widget_by_name().
    //
    int ret = GP2::gp_widget_get_child_by_name(widget, name, result);
    if (ret == GP2::OK)
    {
        // Found it, result should be the child widget.
        return GP2::OK;
    }

    ret = GP2::gp_widget_get_child_by_label(widget, name, result);
    if (ret == GP2::OK)
    {
        // Found it, result should be the child widget.
        return GP2::OK;
    }

    // Not found, iterate through all children to see if any are containers that
    // we can search recursively.
    int count = GP2::gp_widget_count_children(widget);
    for (int i = 0; i < count; i++)
    {
        GP2::CameraWidget * child {nullptr};
        ret = GP2::gp_widget_get_child(widget, i, &child);
        if (ret < GP2::OK)
        {
            continue;
        }

        // Get the type of the child widget.
        GP2::CameraWidgetType type;

        ret = GP2::gp_widget_get_type(child, &type);
        if (ret < GP2::OK)
        {
            continue;
        }

        ret = _lookup_widget(child, name, result);
        if (ret == GP2::OK)
        {
            // Found it, result should be the child widget.
            return GP2::OK;
        }
    }

    // We've finished searching, give up.
    return GP2::ERROR_BAD_PARAMETERS;
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

        if (GP2::OK != _lookup_widget(root.get(), property.c_str(), &child))
        {
            GPHOTO2CPP_ERROR_LOG << "Failed to look up widget '" << property << "', aborting" << std::endl;
            return false;
        }
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
            if (property == "shutterspeed")
            {
                output = _normailize_shutterspeed(output);
            }
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

//
// @brief Normalizes diverse camera shutter speed strings into a uniform symbolic format.
//
// This function processes raw shutter speed strings returned by libgphoto2 (which
// vary heavily by manufacturer, formatting, and precision) and standardizes them
// into a predictable, mathematically grounded symbolic key (e.g., "1/2000", "30", "bulb").
//
// It seamlessly handles:
// - Trailing units (e.g., "0.4s" or "1\"" -> "1/2.5", "1")
// - Manufacturer decimals (e.g., "0.0005" -> "1/2000")
// - Raw mathematical fractions (e.g., "10/25" -> "1/2.5")
// - Mixed/whole exposures (e.g., "25/10" -> "2.5")
// - Named modes (e.g., "Bulb", "Time")
//
// Unsupported or malformed inputs (such as flash sync speeds like "x 200") are
// safely filtered out by returning an empty string.
//
// @param raw The raw shutter speed string directly from the camera widget.
// @return A standardized symbolic string key used for consistent map lookups,
//         or an empty string if the input should be skipped.
//
inline
std::string
_normailize_shutterspeed(const std::string & raw_)
{
    auto raw = std::string(raw_);

    if (raw.empty()) return raw;

    // 1. Lowercase and trim leading/trailing spaces
    std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);
    raw.erase(0, raw.find_first_not_of(" \t\r\n"));
    raw.erase(raw.find_last_not_of(" \t\r\n") + 1);

    // Skip flash sync speeds or unsupported strings (e.g., "x 200")
    if (raw.find('x') != std::string::npos)
    {
        return "";
    }

    // 2. Handle named speeds
    if (raw.find("bulb") != std::string::npos) return "bulb";
    if (raw.find("time") != std::string::npos) return "time";

    // 3. Strip trailing units often appended by cameras
    if (!raw.empty() && (raw.back() == 's' || raw.back() == '"'))
    {
        raw.pop_back();
    }

    // 4. Parse as numeric (handles both fractions "10/25" and decimals "0.0005")
    try
    {
        double val = 0.0;
        size_t slash_pos = raw.find('/');

        if (slash_pos != std::string::npos)
        {
            // Extract and calculate the fraction
            double num = std::stod(raw.substr(0, slash_pos));
            double den = std::stod(raw.substr(slash_pos + 1));

            if (den == 0.0) return ""; // Prevent division by zero
            val = num / den;
        }
        else
        {
            // Parse standard decimal
            val = std::stod(raw);
        }

        // 5. Rebuild into standard symbolic format
        if (val > 0.0)
        {
            if (val < 1.0)
            {
                // Convert to a normalized 1/X fraction (e.g., 0.4 -> 1/2.5)
                double inverted = 1.0 / val;

                // Snap weird hardware timings (e.g., 1666.67) to standard photographic stops
                static const double standard_denoms[] = {
                    32000, 26000, 20000, 16000, 13000, 10000, 8000, 6400, 5000,
                    4000, 3200, 2500, 2000, 1600, 1250, 1000,
                    800, 600, 500, 400, 320, 250, 200, 160, 125, 100,
                    80, 60, 50, 40, 30, 25, 20, 15, 13, 10,
                    8, 6, 5, 4, 3, 2.5, 2, 1.6, 1.3, 1
                };

                for (double s : standard_denoms)
                {
                    // A 12% tolerance safely captures values like 666.67 -> 600
                    // without accidentally bleeding into adjacent valid stops.
                    if (std::abs(inverted - s) / s < 0.12)
                    {
                        inverted = s;
                        break;
                    }
                }

                std::ostringstream oss;
                oss << inverted;
                return "1/" + oss.str();
            }
            else
            {
                // Format whole or mixed numbers cleanly (e.g., 25/10 -> 2.5)
                val = std::round(val * 1000.0) / 1000.0;
                std::ostringstream oss;
                oss << val;
                return oss.str();
            }
        }
        else
        {
            GPHOTO2CPP_ERROR_LOG
                << "_normailize_shutterspeed(\"" << raw_ << "\") failed"
                << std::endl;

            // Fallback: if parsing fails for any reason, return empty to skip it
            return "";
        }
    }
    catch (...)
    {
        GPHOTO2CPP_ERROR_LOG
            << "_normailize_shutterspeed(\"" << raw_ << "\") failed"
            << std::endl;

        // Fallback: if parsing fails for any reason, return empty to skip it
        return "";
    }

    return raw;
}


inline
void
_read_choices_from_gp2(const camera_ptr & camera, const std::string & property, std::vector<std::string> & out, CaseInsensitiveMap & ch_set)
{
    // Call read_property() so the camera and widget pointers iniatlize the
    // property cache.
    std::string temp;
    if (not read_property(camera, property, temp))
    {
        GPHOTO2CPP_ERROR_LOG
            << "Failed to read_property(\"" << property << "\")"
            << std::endl;
    }

    // Grab the child pointer in order to iterate over choices for the property.

    auto & prop_to_child = _get_camera_to_property()[camera];
    auto & child = prop_to_child[property];

    // Grab the widget type.
    GP2::CameraWidgetType widget_type;
    if (GP2::OK != gp_widget_get_type(child, &widget_type))
    {
        GPHOTO2CPP_ERROR_LOG
            << "gp_widget_get_type(\"" << property << "\") failed"
            << std::endl;
    }

    // Iterate over the choices and insert into the CaseInsensitiveMap and
    // output vector.
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

            // Insert the choices into the cache and output vector.
            for (int idx = 0; idx < num_choices; ++idx)
            {
                const char * choice;
                auto ret = GP2::gp_widget_get_choice(child, idx, &choice);
                if (ret < GP2::OK)
                {
                    continue;
                }
                ch_set.insert(std::string(choice));
                out.emplace_back(choice);
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


inline
const ShutterSpeedCache &
_read_shutterspeed_choices(const camera_ptr & camera, std::vector<std::string> & out)
{
    out.clear();

    auto & speed_cam_to_cache = _get_shutterspeed_reverse_map();
    auto itor = speed_cam_to_cache.find(camera);

    // First time for this camera_ptr.
    if (itor == speed_cam_to_cache.end())
    {
        speed_cam_to_cache[camera] = ShutterSpeedCache {};
        itor = speed_cam_to_cache.find(camera);
    }

    // norm_to_raw_map now is a reference for mapping the normailzied to raw
    // shutterspeed strings.
    auto & shutterspeed_cache = itor->second;

    // Cache hit!
    if (not shutterspeed_cache.empty())
    {
        out = shutterspeed_cache.read_choices();
        return shutterspeed_cache;
    }

    CaseInsensitiveMap ch_set;
    _read_choices_from_gp2(camera, "shutterspeed", out, ch_set);

    // Populate the norm to raw map.
    for (const auto & raw: out)
    {
        shutterspeed_cache.push_back(raw);
    }

    out = shutterspeed_cache.read_choices();

    return shutterspeed_cache;
}


inline
const CaseInsensitiveMap &
_read_choices(const camera_ptr & camera, const std::string & property, std::vector<std::string> & out)
{
    if (property == "shutterspeed")
    {
        static const CaseInsensitiveMap tmp;
        _read_shutterspeed_choices(camera, out);
        return tmp;
    }

    // cam_to_choices    maps    camera_ptr   -> choice_map
    // choice_map        maps    property_str -> choice_set
    // choice_set        maps    lower_case   -> ProperCase string

    auto & cam_to_choices = _get_camera_to_choice();
    auto itor1 = cam_to_choices.find(camera);

    // First time for this camera_ptr.
    if (itor1 == cam_to_choices.end())
    {
        cam_to_choices[camera] = choice_map {};
        itor1 = cam_to_choices.find(camera);
    }

    // itor1->second is now the cohice_map.
    auto & ch_map = itor1->second;
    auto itor2 = ch_map.find(property);

    // First time reading this property of the camera.
    if (itor2 == ch_map.end())
    {
        ch_map[property] = choice_set {};
        itor2 = ch_map.find(property);
    }

    // itor2->second is now the choice_set, i.e. CaseInsensitiveMap.
    auto & ch_set = itor2->second;
    out.clear();

    // Cahce hit!
    if (not ch_set.empty())
    {
        for (const auto & [_, value] : ch_set)
        {
            out.emplace_back(value);
        }

        return ch_set;
    }

    // Cache miss, read the choices from the camera via libgphoto2.
    _read_choices_from_gp2(camera, property, out, ch_set);

    return ch_set;
}


inline
std::vector<std::string>
read_choices(const camera_ptr & camera, const std::string & property)
{
    std::vector<std::string> out;
    _read_choices(camera, property, out);
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

        if (GP2::OK != _lookup_widget(root.get(), property.c_str(), &child))
        {
            GPHOTO2CPP_ERROR_LOG << "failed to look up widget '" << property << "', aborting" << std::endl;
            return false;
        };
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
            std::vector<std::string> choices;

            bool missing = false;
            std::string raw_value;

            if (property == "shutterspeed")
            {
                const auto & shutterspeed_cache = _read_shutterspeed_choices(camera, choices);
                auto raw = shutterspeed_cache.get_raw(value);
                missing = true;
                if (raw.has_value())
                {
                    missing = false;
                    raw_value = raw.value();
                }
            }
            else
            {
                const auto & map = _read_choices(camera, property, choices);
                const auto & itor = map.find(value);
                missing = itor == map.end();
                if (not missing)
                {
                    raw_value = itor->second;
                }
            }

            if (missing)
            {
                GPHOTO2CPP_ERROR_LOG
                    << property << " value '" << value << "' is invalid. "
                    << "Please use one of the followng:\n";

                for (const auto & choice : choices)
                {
                    std::cerr << "    '" << choice << "'\n";
                }
                return false;
            }

            GPHOTO2CPP_SAFE_CALL(
                GP2::gp_widget_set_value(child, raw_value.c_str()),
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
bool
wait_for_event(
    const camera_ptr & camera,
    const int timeout_ms,
    Event & out)
{
    void * raw_data {nullptr};
    int res = GP2::ERROR_CAMERA_BUSY;

    while (res == GP2::ERROR_CAMERA_BUSY)
    {
        res = GP2::gp_camera_wait_for_event(
            camera.get(),
            timeout_ms,
            &out.type,
            &raw_data,
            get_context().get()
        );

        if (res == GP2::OK)
        {
            break;
        }
        else
        {
            GPHOTO2CPP_ERROR_LOG
                << "gp_camera_wait_for_event(): call failed with "
                << GP2::gp_result_as_string(res)
                << ", aborting" << std::endl;
            return false;
        }
    };

    out.data = std::shared_ptr<void>(raw_data, [](void * ptr){ if(ptr){::free(ptr);} });

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

inline
FileCapture::FileCapture(const camera_ptr & ptr):
    _camera(ptr),
    _file_ptr(make_camera_file())
{
}

inline
bool
FileCapture::capture()
{
    // Trigger the capture.
    GPHOTO2CPP_SAFE_CALL(
        gp_camera_capture(
            _camera.get(),
            GP2::GP_CAPTURE_IMAGE,
            &_path,
            get_context().get()
        ),
        false
    );

    // IN WORK
    // Force the filename to .JPG to only pull the basic JPEG
    std::string filename(_path.name);
    std::size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos)
    {
        filename.replace(dot_pos, std::string::npos, ".JPG");
        snprintf(_path.name, sizeof(_path.name), "%s", filename.c_str());
    }
    // IN WORK

    GPHOTO2CPP_DEBUG_LOG << "gp_camera_capture() => " << _path.folder << "/" << _path.name << std::endl;

    // Transfer image to RAM.
    GPHOTO2CPP_SAFE_CALL(
        gp_camera_file_get(
            _camera.get(),
            _path.folder,
            _path.name,
            GP2::GP_FILE_TYPE_NORMAL,
            _file_ptr.get(),
            get_context().get()
        ),
        false
    );

    // Get the data pointer and size.
    GPHOTO2CPP_SAFE_CALL(
        gp_file_get_data_and_size(
            _file_ptr.get(),
            &_data,
            &_size
        ),
        false
    );

    GPHOTO2CPP_DEBUG_LOG << "gp_file_get_data_and_size() => size: " << _size << std::endl;

    // Save the file to temp for use by other apps.
    std::ofstream preview_file("/tmp/latest_preview.tmp", std::ios::binary);
    if (preview_file)
    {
        preview_file.write(reinterpret_cast<const char*>(_data), _size);
        preview_file.close();
        if (std::rename("/tmp/latest_preview.tmp", "/tmp/latest_preview.jpg"))
        {
            GPHOTO2CPP_ERROR_LOG << "Failed to rename /tmp/latest_preview.tmp => latest_preview.jpg" << std::endl;
        }
    }
    else
    {
        GPHOTO2CPP_ERROR_LOG << "Failed to write latest_preview.jpg to disk" << std::endl;
    }

    return true;
}


// Custom error handling structure for libjpeg.
struct handle_error_mgr
{
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

// Custom error handler to override libjpeg's default exit().
inline
void
custom_handle_error(j_common_ptr cinfo)
{
    handle_error_mgr * hem = reinterpret_cast<handle_error_mgr *>(cinfo->err);
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message)(cinfo, buffer);
    // Jump back to the point in our code that set up the jump buffer.
    std::longjmp(hem->setjmp_buffer, 1);
}

inline
bool
FileCapture::decompress_jpeg(
    std::vector<unsigned char> & out_pixels,
    unsigned int & out_channels)
{
    jpeg_decompress_struct jinfo;
    handle_error_mgr hem;
    jinfo.err = jpeg_std_error(&hem.pub);
    hem.pub.error_exit = custom_handle_error;

    if (setjmp(hem.setjmp_buffer))
    {
        jpeg_destroy_decompress(&jinfo);
        return false;
    }

    jpeg_create_decompress(&jinfo);
    jpeg_mem_src(&jinfo, reinterpret_cast<const unsigned char*>(_data), _size);
    jpeg_read_header(&jinfo, TRUE);


    // Always compute histograms in grayscale, since JPEG nativly stores data in
    // a YCbCr colorspace:
    //
    // Y (Luminance): The structural black-and-white brightness of the image.
    // Cb (Chrominance Blue): The blue color difference.
    // Cr (Chrominance Red): The red color difference.
    //

    jinfo.out_color_space = JCS_GRAYSCALE;

    // down sample to 1/4 size.
    jinfo.scale_num = 1;
    jinfo.scale_denom = 4;

    jpeg_start_decompress(&jinfo);

    _num_channels = out_channels = jinfo.output_components;

    const auto width = jinfo.output_width;
    const auto height = jinfo.output_height;

    const std::size_t row_stride = width * jinfo.output_components;
    out_pixels.assign(height * row_stride, 0);
    auto pix_ptr = out_pixels.data();

    bool success = true;

	JSAMPROW row_pointer[1] = { nullptr };

    while (jinfo.output_scanline < jinfo.output_height)
    {
        row_pointer[0] = pix_ptr;

        const auto num_lines_read = jpeg_read_scanlines(&jinfo, row_pointer, 1);
        if (num_lines_read != 1)
        {
            GPHOTO2CPP_ERROR_LOG << "jpeg_read_scanlines() returned " << num_lines_read << std::endl;
            success = false;
            break;
        }
        pix_ptr += row_stride;
    }

    jpeg_finish_decompress(&jinfo);
    jpeg_destroy_decompress(&jinfo);

    return success;
}

inline
bool
FileCapture::delete_last_capture()
{
    GPHOTO2CPP_SAFE_CALL(
        gp_camera_file_delete(
            _camera.get(),
            _path.folder,
            _path.name,
            get_context().get()
        ),
        false
    );

    return true;
}


} // namespace gphoto2cpp