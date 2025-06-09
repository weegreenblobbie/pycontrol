#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <common/io.h>
#include <common/types.h>

namespace pycontrol
{

struct KeyValuePair
{
    std::string key;
    std::string value;
};

using kv_pair_vec = std::vector<KeyValuePair>;
using str_vec = std::vector<std::string>;

result read_config(const std::string & config_file, kv_pair_vec & output);
str_vec split(const std::string & str, const std::string & sep="");
void lstrip(std::string &str, char tok);
void rstrip(std::string &str, char tok);
void strip(std::string &str, char tok);

std::ostream & operator<<(std::ostream & out, const kv_pair_vec & rhs);
std::ostream & operator<<(std::ostream & out, const str_vec & rhs);

// Common conerstions.
template <typename T>
result as_type(const std::string & input, T & output);

// Converts a HH:MM:SS.sss string to total seconds.
result
convert_hms_to_milliseconds(
    const std::string& hms_string,
    milliseconds & total_seconds
);

std::string
convert_milliseconds_to_hms(const milliseconds & total_milliseconds);

//-----------------------------------------------------------------------------
// Inline implementations.
//-----------------------------------------------------------------------------
template <typename T>
result
as_type(const std::string & input, T & output)
{
    auto sstream = std::stringstream(input);
    ABORT_IF_NOT(sstream >> output, "conversion failed", result::failure);
    return result::success;
}


} /* namespace */
