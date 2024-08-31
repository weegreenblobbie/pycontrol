#include <algorithm>
#include <chrono>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include <gphoto2/gphoto2-abilities-list.h>
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-context.h>
#include <gphoto2/gphoto2-port-info-list.h>
#include <gphoto2/gphoto2-widget.h>

// Emit UDP Packet for health and status.
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
//#include <string.h>


#define DEBUG_LOG std::cout << __FILE__ << "(" << __LINE__ << "): DEBUG: "
#define ERROR_LOG std::cerr << __FILE__ << "(" << __LINE__ << "): ERROR: "

#define SAFE_CALL(expr, return_code) \
    if ((expr) < GP_OK) \
    { \
        ERROR_LOG << "gphoto2 internal error, aborting" << std::endl; \
        return (return_code); \
    }

#define CHECK_PTR(ptr_expr, return_code) \
    if (not (ptr_expr)) \
    { \
        ERROR_LOG << "gphoto2 internal error, aborting" << std::endl; \
        return (return_code); \
    }


std::string to_string(const CameraWidgetType & type)
{
    switch (type)
    {
        case GP_WIDGET_BUTTON: return "button";
        case GP_WIDGET_DATE: return "date";
        case GP_WIDGET_MENU: return "menu";
        case GP_WIDGET_RADIO: return "radio";
        case GP_WIDGET_RANGE: return "range";
        case GP_WIDGET_SECTION: return "section";
        case GP_WIDGET_TEXT: return "text";
        case GP_WIDGET_TOGGLE: return "toggle";
        case GP_WIDGET_WINDOW: return "window";
        default: return "unknown";
    }
    return "unkown";
}

std::string to_string(const CameraEventType & type)
{
    switch (type)
    {
        case GP_EVENT_UNKNOWN: return "unknown";
        case GP_EVENT_TIMEOUT: return "timeout";
        case GP_EVENT_FILE_ADDED: return "file-added";
        case GP_EVENT_FOLDER_ADDED: return "folder-added";
        case GP_EVENT_CAPTURE_COMPLETE: return "capture-complete";
        case GP_EVENT_FILE_CHANGED: return "file-changed";
        default: return "unkown";
    }
    return "unkown";
}

static std::string progress_text;

extern "C"
{
    static float target = 0.0f;

    unsigned int start_func(GPContext * context, float target_, const char * text, void *data)
    {
        target = target_;
        progress_text = std::string(text);
        std::cout << "start: target: " << target << " progress: " << progress_text;
        return 1;
    }

    void update_func(GPContext * context, unsigned int id, float current, void *data)
    {
        std::cout << ".";
    }

    void stop_func(GPContext * context, unsigned int id, void * data)
    {
        std::cout << " DONE\n";
    }

    void idle_func(GPContext * context, void * data)
    {
        std::cout << "idle_func()\n";
    }

    void error_func(GPContext * context, const char * text, void * data)
    {
        std::cerr << "gphoto2 ERROR: " << text << "\n";
    }

    void status_func(GPContext * context, const char * text, void * data)
    {
        std::cerr << "gphoto2 STATUS: " << text << "\n";
    }

    void message_func(GPContext * context, const char * text, void * data)
    {
        std::cerr << "gphoto2 MESSAGE: " << text << "\n";
    }
}

