#pragma once

#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>


namespace pycontrol
{

using EventId = std::string;
using Serial = std::string;
using CamId = std::string;
using UsbPort = std::string;
using milliseconds = std::int64_t;

constexpr milliseconds MAX_TIME = std::numeric_limits<milliseconds>::max();


struct KeyValuePair
{
    std::string key;
    std::string value;
};

using kv_pair_vec = std::vector<KeyValuePair>;
using str_vec = std::vector<std::string>;
using str_set = std::set<std::string>;
using map_str = std::map<std::string, std::string>;

struct result
{
    static const bool failure = true;
    static const bool success = false;

    result(const bool b = false) : _value(b) {}

    operator bool() const
    {
        return _value;
    }

private:
    bool _value {false};

};


}