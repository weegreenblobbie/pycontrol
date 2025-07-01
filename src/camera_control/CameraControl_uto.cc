#include <interface/UdpSocket.h>
#include <interface/GPhoto2Cpp.h>
#include <interface/WallClock.h>
#include <camera_control/CameraControl.h>

#include <fstream>
#include <filesystem>


// Print std::vector<std::string>.
std::ostream &
operator<< (std::ostream & oss, const std::vector<std::string> & vec)
{
    oss << "str_vec:\n";
    for (std::size_t i = 0; i < vec.size(); ++i)
    {
        oss << "    " << i << ": " << vec[i] << "\n";
    }

    return oss;
}

#include <catch2/catch_tostring.hpp>

// Add vector to string into Catch's machinery.
namespace Catch
{
template<>
struct StringMaker<std::vector<std::string>>
{
    static std::string convert(const std::vector<std::string>& vec)
    {
        // Use a stringstream to build the string representation
        std::stringstream ss;
        ss << "str_vec:\n";
        for (std::size_t i = 0; i < vec.size(); ++i)
        {
            ss << "    " << i << ": " << vec[i] << "\n";
        }
        return ss.str();
    }
};
} /* namespace Catch */


#include <catch2/catch_test_macros.hpp>

#include <stdexcept>


using namespace pycontrol;


struct UtoSocket : interface::UdpSocket
{
    UtoSocket()
        : _to_recv(),
          _from_send()
    {}

    void reset()
    {
        _from_send = {};
    }

    result recv(std::string & out) override
    {
        out = _to_recv;
        return result::success;
    }

    result send(const std::string & out) override
    {
        // CameraControl is using a std::stringstream with a reserved capacity
        // of something like 4048 chars.  std::stringstream.str() will return
        // a string with the same capacity.  But our real UdpSocket::send()
        // implementation uses strlen(char *), so we construct a std::string
        // from a char * to "read" only the first, null-terminated string.
        _from_send.emplace_back(out.c_str());
        return result::success;
    }

    void to_recv(const std::string & message)
    {
        _to_recv = message;
    }

    str_vec & from_send() { return _from_send; }

private:
    std::string _to_recv;
    str_vec _from_send;
};


using choice_map_t = std::map<std::string, str_vec>;

struct TestCamera
{
    std::string maker = "N/A";
    std::string model = "N/A";

    bool connected = false;
    std::string serial = "N/A";
    std::string port = "N/A";
    std::string desc = "N/A";
    std::string mode = "N/A";
    std::string shutter = "N/A";
    std::string fstop = "N/A";
    std::string iso = "N/A";
    std::string quality = "N/A";
    std::string batt_level = "N/A";
    std::string num_photos = "-1";

    choice_map_t choice_map;

    int open_count = 0;
    int read_choices_count = 0;
    int read_config_count = 0;
    int read_property_count = 0;
    int reset_cache_count = 0;
    int trigger_count = 0;
    int write_config_count = 0;
    int write_property_count = 0;

    bool read_config_result = true;
    bool read_property_result = true;
    bool trigger_result = true;
    bool write_config_result = true;
    bool write_property_result = true;

};


namespace GP2
{
    struct _Camera {};
}

using test_camera_ptr = std::shared_ptr<TestCamera>;


test_camera_ptr make_test_camera(
    const std::string & model = "Z 7",
    const std::string & port = "usb:001,001",
    const std::string & serial = "1234")
{
    auto tc = std::make_shared<TestCamera>();

    tc->maker = "Nikon Corporation";
    tc->model = model;
    tc->connected = true;
    tc->serial = serial;
    tc->port = port;
    tc->desc = tc->maker + " " + tc->model;
    tc->mode = "M";
    tc->shutter = "1/1000";
    tc->fstop = "F/8";
    tc->iso = "64";
    tc->quality = "NEF (Raw)";
    tc->batt_level = "100%";
    tc->num_photos = "850";

    tc->choice_map = {
        {"expprogram", {"M", "A", "S", "P"}},
        {"f-number", {"f/8", "f/5.6", "f/4"}},
        {"imagequality", {"NEF (Raw)", "JPEG Basic", "JPEG Fine"}},
        {"iso", {"64", "100", "200", "500"}},
        {"shutterspeed2", {"1/1000", "1/500", "1/250"}},
    };

    return tc;
}