struct Timer
{
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


// Strip off leaning '0' characters from a std::string.
inline void lstrip(std::string &str, char tok)
{
    str.erase(
        str.begin(),
        std::find_if(
            str.begin(),
            str.end(),
            [&](unsigned char ch)
            {
                return ch != tok;
            }
        )
    );
}

inline void rstrip(std::string &str, char tok)
{
    str.erase(
        std::find_if(
            str.rbegin(),
            str.rend(),
            [&](unsigned char ch)
            {
                return ch != tok;
            }
        ).base(),
        str.end()
    );
}

inline void strip(std::string &str, char tok)
{
    lstrip(str, tok);
    rstrip(str, tok);
}

// Sort name, remove some common strings from camera manufacture string.
inline void filter_manufacture(std::string &str)
{
    for (const auto & substr : {"Corporation"})
    {
        const auto pos = str.find(substr);
        if (pos != str.npos)
        {
            str.erase(pos, std::strlen(substr));
        }
    }

    strip(str, ' ');
}

constexpr size_t MAX_LEN = 128;

struct CameraInfo
{
    char serial[MAX_LEN];
    char port[MAX_LEN];
    char desc[MAX_LEN];
};


// Ugly singleton pattern.
std::shared_ptr<GPContext> get_context()
{
    static std::shared_ptr<GPContext> _ctx(gp_context_new(), [](GPContext * p){gp_context_unref(p);});
    return _ctx;
}

std::shared_ptr<CameraList> make_camera_list()
{
    CameraList * local {nullptr};
    auto result = gp_list_new(&local);

    if (result < GP_OK)
    {
        ERROR_LOG << "FAILED: gp_list_new()\n";
        return std::shared_ptr<CameraList>(nullptr);
    }

    return std::shared_ptr<CameraList>(local, [](CameraList * p){gp_list_free(p);});
}


std::shared_ptr<GPPortInfoList> make_port_info_list()
{
    GPPortInfoList * local {nullptr};
    auto result = gp_port_info_list_new(&local);

    if (result < GP_OK)
    {
        ERROR_LOG << "FAILED: gp_port_info_list_new()\n";
        return std::shared_ptr<GPPortInfoList>(nullptr);
    }

    return std::shared_ptr<GPPortInfoList>(local, [](GPPortInfoList * p){gp_port_info_list_free(p);});
}


bool scan_for_cameras(std::vector<CameraInfo> & out)
{
    out.clear();

    auto ctx = get_context();

    //-------------------------------------------------------------------------
    // Autodetect cameras.
    auto camera_list = make_camera_list();
    CHECK_PTR(camera_list, 1);
    // loads backend driver.
    SAFE_CALL(gp_camera_autodetect(camera_list.get(), ctx.get()), 1);
    SAFE_CALL(gp_list_count(camera_list.get()), 1);
    const int num_cameras = gp_list_count(camera_list.get());
    if (num_cameras == 0)
    {
        // Nothing to do!
        return 0;
    }

    //-------------------------------------------------------------------------
    // Collect port information.

    auto portinfo_list = make_port_info_list();

    SAFE_CALL(gp_port_info_list_load(portinfo_list.get()), 1);
    SAFE_CALL(gp_port_info_list_count(portinfo_list.get()), 1);

    //-------------------------------------------------------------------------
    // Connect to each camera found and ask for it's serial number.

    for (int i = 0; i < num_cameras; ++i)
    {
        const char * name;
        const char * port;
        SAFE_CALL(gp_list_get_name(camera_list.get(), i, &name), 1);
        SAFE_CALL(gp_list_get_value(camera_list.get(), i, &port), 1);

        // Look up port info.
        const int port_index = gp_port_info_list_lookup_path(portinfo_list.get(), port);
        if (port_index == GP_ERROR_UNKNOWN_PORT or port_index < GP_OK)
        {
            ERROR_LOG << "Failed to lookup port '" << port << "' using gp_port_info_list_lookup_path()\n";
            continue;
        }

        GPPortInfo port_info;
        SAFE_CALL(gp_port_info_list_get_info(portinfo_list.get(), port_index, &port_info), 1);

        //-------------------------------------------------------------------------
        // Connect.

        Camera * camera {nullptr};
        SAFE_CALL(gp_camera_new(&camera), 1);

        // Set camera port info.
        SAFE_CALL(gp_camera_set_port_info(camera, port_info), 1);

        // Connect to the camera, loads backend driver.
        auto response = gp_camera_init(camera, ctx.get());
        if (response < GP_OK)
        {
            std::cerr << "Failed to connect to port index " << i << "\n"
                      << "Try unmounting it.\n";

            continue;
        }

        auto get_config = [&](const std::string & key) -> std::string
        {
            CameraWidget *widget {nullptr};
            SAFE_CALL(gp_camera_get_single_config(camera, key.c_str(), &widget, ctx.get()), "__ERROR__");
            const char * value;
            SAFE_CALL(gp_widget_get_value(widget, &value), "__ERROR__");
            auto str = std::string(value);
            if (key == "serialnumber")
            {
                lstrip(str, '0');
            }

            SAFE_CALL(gp_widget_free(widget), "__ERROR__");
            return str;
        };

        auto serial_number = get_config("serialnumber");
        auto make = get_config("manufacturer");
        auto model = get_config("cameramodel");

        filter_manufacture(make);

        auto info = CameraInfo();
        snprintf(info.serial, MAX_LEN, "%s", serial_number.c_str());
        snprintf(info.port, MAX_LEN, "%s", port);
        snprintf(info.desc, MAX_LEN, "%s %s", make.c_str(), model.c_str());

        out.push_back(info);

        SAFE_CALL(gp_camera_free(camera), 1);
    }

    return 0;
}


int main(int argc, char ** argv)
{
    using namespace std::chrono_literals;

    constexpr auto duration = 3s;

    std::vector<CameraInfo> detected;
    detected.reserve(16);
    while (true)
    {
        auto any_failure = scan_for_cameras(detected);

        if (any_failure)
        {
            std::this_thread::sleep_for(duration);
            continue;
        }

        DEBUG_LOG << "Detected " << detected.size() << " cameras\n";

        for (const auto & info : detected)
        {
            std::cout << "    "
                      << "SN: " << info.serial << " "
                      << "port: " << info.port << " "
                      << "desc: " << info.desc << "\n";
        }

        // Emit udp packet.
        {
            int sockfd;
            struct sockaddr_in multicastAddr;

            // create UDP socket.
            sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if (not sockfd < 0)
            {
                ERROR_LOG << "Failed to open socket!\n";
                continue;
            }

            // Set up the multicast address.
            memset(&multicastAddr, 0, sizeof(multicastAddr));
            multicastAddr.sin_family = AF_INET;

            // TODO: read these settings from config file.

            // Multicast address: https://stackoverflow.com/q/236231
            multicastAddr.sin_addr.s_addr = inet_addr("239.192.168.1");
            // Multicast port.
            multicastAddr.sin_port = htons(10'018);

            // TODO: Formally define this message, with message ID, etc.
            char message[16 * sizeof(CameraInfo)];

            char * ptr = message;

            // MESSAGE ID.
            auto nbytes = sprintf(ptr, "%16s\n", "CAM_INFO");
            if (nbytes < 0)
            {
                ERROR_LOG << "sprintf() failed\n";
                continue;
            }
            ptr += nbytes;

            // COUNT.
            nbytes = sprintf(ptr, "%4lu\n", detected.size());
            if (nbytes < 0)
            {
                ERROR_LOG << "sprintf() failed\n";
                continue;
            }
            ptr += nbytes;

            for (const auto & info : detected)
            {
                const auto len_serial = strlen(info.serial);
                const auto len_port = strlen(info.port);
                const auto len_desc = strlen(info.desc);
                const auto num_bytes = 3 * 6 + len_serial + len_port + len_desc;

                nbytes = sprintf(
                    ptr,
                    "%4lu %s\n"
                    "%4lu %s\n"
                    "%4lu %s\n",
                    len_serial, info.serial,
                    len_port, info.port,
                    len_desc, info.desc
                );
                if (nbytes < 0)
                {
                    ERROR_LOG << "sprintf() failed\n";
                    continue;
                }
                ptr += nbytes;
            }

            // Send the message
            auto result = sendto(
                sockfd,
                message,
                strlen(message),
                0,
                (struct sockaddr *)&multicastAddr,
                sizeof(multicastAddr)
            );

            if (result < 0)
            {
                ERROR_LOG << "Failed to send UDP message!\n";
                continue;
            }

        }


        // Sleep a bit before scanning again.
        std::this_thread::sleep_for(duration);
    }
    return 0;
}
