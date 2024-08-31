#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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
str_vec split(const std::string & str);
void lstrip(std::string &str, char tok);
void rstrip(std::string &str, char tok);
void strip(std::string &str, char tok);

std::ostream & operator<<(std::ostream & out, const kv_pair_vec & rhs);
std::ostream & operator<<(std::ostream & out, const str_vec & rhs);

// Common conerstions.
template <typename T>
T as_type(const std::string & input);


//-----------------------------------------------------------------------------
// Inline implementations.
//-----------------------------------------------------------------------------
template <typename T>
T as_type(const std::string & input)
{
    auto sstream = std::stringstream(input);
    T out;
    sstream >> out;
    return out;
}

}