using gphoto2cpp::camera_ptr;


struct UtoGp2Cpp : interface::GPhoto2Cpp
{
    void add_camera(test_camera_ptr cam)
    {
        if (not cam)
        {
            throw std::runtime_error("cam is null");
        }
        _test_cams.insert(cam);
    }

    str_vec auto_detect() override
    {
        str_vec out;
        for (auto & cam : _test_cams)
        {
            if (cam->connected)
            {
                out.push_back(cam->port);
            }
        }
        return out;
    }

    camera_ptr open_camera(const std::string & port) override
    {
        for (auto & test_cam : _test_cams)
        {
            if (test_cam->port == port)
            {
                auto ptr = std::make_shared<GP2::Camera>();
                _camera_to_test[ptr] = test_cam;
                test_cam->open_count++;
                return ptr;
            }
        }
        return nullptr;
    }

    str_vec read_choices(const camera_ptr & camera, const std::string & property) override
    {
        auto test_cam = _camera_to_test[camera];
        test_cam->read_choices_count++;
        return test_cam->choice_map[property];
    }

    bool _read_config_result = true;

    bool read_config(const camera_ptr & camera) override
    {
        auto test_cam = _camera_to_test[camera];
        test_cam->read_config_count++;
        return test_cam->read_config_result;
    }

    bool read_property(
        const camera_ptr & camera,
        const std::string & property,
        std::string & output)
    {
        auto test_cam = _camera_to_test[camera];
        test_cam->read_property_count++;

        if (property == "manufacturer")
        {
            if(test_cam->read_property_result)
            {
                output = test_cam->maker;
            }
        }

        else if (property == "cameramodel")
        {
            if(test_cam->read_property_result)
            {
                output = test_cam->model;
            }
        }

        else if (property == "serialnumber")
        {
            if(test_cam->read_property_result)
            {
                output = test_cam->serial;
            }
        }

        else if (property == "batterylevel")
        {
            if(test_cam->read_property_result)
            {
                output = test_cam->batt_level;
            }
        }

        else if (property == "shutterspeed2")
        {
            if(test_cam->read_property_result)
            {
                output = test_cam->shutter;
            }
        }

        else if (property == "expprogram")
        {
            if(test_cam->read_property_result)
            {
                output = test_cam->mode;
            }
        }

        else if (property == "f-number")
        {
            if(test_cam->read_property_result)
            {
                output = test_cam->fstop;
            }
        }

        else if (property == "iso")
        {
            if(test_cam->read_property_result)
            {
                output = test_cam->iso;
            }
        }

        else if (property == "imagequality")
        {
            if(test_cam->read_property_result)
            {
                output = test_cam->quality;
            }
        }

        else if (property == "availableshots")
        {
            if(test_cam->read_property_result)
            {
                output = test_cam->num_photos;
            }
        }
        else
        {
            return false;
        }

        return test_cam->read_property_result;
    }

    void reset_cache(const camera_ptr & camera) override
    {
        auto test_cam = _camera_to_test[camera];
        test_cam->reset_cache_count++;
    }

    bool trigger(const camera_ptr & camera) override
    {
        auto test_cam = _camera_to_test[camera];
        test_cam->trigger_count++;
        return test_cam->trigger_result;
    }

    bool write_config(camera_ptr & camera) override
    {
        auto test_cam = _camera_to_test[camera];
        test_cam->write_config_count++;
        return test_cam->write_config_result;
    }

    bool write_property(
        camera_ptr & camera,
        const std::string & property,
        const std::string & value) override
    {
        auto test_cam = _camera_to_test[camera];
        test_cam->write_property_count++;

        // Read-only property.
        if (property == "manufacturer")
        {
            return false;
        }

        // Read-only property.
        else if (property == "cameramodel")
        {
            return false;
        }

        // Read-only property.
        else if (property == "batterylevel")
        {
            return false;
        }

        else if (property == "shutterspeed2")
        {
            if(test_cam->write_property_result)
            {
                test_cam->shutter = value;
            }
        }

        else if (property == "expprogram")
        {
            if(test_cam->write_property_result)
            {
                test_cam->mode = value;
            }
        }

        else if (property == "f-number")
        {
            if(test_cam->write_property_result)
            {
                test_cam->fstop = value;
            }
        }

        else if (property == "iso")
        {
            if(test_cam->write_property_result)
            {
                test_cam->iso = value;
            }
        }

        else if (property == "imagequality")
        {
            if(test_cam->write_property_result)
            {
                test_cam->quality = value;
            }
        }

        else if (property == "availableshots")
        {
            if(test_cam->write_property_result)
            {
                test_cam->num_photos = value;
            }
        }
        else
        {
            return false;
        }

        return test_cam->write_property_result;
    }

private:

