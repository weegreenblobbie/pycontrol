#include <camera_control/CameraControl_uto_telem.h>

// This tells Catch to provide a main() - only do this in one cpp file
#define CATCH_CONFIG_MAIN

#include <catch2/catch_all.hpp>

// The heavy-duty external heaer.
#include <nlohmann/json.hpp>

#include <iostream>
#include <exception>

using json = nlohmann::json;
using pycontrol::str_vec;


Telem parse_telem(const std::string & json_str)
{
    Telem out;
    auto data = json::parse(json_str);

    out.state = data["state"];
    out.time = data["time"];

    auto cmd = data["command_response"];

    out.command_response.id = cmd["id"];
    out.command_response.success = cmd["success"];
    out.command_response.message = cmd.value("message", "");
    out.command_response.data = cmd.value("data", str_vec());

    for (const auto & cam_obj : data["detected_cameras"])
    {
        out.detected_cameras.emplace_back(
            cam_obj["connected"],
            cam_obj["serial"],
            cam_obj["port"],
            cam_obj["desc"],
            cam_obj["mode"],
            cam_obj["shutter"],
            cam_obj["fstop"],
            cam_obj["iso"],
            cam_obj["quality"],
            cam_obj["batt"],
            cam_obj["num_photos"]
        );
    }

    for (auto & [event_id, timestamp] : data["events"].items())
    {
        out.events[event_id] = timestamp;
    }

    out.sequence = data["sequence"];

    for (auto & data_seq_state : data["sequence_state"])
    {
        SequenceState ss;

        ss.num_events = data_seq_state["num_events"];
        ss.id = data_seq_state["id"];

        for (auto & cam_event : data_seq_state["events"])
        {
            ss.events.emplace_back(
                cam_event["pos"],
                cam_event["event_id"],
                cam_event["event_time_offset"],
                cam_event["eta"],
                cam_event["channel"],
                cam_event["value"]
            );
        }

        out.sequence_state.push_back(ss);
    }

    return out;
}
