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

str_vec split(const std::string & str)
{
    str_vec output;

    std::string buffer;
    for (const auto & ch : str)
    {
        if (std::isspace(ch))
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

}