    std::set<test_camera_ptr> _test_cams;
    std::map<camera_ptr, test_camera_ptr> _camera_to_test;

};

struct FakeClock : public interface::WallClock
{
    milliseconds now() override
    {
        return time_ms;
    }

    milliseconds time_ms = 0;
};


struct TempFile
{
    TempFile(const std::string & filename, const std::string & content)
    {
        path = std::filesystem::temp_directory_path() / filename;
        std::ofstream file_stream(path);
        file_stream << content;
        file_stream.close();
    }
    ~TempFile()
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    std::filesystem::path path;
};


struct Harness
{
    Harness()
        : cmd_socket()
        , tlm_socket()
        , gp2cpp()
        , clock()
        , cc(cmd_socket, tlm_socket, gp2cpp, clock, {})
    {}

    UtoSocket cmd_socket;
    UtoSocket tlm_socket;
    UtoGp2Cpp gp2cpp;
    FakeClock clock;
    CameraControl cc;

    result dispatch(milliseconds ms = 50)
    {
        clock.time_ms += ms;
        return cc.dispatch();
    }

    std::string dispatch_to_next_message(milliseconds ms = 50)
    {
        auto & telem_vec = tlm_socket.from_send();
        auto current_size = telem_vec.size();
        while (telem_vec.size() == current_size)
        {
            clock.time_ms += ms;
            REQUIRE(cc.dispatch() == result::success);
        }
        return telem_vec[current_size];
    }

    str_vec _cleanup_files;

    void make_file(const std::string & filename, const std::string & content)
    {

    }

};


TEST_CASE("CameraControl", "[CameraControl][init]")
{
    Harness harness;
    REQUIRE(harness.tlm_socket.from_send().empty());
    REQUIRE(harness.dispatch() == result::success);
    REQUIRE(harness.tlm_socket.from_send().size() == 1);
    auto msg = harness.tlm_socket.from_send()[0];
    REQUIRE(msg ==
        R"({"state":"scan",)"
        R"("command_response":{"id":0,"success":true},)"
        R"("detected_cameras":[],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[])"
        "}"
    );
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":{"id":0,"success":true},)"
        R"("detected_cameras":[],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[])"
        "}"
    );
}

