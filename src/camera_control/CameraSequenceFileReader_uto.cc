#include <fstream>
#include <cstdio> // For std::remove

// This tells Catch to provide a main() - only do this in one cpp file
#define CATCH_CONFIG_MAIN

#include <catch2/catch_test_macros.hpp>

#include <camera_control/CameraSequenceFileReader.h>

using namespace pycontrol;

// Helper function to create a temporary test file
void create_test_file(const std::string& filename, const std::string& content)
{
    std::ofstream file(filename);
    REQUIRE(file.is_open()); // Ensure the file was opened successfully
    file << content;
    file.close();
}

// Helper function to clean up a temporary test file
void cleanup_test_file(const std::string& filename)
{
    std::remove(filename.c_str());
}

TEST_CASE("CameraSequenceFileReader: Successful Parsing of Valid File", "[CameraSequenceFileReader][Success]")
{
    const std::string test_filename = "valid_config.txt";
    const std::string content = R"(
        event0     0      camA.shutter_speed    1/125
        event1     1.5    camB.fstop            f/8
        event2     2      camA.iso              400
        event3     2.1    camB.quality          RAW
        event4     2.2    camA.fps              5.0
        event5     2.2    camA.fps              start
        event6     5      camA.fps              stop
        event7   -10      camC.shutter_speed    1/4000
        event8   100.0    camC.trigger          1
    )";
    create_test_file(test_filename, content);

    CameraSequenceFileReader sequence_reader;
    REQUIRE(result::success == sequence_reader.read_file(test_filename));

    const auto& entries = sequence_reader.get_events();
    REQUIRE(entries.size() == 9);

    // Verify a few specific entries
    REQUIRE(entries[0].event_id == "event0");
    REQUIRE(entries[0].event_time_offset_ms == 0);
    REQUIRE(entries[0].camera_id == "camA");
    REQUIRE(entries[0].channel == Channel::shutter_speed);
    REQUIRE(entries[0].channel_value == "1/125");

    REQUIRE(entries[1].event_id == "event1");
    REQUIRE(entries[1].event_time_offset_ms == 1'500);
    REQUIRE(entries[1].camera_id == "camB");
    REQUIRE(entries[1].channel == Channel::fstop);
    REQUIRE(entries[1].channel_value == "f/8");

    REQUIRE(entries[2].event_id == "event2");
    REQUIRE(entries[2].event_time_offset_ms == 2'000);
    REQUIRE(entries[2].camera_id == "camA");
    REQUIRE(entries[2].channel == Channel::iso);
    REQUIRE(entries[2].channel_value == "400");

    REQUIRE(entries[3].event_id == "event3");
    REQUIRE(entries[3].event_time_offset_ms == 2'100);
    REQUIRE(entries[3].camera_id == "camB");
    REQUIRE(entries[3].channel == Channel::quality);
    REQUIRE(entries[3].channel_value == "RAW");

    REQUIRE(entries[4].event_id == "event4");
    REQUIRE(entries[4].event_time_offset_ms == 2'200);
    REQUIRE(entries[4].camera_id == "camA");
    REQUIRE(entries[4].channel == Channel::fps);
    REQUIRE(entries[4].channel_value == "5.0");

    REQUIRE(entries[5].event_id == "event5");
    REQUIRE(entries[5].event_time_offset_ms == 2'200);
    REQUIRE(entries[5].camera_id == "camA");
    REQUIRE(entries[5].channel == Channel::fps);
    REQUIRE(entries[5].channel_value == "start");

    REQUIRE(entries[6].event_id == "event6");
    REQUIRE(entries[6].event_time_offset_ms == 5'000);
    REQUIRE(entries[6].camera_id == "camA");
    REQUIRE(entries[6].channel == Channel::fps);
    REQUIRE(entries[6].channel_value == "stop");

    REQUIRE(entries[7].event_id == "event7");
    REQUIRE(entries[7].event_time_offset_ms == -10'000);
    REQUIRE(entries[7].camera_id == "camC");
    REQUIRE(entries[7].channel == Channel::shutter_speed);
    REQUIRE(entries[7].channel_value == "1/4000");

    REQUIRE(entries[8].event_id == "event8");
    REQUIRE(entries[8].event_time_offset_ms == 100'000);
    REQUIRE(entries[8].camera_id == "camC");
    REQUIRE(entries[8].channel == Channel::trigger);
    REQUIRE(entries[8].channel_value == "1");

    sequence_reader.clear();
    REQUIRE(entries.empty() == true);

    cleanup_test_file(test_filename);
}

TEST_CASE("CameraSequenceFileReader: Ignores Comments and Blank Lines", "[CameraSequenceFileReader][Ignore]")
{
    const std::string test_filename = "comments_blanks.txt";
    const std::string content = R"(
        # This is a full-line comment
            # This is a commented line with leading space

        event_1 1.0 camA.iso 100      # End of o line comments.

            # Another comment
           event_2 2.0 camB.quality JPEG fine

        event_3 3.0 camC.fps 29.97  # Also an ignored comment.

    )";
    create_test_file(test_filename, content);

    CameraSequenceFileReader sequence_reader;
    REQUIRE(result::success == sequence_reader.read_file(test_filename));

    const auto& entries = sequence_reader.get_events();
    REQUIRE(entries.size() == 3); // Only 3 actual data lines

    REQUIRE(entries[0].event_id == "event_1");
    REQUIRE(entries[1].event_id == "event_2");
    REQUIRE(entries[2].event_id == "event_3");

    REQUIRE(entries[0].channel_value == "100");
    REQUIRE(entries[1].channel_value == "JPEG fine");
    REQUIRE(entries[2].channel_value == "29.97");

    cleanup_test_file(test_filename);
}


TEST_CASE("CameraSequenceFileReader: event time offset format", "[CameraSequenceFileReader][HH:MM:SS]")
{
    const std::string test_filename = "event_time_offsets.txt";
    const std::string content = R"(
        C1      -15          z7.iso    800
        C1      -15.123      z7.iso    800

        C1    59:01.456      z7.iso    800
        C1   -59:10          z7.iso    800
        C1    61:10          z7.iso    800

        C1    1:01:01.789    z7.iso    800
        C1   -1:02:10.012    z7.iso    800

    )";
    create_test_file(test_filename, content);

    CameraSequenceFileReader sequence_reader;
    REQUIRE(result::success == sequence_reader.read_file(test_filename));

    const auto& entries = sequence_reader.get_events();
    REQUIRE(entries.size() == 7);

    REQUIRE(entries[0].event_time_offset_ms ==    -15'000);
    REQUIRE(entries[1].event_time_offset_ms ==    -15'123);

    REQUIRE(entries[2].event_time_offset_ms ==  3'541'456);
    REQUIRE(entries[3].event_time_offset_ms == -3'550'000);
    REQUIRE(entries[4].event_time_offset_ms ==  3'670'000);

    REQUIRE(entries[5].event_time_offset_ms ==  3'661'789);
    REQUIRE(entries[6].event_time_offset_ms == -3'730'012);

    cleanup_test_file(test_filename);
}


TEST_CASE("CameraSequenceFileReader: Failures", "[CameraSequenceFileReader][Failure]")
{
    CameraSequenceFileReader sequence_reader;
    REQUIRE(result::failure == sequence_reader.read_file("non_existent_file.txt"));
    REQUIRE(sequence_reader.get_events().empty());

    std::vector<std::string> test_cases = {
        // Missing 4th column.
        "event_start    0.0    camA.shutter_speed\n",

        // Extra 5th columns.
        "event_start    0.0    camA.shutter_speed    1/125    extra_column\n",
        "event_start  - 0.0    camA.shutter_speed    1/125\n",

        // Invalid event time offsets.
        "event_start    bad    camA.shutter_speed    1/125\n",

        // Missing dot in channel.
        "event_start    0.0    camA_shutter_speed    1/125\n",

        // Empty camera ID.
        "event_start    0.0        .shutter_speed    1/125\n",

        // Empty channel name.
        "event_start    0.0    camA.                 1/125\n",

        // Invalid channel name.
        "event_start    0.0    camA.unknown          1/125\n",

        // Invalid shutter speed value.
        "event_start    0.0    camA.shutter_speed    1/999\n",

        // Invalid fstop value.
        "event_start    0.0    camA.fstop            f/6.1\n",

        // Invalid fps value.
        "event_start    0.0    camA.fps              abc\n",

        // Invalid fps float.
        "event_start    0.0    camA.fps              1.2.3\n",
        "event_start    0.0    camA.fps              -1.0\n",

        // Invalid trigger.
        "event_start    0.0    camA.trigger           66\n",
        "event_start    0.0    camA.trigger           0\n",

        // Mixure of valid and invalid lines.
        R"(
            event_1        1.0 camA.iso               100
            invalid_line_1 3.0 camC.bad_channel       X
        )",
        R"(
            invalid_line_1 3.0 camC.bad_channel       X
            event_1        1.0 camA.iso               100
        )",

        // Invalid HH:MM:SS formats.
        "event_start     59:61    camA.iso          8000\n",
        "event_start   1:61:59    camA.iso          8000\n",
    };

    const std::string test_filename = "malformed_file.txt";

    for (const auto & content : test_cases)
    {
        create_test_file(test_filename, content);

        INFO("File content:\n    " << content << "EOF\n");

        REQUIRE(result::failure == sequence_reader.read_file(test_filename));
        REQUIRE(sequence_reader.get_events().empty());
    }

    cleanup_test_file(test_filename);
}
