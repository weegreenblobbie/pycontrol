#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>

#include <common/io.h>
#include <common/str_utils.h>

namespace pycontrol
{


// Reads the config file and retuns a vector of key value pairs.
// Return 0 on success.
result read_config(const std::string & config_file, kv_pair_vec & output)
{
    output.clear();

    std::ifstream fin(config_file);

    ABORT_IF_NOT(fin.is_open(), "error opening file '" << config_file << "'", result::failure);

    std::string line;
    while (std::getline(fin, line))
    {
        auto tokens = split(line);

        // Skip comments.
        if (tokens.size() > 0 and tokens[0].length() > 0 and tokens[0][0] == '#')
        {
            continue;
        }

        // Skip empty lines.
        if (tokens.size() == 0)
        {
            continue;
        }

        ABORT_IF(tokens.size() < 2, "expecing key value pair, got '" << tokens, result::failure);

        output.push_back({tokens[0], tokens[1]});
    }

    fin.close();

    return result::success;
}

str_vec split(const std::string & str, const std::string & sep)
{
    if (not sep.empty())
    {
        ABORT_IF_NOT(sep.length() == 1, "sep can only 1 char long", {});
    }

    str_vec output;

    std::string buffer;
    for (const auto & ch : str)
    {
        if ((sep.empty() and std::isspace(ch)) or ch == sep[0])
        {
            // If buffer is non-empty, put into the output vector and reset.
            if (not buffer.empty())
            {
                output.push_back(buffer);
                buffer.clear();
            }
        }
        else
        {
            buffer.push_back(ch);
        }
    }

    if (not buffer.empty())
    {
        output.push_back(buffer);
    }


    return output;
}



void lstrip(std::string &str, char tok)
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

void rstrip(std::string &str, char tok)
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

void strip(std::string &str, char tok)
{
    lstrip(str, tok);
    rstrip(str, tok);
}

std::ostream & operator<<(std::ostream & out, const kv_pair_vec & rhs)
{
    out << "[{";
    for (const auto & pair : rhs)
    {
        out << "key: '" << pair.key << "' value: '" << pair.value << "'},";
    }
    out << "}]";
    return out;
}

std::ostream & operator<<(std::ostream & out, const str_vec & rhs)
{
    out << "[";
    for (const auto & str : rhs)
    {
        out << "'" << str << "',";
    }
    out << "]";
    return out;
}

result
convert_hms_to_seconds(
    const std::string& hms_string,
    float & total_seconds
)
{
    total_seconds = 0.0f;

    ABORT_IF(hms_string.empty(), "noting to convert", result::failure);

    bool is_negative = false;
    std::string clean_string = hms_string;

    // Handle optional leading negative sign.
    if (clean_string[0] == '-')
    {
        is_negative = true;
        // Remove the '-'
        clean_string = clean_string.substr(1);
    }

    // Use stringstream to split by ':', store in parts.
    std::stringstream ss(clean_string);
    std::string segment;
    std::vector<float> parts;

    while (std::getline(ss, segment, ':'))
    {
        // Strip off any leading zeros.
//~        lstrip(segment, '0');
        float part;
        ABORT_IF(
            as_type<float>(segment, part),
            "as_type<float>('" << segment << "') failed",
            result::failure
        );
        parts.push_back(part);
    }

    ABORT_IF(
        parts.size() != 2 and parts.size() != 3,
        "Invalid MM:SS.sss format: " << hms_string,
        result::failure
    );

    if (parts.size() == 2)
    {
        // Format: M:S
        float minutes = parts[0];
        float seconds = parts[1];

        // Basic validation for minutes and seconds.
        ABORT_IF(
            minutes < 0.0f or
            seconds < 0.0f or
            seconds >= 60.0f,
            "Invalid MM:SS.sss format: " << hms_string,
            result::failure
        );

        total_seconds = minutes * 60.0 + seconds;
    }
    else if (parts.size() == 3)
    {
        // Format: H:M:S
        float hours = parts[0];
        float minutes = parts[1];
        float seconds = parts[2];

        // Basic validation for minutes and seconds.
        ABORT_IF(
            hours < 0.0f or
            minutes < 0.0f or
            seconds < 0.0f or
            minutes >= 60.0f or
            seconds >= 60.0f,
            "Invalid HH:MM:SS.sss format: " << hms_string,
            result::failure
        );
        total_seconds = hours * 3600.0f + minutes * 60.0f + seconds;
    }

    if (is_negative)
    {
        total_seconds *= -1.0f;
    }

    return result::success;
}


} /* namespace */