TEST_CASE("CameraControl", "[CameraControl][auto_detect]")
{
    Harness harness;

    //-------------------------------------------------------------------------
    // Add a camera, initially connected.
    //
    auto cam1 = make_test_camera();
    harness.gp2cpp.add_camera(cam1);
    auto msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"scan",)"
        R"("command_response":{"id":0,"success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Disconnect the camera.
    //
    cam1->connected = false;
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":{"id":0,"success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":false,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"N/A",)"
            R"("shutter":"N/A",)"
            R"("fstop":"N/A",)"
            R"("iso":"N/A",)"
            R"x("quality":"N/A",)x"
            R"("batt":"N/A",)"
            R"("num_photos":-1}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Plug the camera back in.
    //
    cam1->connected = true;
    cam1->port = "usb:001,002";
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":{"id":0,"success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,002",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Plug another camera in.
    //
    auto cam2 = make_test_camera("Z 8", "usb:001,003", "5678");
    harness.gp2cpp.add_camera(cam2);
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":{"id":0,"success":true},)"
        R"("detected_cameras":[)"
            // z7
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,002",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850},)"
            // z8
            R"({"connected":true,)"
            R"("serial":"5678",)"
            R"("port":"usb:001,003",)"
            R"("desc":"Nikon Corporation Z 8",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"

        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Unplug the z7 again.
    //
    cam1->connected = false;
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":{"id":0,"success":true},)"
        R"("detected_cameras":[)"
            // z7
            R"({"connected":false,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,002",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"N/A",)"
            R"("shutter":"N/A",)"
            R"("fstop":"N/A",)"
            R"("iso":"N/A",)"
            R"x("quality":"N/A",)x"
            R"("batt":"N/A",)"
            R"("num_photos":-1},)"
            // z8
            R"({"connected":true,)"
            R"("serial":"5678",)"
            R"("port":"usb:001,003",)"
            R"("desc":"Nikon Corporation Z 8",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"

        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Unplug the z8.
    //
    cam2->connected = false;
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":{"id":0,"success":true},)"
        R"("detected_cameras":[)"
            // z7
            R"({"connected":false,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,002",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"N/A",)"
            R"("shutter":"N/A",)"
            R"("fstop":"N/A",)"
            R"("iso":"N/A",)"
            R"x("quality":"N/A",)x"
            R"("batt":"N/A",)"
            R"("num_photos":-1},)"
            // z8
            R"({"connected":false,)"
            R"("serial":"5678",)"
            R"("port":"usb:001,003",)"
            R"("desc":"Nikon Corporation Z 8",)"
            R"("mode":"N/A",)"
            R"("shutter":"N/A",)"
            R"("fstop":"N/A",)"
            R"("iso":"N/A",)"
            R"x("quality":"N/A",)x"
            R"("batt":"N/A",)"
            R"("num_photos":-1}],)"

        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Fiddle with the dials and plug both back in.
    cam1->connected = true;
    cam1->port = "usb:001,004";
    cam1->fstop = "F/4";
    cam1->iso = "400";
    cam1->shutter = "1/400";

    cam2->connected = true;
    cam2->port = "usb:001,005";
    cam2->fstop = "F/8";
    cam2->iso = "800";
    cam2->shutter = "1/800";

    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":{"id":0,"success":true},)"
        R"("detected_cameras":[)"
            // z7
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,004",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/400",)"
            R"("fstop":"F/4",)"
            R"("iso":"400",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850},)"
            // z8
            R"({"connected":true,)"
            R"("serial":"5678",)"
            R"("port":"usb:001,005",)"
            R"("desc":"Nikon Corporation Z 8",)"
            R"("mode":"M",)"
            R"("shutter":"1/800",)"
            R"("fstop":"F/8",)"
            R"("iso":"800",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"

        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );
}


TEST_CASE("CameraControl", "[CameraControl][set_camera_id]")
{
    Harness harness;

    //-------------------------------------------------------------------------
    // Add a camera, initially connected.
    //
    auto cam1 = make_test_camera();
    harness.gp2cpp.add_camera(cam1);
    auto msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"scan",)"
        R"("command_response":)"
            R"({"id":0,)"
            R"("success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Rename the camera.
    //
    harness.cmd_socket.to_recv("1 set_camera_id 1234 z7");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":1,)"
            R"("success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"z7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Rename the camera, but forget to increment the command id, should be
    // ignored.
    //
    harness.cmd_socket.to_recv("1 set_camera_id 1234 znick");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":1,)"
            R"("success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"z7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Increment the command id, should actually rename the camera.
    //
    harness.cmd_socket.to_recv("2 set_camera_id 1234 znick");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":2,)"
            R"("success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"znick",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Rename a non existing camera.
    //
    harness.cmd_socket.to_recv("3 set_camera_id 5678 nothing");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":3,)"
            R"("success":false,)"
            R"("message":"serial "5678" does not exist"},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"znick",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Add another camerera.
    //
    auto cam2 = make_test_camera("Z 8", "usb:001,002", "5678");
    harness.gp2cpp.add_camera(cam2);
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":3,)"
            R"("success":false,)"
            R"("message":"serial "5678" does not exist"},)"
        R"("detected_cameras":[)"
            // znick
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"znick",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850},)"
            // z8
            R"({"connected":true,)"
            R"("serial":"5678",)"
            R"("port":"usb:001,002",)"
            R"("desc":"Nikon Corporation Z 8",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Rename the Z 8 to znick, which should fail.
    //
    harness.cmd_socket.to_recv("4 set_camera_id 5678 znick");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":4,)"
            R"("success":false,)"
            R"("message":"id "znick" already exists"},)"
        R"("detected_cameras":[)"
            // znick
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"znick",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850},)"
            // z8
            R"({"connected":true,)"
            R"("serial":"5678",)"
            R"("port":"usb:001,002",)"
            R"("desc":"Nikon Corporation Z 8",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Rename the Z 8 to z8.
    //
    harness.cmd_socket.to_recv("5 set_camera_id 5678 z8");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":5,)"
            R"("success":true},)"
        R"("detected_cameras":[)"
            // znick
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"znick",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850},)"
            // z8
            R"({"connected":true,)"
            R"("serial":"5678",)"
            R"("port":"usb:001,002",)"
            R"("desc":"z8",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );
}


