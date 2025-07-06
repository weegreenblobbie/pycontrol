#pragma once

#include <interface/UdpSocket.h>
#include <interface/GPhoto2Cpp.h>
#include <interface/WallClock.h>
#include <camera_control/CameraControl.h>
#include <camera_control/CameraControl_uto_telem.h>

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <filesystem>
#include <stdexcept>


using namespace pycontrol;


struct UtoSocket : interface::UdpSocket
{
    UtoSocket() = default;

    void reset();
    result recv(std::string & out) override;
    result send(const std::string & out) override;
    void to_recv(const std::string & message);
    str_vec & from_send();

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

test_camera_ptr
make_test_camera(
    const std::string & model = "Z 7",
    const std::string & port = "usb:001,001",
    const std::string & serial = "1234");

using gphoto2cpp::camera_ptr;


struct UtoGp2Cpp : interface::GPhoto2Cpp
{
    void add_camera(test_camera_ptr cam);

    str_vec auto_detect() override;
    bool list_files(const camera_ptr & camera, str_vec & out) override;
    camera_ptr open_camera(const std::string & port) override;
    str_vec read_choices(const camera_ptr & camera, const std::string & property) override;

    bool _read_config_result = true;

    bool read_config(const camera_ptr & camera) override;
    bool read_property(
        const camera_ptr & camera,
        const std::string & property,
        std::string & output) override;
    void reset_cache(const camera_ptr & camera) override;
    bool trigger(const camera_ptr & camera) override;
    bool write_config(camera_ptr & camera) override;
    bool write_property(
        camera_ptr & camera,
        const std::string & property,
        const std::string & value) override;

    bool wait_for_event(
        const camera_ptr & camera,
        const int timeout_ms,
        gphoto2cpp::Event & out) override;

private:

    std::set<test_camera_ptr> _test_cams;
    std::map<camera_ptr, test_camera_ptr> _camera_to_test;
    str_vec _filenames;

};

struct FakeClock : public interface::WallClock
{
    milliseconds now() override;
    milliseconds time_ms = 0;
};


struct TempFile
{
    TempFile(const std::string & filename, const std::string & content);
    ~TempFile();
    std::filesystem::path path;
};


struct Harness
{
    Harness();

    UtoSocket cmd_socket;
    UtoSocket tlm_socket;
    UtoGp2Cpp gp2cpp;
    FakeClock clock;
    CameraControl cc;

    result dispatch(milliseconds ms = 50);
    Telem dispatch_to(milliseconds destination);
    Telem dispatch_to_next_message(milliseconds ms = 50);
};

