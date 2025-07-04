#pragma once

// Including <nlohmann/json.hpp> directly in unit tests is too heavy, requiring
// a 1.5 minute compile time penality for each file including it.
//
// This is an interface to isolate the JSON string parsing to a single
// compiliation unit which should allow un-burden the rest of the unit tests.

#include <common/types.h>


struct CommandResponse
{
    unsigned int id;
    bool success;
    std::string message;
    std::vector<std::string> data;
};

struct DetectedCamera
{
    bool connected;
    std::string serial;
    std::string port;
    std::string desc;
    std::string mode;
    std::string shutter;
    std::string fstop;
    std::string iso;
    std::string quality;
    std::string batt;
    int num_photos;
};

struct CameraEvent
{
    unsigned int pos;
    std::string  event_id;
    std::string  event_time_offset;
    std::string  eta;
    std::string  channel;
    std::string  value;
};

struct SequenceState
{
    unsigned int num_events;
    std::string id;
    std::vector<CameraEvent> events;
};


struct Telem
{
    std::string state;
    pycontrol::milliseconds time;
    CommandResponse command_response;
    std::vector<DetectedCamera> detected_cameras;
    std::map<std::string, pycontrol::milliseconds> events;
    std::string sequence;
    std::vector<SequenceState> sequence_state;
};


Telem parse_telem(const std::string & json_str);