TEST_CASE("CameraControl", "[CameraControl][set_events]")
{
    Harness harness;

    //-------------------------------------------------------------------------
    // Inital message.
    //
    auto msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"scan",)"
        R"("command_response":)"
            R"({"id":0,)"
            R"("success":true},)"
        R"("detected_cameras":[],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Set some events.
    //
    harness.cmd_socket.to_recv("1 set_events e1 1000 e2 2000 e3 3000");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":1,)"
            R"("success":true},)"
        R"("detected_cameras":[],)"
        R"("events":{)"
            R"("e1":1000,)"
            R"("e2":2000,)"
            R"("e3":3000},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Omitting a complete pair, deletes the event, seems okay to do as we may
    // be swtiching to a new set of events, so all pervious events are
    // discarded first.
    //
    harness.cmd_socket.to_recv("2 set_events e1 4000 e2 5000 e3");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":2,)"
            R"("success":true},)"
        R"("detected_cameras":[],)"
        R"("events":{)"
            R"("e1":4000,)"
            R"("e2":5000},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // No events?  Parse error and existing events get cleared.
    //
    harness.cmd_socket.to_recv("3 set_events ");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":3,)"
            R"("success":false,)"
            R"("message":"failed to parse events from '3 set_events '"},)"
        R"("detected_cameras":[],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );
}

TEST_CASE("CameraControl", "[CameraControl][load_sequence][reset_sequence]")
{
    Harness harness;

    //-------------------------------------------------------------------------
    // Inital message.
    //
    auto msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"scan",)"
        R"("command_response":)"
            R"({"id":0,)"
            R"("success":true},)"
        R"("detected_cameras":[],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Set a non-existing sequence.
    //
    harness.cmd_socket.to_recv("1 load_sequence /nope/does-not-exist.seq ");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":1,)"
            R"("success":false,)"
            R"("message":"failed to parse camera sequence file '/nope/does-not-exist.seq'},)"
        R"("detected_cameras":[],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Command an existing sequence.
    //
    auto seq1 = TempFile(
        "test.seq",
        R"(
            e1 -10.0 z7.trigger 1
            e1  -9.0 z7.trigger 1
        )"
    );

    harness.cmd_socket.to_recv("2 load_sequence " + seq1.path.string());
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":2,)"
            R"("success":true},)"
        R"("detected_cameras":[],)"
        R"("events":{},)"
        R"("sequence":")" + seq1.path.string() + R"(",)"
        R"("sequence_state":[)"
            R"({"num_events":2,)"
            R"("id":"z7",)"
            R"("events":[)"

                R"({"pos":1,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:10.000",)"
                R"("eta":"N/A",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"},)"

                R"({"pos":2,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:09.000",)"
                R"("eta":"N/A",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"})"
            "]}"
        "]}"
    );

    //-------------------------------------------------------------------------
    // Command another sequence
    //
    auto seq2 = TempFile(
        "test.seq",
        R"(
            e1 -11.0 z7.trigger 1
            e1 -10.0 z7.trigger 1
            e1  -9.0 z7.trigger 1
        )"
    );

    harness.cmd_socket.to_recv("3 load_sequence " + seq2.path.string());
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":3,)"
            R"("success":true},)"
        R"("detected_cameras":[],)"
        R"("events":{},)"
        R"("sequence":")" + seq2.path.string() + R"(",)"
        R"("sequence_state":[)"
            R"({"num_events":3,)"
            R"("id":"z7",)"
            R"("events":[)"

                R"({"pos":1,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:11.000",)"
                R"("eta":"N/A",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"},)"

                R"({"pos":2,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:10.000",)"
                R"("eta":"N/A",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"},)"

                R"({"pos":3,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:09.000",)"
                R"("eta":"N/A",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"})"
            "]}"
        "]}"
    );

    //-------------------------------------------------------------------------
    // Command the event times and show ETA update.
    //
    auto control_time = harness.cc.control_time();
    REQUIRE(control_time == 3050);
    harness.cmd_socket.to_recv("4 set_events e1 20050");
    msg = harness.dispatch_to_next_message();

    // Only 1 second should have elapsed.
    REQUIRE(control_time + 1'000 == harness.cc.control_time());
    auto e1_eta = 20'050 - harness.cc.control_time();
    REQUIRE(e1_eta == 16'000);

    // Now applying event time offsets below in the sequence should make sense.
    // 16 - 11 -> 5 seconds eta
    // 16 - 10 -> 6 seconds eta
    // 16 -  9 -> 7 seconds eta
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":4,)"
            R"("success":true},)"
        R"("detected_cameras":[],)"
        R"("events":{"e1":20050},)"
        R"("sequence":")" + seq2.path.string() + R"(",)"
        R"("sequence_state":[)"
            R"({"num_events":3,)"
            R"("id":"z7",)"
            R"("events":[)"

                R"({"pos":1,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:11.000",)"
                R"("eta":" 00:00:05.000",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"},)"

                R"({"pos":2,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:10.000",)"
                R"("eta":" 00:00:06.000",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"},)"

                R"({"pos":3,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:09.000",)"
                R"("eta":" 00:00:07.000",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"})"
            "]}"
        "]}"
    );

    //-------------------------------------------------------------------------
    // Plugin a camera.
    //
    auto cam1 = make_test_camera();
    harness.gp2cpp.add_camera(cam1);
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":4,)"
            R"("success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{"e1":20050},)"
        R"("sequence":")" + seq2.path.string() + R"(",)"
        R"("sequence_state":[)"
            R"({"num_events":3,)"
            R"("id":"z7",)"
            R"("events":[)"

                R"({"pos":1,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:11.000",)"
                R"("eta":" 00:00:04.000",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"},)"

                R"({"pos":2,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:10.000",)"
                R"("eta":" 00:00:05.000",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"},)"

                R"({"pos":3,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:09.000",)"
                R"("eta":" 00:00:06.000",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"})"
            "]}"
        "]}"
    );

    //-------------------------------------------------------------------------
    // Rename the camera.
    harness.cmd_socket.to_recv("5 set_camera_id 1234 z7");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"executing",)"
        R"("command_response":)"
            R"({"id":5,)"
            R"("success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"z7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{"e1":20050},)"
        R"("sequence":")" + seq2.path.string() + R"(",)"
        R"("sequence_state":[)"
            R"({"num_events":3,)"
            R"("id":"z7",)"
            R"("events":[)"

                R"({"pos":1,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:11.000",)"
                R"("eta":" 00:00:03.000",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"},)"

                R"({"pos":2,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:10.000",)"
                R"("eta":" 00:00:04.000",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"},)"

                R"({"pos":3,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:09.000",)"
                R"("eta":" 00:00:05.000",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"})"
            "]}"
        "]}"
    );

    //-------------------------------------------------------------------------
    // 3 seconds away from executing the first event in the sequence.
    // Now that the state is executing, we stop monitoring camera changes and
    // emit telemetry at 4 Hz.  Dispatch for 3 seconds such that the first
    // event is executed.
    for (int i = 0; i < 3 * 4; ++i)
    {
        msg = harness.dispatch_to_next_message();
    }
    REQUIRE(msg ==
        R"({"state":"executing",)"
        R"("command_response":)"
            R"({"id":5,)"
            R"("success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"z7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{"e1":20050},)"
        R"("sequence":")" + seq2.path.string() + R"(",)"
        R"("sequence_state":[)"
            R"({"num_events":3,)"
            R"("id":"z7",)"
            R"("events":[)"

                R"({"pos":2,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:10.000",)"
                R"("eta":" 00:00:01.000",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"},)"

                R"({"pos":3,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:09.000",)"
                R"("eta":" 00:00:02.000",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"})"
            "]}"
        "]}"
    );
    REQUIRE(cam1->trigger_count == 1);

    //-------------------------------------------------------------------------
    // Command the event times and show ETA update.
    //
    control_time = harness.cc.control_time();
    REQUIRE(control_time == 9050);
    harness.cmd_socket.to_recv("6 set_events e1 30300");
    msg = harness.dispatch_to_next_message();

    // Running at 4 Hz, 0.250 seconds should have elapsed.
    REQUIRE(control_time + 250 == harness.cc.control_time());
    e1_eta = 30'300 - harness.cc.control_time();
    REQUIRE(e1_eta == 21'000);
    // 21 - 10 => 11 seconds
    // 21 -  9 => 12 seconds
    REQUIRE(msg ==
        R"({"state":"executing",)"
        R"("command_response":)"
            R"({"id":6,)"
            R"("success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"z7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{"e1":30300},)"
        R"("sequence":")" + seq2.path.string() + R"(",)"
        R"("sequence_state":[)"
            R"({"num_events":3,)"
            R"("id":"z7",)"
            R"("events":[)"

                R"({"pos":2,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:10.000",)"
                R"("eta":" 00:00:11.000",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"},)"

                R"({"pos":3,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:09.000",)"
                R"("eta":" 00:00:12.000",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"})"
            "]}"
        "]}"
    );

    //-------------------------------------------------------------------------
    // Command the sequence to reset.  We should get 3 events back.
    harness.cmd_socket.to_recv("7 reset_sequence");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"executing",)"
        R"("command_response":)"
            R"({"id":7,)"
            R"("success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"z7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":")" + seq2.path.string() + R"(",)"
        R"("sequence_state":[)"
            R"({"num_events":3,)"
            R"("id":"z7",)"
            R"("events":[)"

                R"({"pos":1,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:11.000",)"
                R"("eta":"N/A",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"},)"

                R"({"pos":2,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:10.000",)"
                R"("eta":"N/A",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"},)"

                R"({"pos":3,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:09.000",)"
                R"("eta":"N/A",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"})"
            "]}"
        "]}"
    );

    //-------------------------------------------------------------------------
    // Dispatch until the first trigger event executes.
    harness.cmd_socket.to_recv("8 set_events e1 30300");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"execute_ready",)"
        R"("command_response":)"
            R"({"id":8,)"
            R"("success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"z7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{"e1":30300},)"
        R"("sequence":")" + seq2.path.string() + R"(",)"
        R"("sequence_state":[)"
            R"({"num_events":3,)"
            R"("id":"z7",)"
            R"("events":[)"

                R"({"pos":1,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:11.000",)"
                R"("eta":" 00:00:09.500",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"},)"

                R"({"pos":2,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:10.000",)"
                R"("eta":" 00:00:10.500",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"},)"

                R"({"pos":3,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:09.000",)"
                R"("eta":" 00:00:11.500",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"})"
            "]}"
        "]}"
    );

    //-------------------------------------------------------------------------
    // Dispatch until the first trigger event executes.
    for (int i = 0; i < 3 + 8*4; ++i)
    {
        msg = harness.dispatch_to_next_message();
    }
    REQUIRE(msg ==
        R"({"state":"executing",)"
        R"("command_response":)"
            R"({"id":8,)"
            R"("success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"z7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{"e1":30300},)"
        R"("sequence":")" + seq2.path.string() + R"(",)"
        R"("sequence_state":[)"
            R"({"num_events":3,)"
            R"("id":"z7",)"
            R"("events":[)"

                R"({"pos":2,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:10.000",)"
                R"("eta":" 00:00:01.000",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"},)"

                R"({"pos":3,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:09.000",)"
                R"("eta":" 00:00:02.000",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"})"
            "]}"
        "]}"
    );
    REQUIRE(cam1->trigger_count == 2);

    //-------------------------------------------------------------------------
    // Dispatch until the next trigger event executes.
    for (int i = 0; i < 4; ++i)
    {
        msg = harness.dispatch_to_next_message();
    }
    REQUIRE(msg ==
        R"({"state":"executing",)"
        R"("command_response":)"
            R"({"id":8,)"
            R"("success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"z7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{"e1":30300},)"
        R"("sequence":")" + seq2.path.string() + R"(",)"
        R"("sequence_state":[)"
            R"({"num_events":3,)"
            R"("id":"z7",)"
            R"("events":[)"

                R"({"pos":3,)"
                R"("event_id":"e1",)"
                R"("event_time_offset":"-00:00:09.000",)"
                R"("eta":" 00:00:01.000",)"
                R"("channel":"z7.trigger",)"
                R"("value":"1"})"
            "]}"
        "]}"
    );
    REQUIRE(cam1->trigger_count == 3);

    //-------------------------------------------------------------------------
    // Dispatch until the last trigger event executes.
    // With no upcoming events in the sequence, state transition to
    // execute_ready.
    for (int i = 0; i < 4; ++i)
    {
        msg = harness.dispatch_to_next_message();
    }
    REQUIRE(msg ==
        R"({"state":"execute_ready",)"
        R"("command_response":)"
            R"({"id":8,)"
            R"("success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"z7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{"e1":30300},)"
        R"("sequence":")" + seq2.path.string() + R"(",)"
        R"("sequence_state":[)"
            R"({"num_events":3,)"
            R"("id":"z7",)"
            R"("events":[]})"
        "]}"
    );
    REQUIRE(cam1->trigger_count == 4);
}

TEST_CASE("CameraControl", "[CameraControl][read_choices][set_choice]")
{
    Harness harness;

    //-------------------------------------------------------------------------
    // Inital message.
    //
    auto msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"scan",)"
        R"("command_response":)"
            R"({"id":0,)"
            R"("success":true},)"
        R"("detected_cameras":[],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Plugin a camera.
    //
    auto cam1 = make_test_camera();
    harness.gp2cpp.add_camera(cam1);
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":0,)"
            R"("success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Reading from a non-existing camera should fail.
    //
    harness.cmd_socket.to_recv("1 read_choices 9999 imagequality");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":1,)"
            R"("success":false,)"
            R"("message":"serial '9999' does not exist"},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Reading a non-existing property should fail.
    //
    harness.cmd_socket.to_recv("2 read_choices 1234 wow-factor");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":2,)"
            R"("success":false,)"
            R"("message":"property 'wow-factor' does not exist"},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // Reading existing properties.
    //
    harness.cmd_socket.to_recv("3 read_choices 1234 expprogram");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":3,)"
            R"("success":true,)"
            R"("data":["M","A","S","P"]},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    harness.cmd_socket.to_recv("4 read_choices 1234 f-number");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":4,)"
            R"("success":true,)"
            R"("data":["f/8","f/5.6","f/4"]},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    harness.cmd_socket.to_recv("5 read_choices 1234 imagequality");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":5,)"
            R"("success":true,)"
            R"x("data":["NEF (Raw)","JPEG Basic","JPEG Fine"]},)x"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    harness.cmd_socket.to_recv("6 read_choices 1234 iso");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":6,)"
            R"("success":true,)"
            R"("data":["64","100","200","500"]},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    harness.cmd_socket.to_recv("7 read_choices 1234 shutterspeed2");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":7,)"
            R"("success":true,)"
            R"("data":["1/1000","1/500","1/250"]},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // set_choice - missing value
    //
    harness.cmd_socket.to_recv("8 set_choice 999 shutterspeed2");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":8,)"
            R"("success":false,)"
            R"("message":"Failed to parse set_choice command: '8 set_choice 999 shutterspeed2'"},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // set_choice - bad serial
    //
    harness.cmd_socket.to_recv("9 set_choice 999 shutterspeed2 1/500");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":9,)"
            R"("success":false,)"
            R"("message":"serial '999' does not exist"},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // set_choice - bad property
    //
    harness.cmd_socket.to_recv("10 set_choice 1234 wow-factor 5000");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":10,)"
            R"("success":false,)"
            R"("message":"property 'wow-factor' does not exist"},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/1000",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // set_choice - good
    //
    harness.cmd_socket.to_recv("11 set_choice 1234 shutterspeed2 1/500");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":11,)"
            R"("success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/500",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"NEF (Raw)",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

    //-------------------------------------------------------------------------
    // set_choice - value with space.
    //
    harness.cmd_socket.to_recv("12 set_choice 1234 imagequality jpeg fine*");
    msg = harness.dispatch_to_next_message();
    REQUIRE(msg ==
        R"({"state":"monitor",)"
        R"("command_response":)"
            R"({"id":12,)"
            R"("success":true},)"
        R"("detected_cameras":[)"
            R"({"connected":true,)"
            R"("serial":"1234",)"
            R"("port":"usb:001,001",)"
            R"("desc":"Nikon Corporation Z 7",)"
            R"("mode":"M",)"
            R"("shutter":"1/500",)"
            R"("fstop":"F/8",)"
            R"("iso":"64",)"
            R"x("quality":"jpeg fine*",)x"
            R"("batt":"100%",)"
            R"("num_photos":850}],)"
        R"("events":{},)"
        R"("sequence":"",)"
        R"("sequence_state":[]})"
    );

}