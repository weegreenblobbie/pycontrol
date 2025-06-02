#include <fstream>
#include <cstdio> // For std::remove

// This tells Catch to provide a main() - only do this in one cpp file
#define CATCH_CONFIG_MAIN

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <camera_control/CameraSequence.h>

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

TEST_CASE("CameraSequence: Successful Parsing of Valid File", "[CameraSequence][Success]")
{
    const std::string test_filename = "valid_config.txt";
    const std::string content = R"(
        event_start       0.0    camA.shutter_speed    1/125
        event_focus       1.5    camB.fstop            8.0
        event_trigger     2.0    camA.iso              400
        event_burst       2.1    camB.quality          RAW
        event_burst       2.2    camA.fps              5.0
        event_burst       2.2    camA.fps              start
        event_stop        5.0    camA.fps              stop
        another_event   -10.0    camC.shutter_speed    1/4000
    )";
    create_test_file(test_filename, content);

    CameraSequence sequence_reader;
    bool success = sequence_reader.load_from_file(test_filename);
    REQUIRE(success == true);

    const auto& entries = sequence_reader.get_entries();
    REQUIRE(entries.size() == 8);

    // Verify a few specific entries
    REQUIRE(entries[0].event_id == "event_start");
    REQUIRE(entries[0].event_time_offset_seconds == Catch::Approx(0.0f));
    REQUIRE(entries[0].camera_id == "camA");
    REQUIRE(entries[0].channel_name == "shutter_speed");
    REQUIRE(entries[0].channel_value == "1/125");

    REQUIRE(entries[4].event_id == "event_burst");
    REQUIRE(entries[4].event_time_offset_seconds == Catch::Approx(2.2f));
    REQUIRE(entries[4].camera_id == "camA");
    REQUIRE(entries[4].channel_name == "fps");
    REQUIRE(entries[4].channel_value == "5.0");

    REQUIRE(entries[7].event_id == "another_event");
    REQUIRE(entries[7].event_time_offset_seconds == Catch::Approx(-10.0f));
    REQUIRE(entries[7].camera_id == "camC");
    REQUIRE(entries[7].channel_name == "shutter_speed");
    REQUIRE(entries[7].channel_value == "1/4000");

    cleanup_test_file(test_filename);
}

TEST_CASE("CameraSequence: Ignores Comments and Blank Lines", "[CameraSequence][Ignore]")
{
    const std::string test_filename = "comments_blanks.txt";
    const std::string content = R"(
        # This is a full-line comment
            # This is a commented line with leading space

        event_1 1.0 camA.iso 100

           event_2 2.0 camB.quality JPEG

            # Another comment
        event_3 3.0 camC.fps 29.97

    )";
    create_test_file(test_filename, content);

    CameraSequence sequence_reader;
    bool success = sequence_reader.load_from_file(test_filename);
    REQUIRE(success == true);

    const auto& entries = sequence_reader.get_entries();
    REQUIRE(entries.size() == 3); // Only 3 actual data lines

    REQUIRE(entries[0].event_id == "event_1");
    REQUIRE(entries[1].event_id == "event_2");
    REQUIRE(entries[2].event_id == "event_3");

    cleanup_test_file(test_filename);
}

TEST_CASE("CameraSequence: Failure - Non-Existent File", "[CameraSequence][Failure]")
{
    CameraSequence sequence_reader;
    bool success = sequence_reader.load_from_file("non_existent_file.txt");
    REQUIRE(success == false);
    REQUIRE(sequence_reader.get_entries().empty()); // Should not add any entries
}

TEST_CASE("CameraSequence: Failure - Incorrect Number of Columns (Too Few)", "[CameraSequence][Failure]")
{
    const std::string test_filename = "too_few_cols.txt";
    // Missing 4th column.
    const std::string content = "event_start 0.0 camA.shutter_speed\n";
    create_test_file(test_filename, content);

    CameraSequence sequence_reader;
    bool success = sequence_reader.load_from_file(test_filename);
    REQUIRE(success == false);
    // Should be empty due to parse error.
    REQUIRE(sequence_reader.get_entries().empty());

    cleanup_test_file(test_filename);
}

TEST_CASE("CameraSequence: Failure - Incorrect Number of Columns (Too Many)", "[CameraSequence][Failure]")
{
    const std::string test_filename = "too_many_cols.txt";
    const std::string content = "event_start 0.0 camA.shutter_speed 1/125 extra_column\n"; // Extra 5th column
    create_test_file(test_filename, content);

    CameraSequence sequence_reader;
    bool success = sequence_reader.load_from_file(test_filename);
    REQUIRE(success == false);
    // Should be empty due to parse error.
    REQUIRE(sequence_reader.get_entries().empty());

    cleanup_test_file(test_filename);
}

TEST_CASE("CameraSequence: Failure - Invalid Time Offset Format", "[CameraSequence][Failure]")
{
    const std::string test_filename = "invalid_time_offset.txt";
    const std::string content = "event_start INVALID_FLOAT camA.shutter_speed 1/125\n"; // Invalid float
    create_test_file(test_filename, content);

    CameraSequence sequence_reader;
    bool success = sequence_reader.load_from_file(test_filename);
    REQUIRE(success == false);
    REQUIRE(sequence_reader.get_entries().empty());

    cleanup_test_file(test_filename);
}

TEST_CASE("CameraSequence: Failure - Malformed Camera.Channel (Missing Dot)", "[CameraSequence][Failure]")
{
    const std::string test_filename = "malformed_cam_chan_missing_dot.txt";
    const std::string content = "event_start 0.0 camA_shutter_speed 1/125\n"; // Missing dot
    create_test_file(test_filename, content);

    CameraSequence sequence_reader;
    bool success = sequence_reader.load_from_file(test_filename);
    REQUIRE(success == false);
    REQUIRE(sequence_reader.get_entries().empty());

    cleanup_test_file(test_filename);
}

TEST_CASE("CameraSequence: Failure - Malformed Camera.Channel (Empty Camera ID)", "[CameraSequence][Failure]")
{
    const std::string test_filename = "malformed_cam_chan_empty_cam_id.txt";
    const std::string content = "event_start 0.0 .shutter_speed 1/125\n"; // Empty camera ID
    create_test_file(test_filename, content);

    CameraSequence sequence_reader;
    bool success = sequence_reader.load_from_file(test_filename);
    REQUIRE(success == false);
    REQUIRE(sequence_reader.get_entries().empty());

    cleanup_test_file(test_filename);
}

TEST_CASE("CameraSequence: Failure - Malformed Camera.Channel (Empty Channel Name)", "[CameraSequence][Failure]")
{
    const std::string test_filename = "malformed_cam_chan_empty_chan_name.txt";
    const std::string content = "event_start 0.0 camA. 1/125\n"; // Empty channel name
    create_test_file(test_filename, content);

    CameraSequence sequence_reader;
    bool success = sequence_reader.load_from_file(test_filename);
    REQUIRE(success == false);
    REQUIRE(sequence_reader.get_entries().empty());

    cleanup_test_file(test_filename);
}

TEST_CASE("CameraSequence: Failure - Invalid Channel Name", "[CameraSequence][Failure]")
{
    const std::string test_filename = "invalid_channel_name.txt";
    const std::string content = "event_start 0.0 camA.unknown_channel some_value\n"; // Invalid channel name
    create_test_file(test_filename, content);

    CameraSequence sequence_reader;
    bool success = sequence_reader.load_from_file(test_filename);
    REQUIRE(success == false);
    REQUIRE(sequence_reader.get_entries().empty());

    cleanup_test_file(test_filename);
}

TEST_CASE("CameraSequence: Failure - Invalid Shutter Speed Value", "[CameraSequence][Failure]")
{
    const std::string test_filename = "invalid_shutter_speed.txt";
    const std::string content = "event_start 0.0 camA.shutter_speed 1/9999999\n"; // Not in hardcoded list
    create_test_file(test_filename, content);

    CameraSequence sequence_reader;
    bool success = sequence_reader.load_from_file(test_filename);
    REQUIRE(success == false);
    REQUIRE(sequence_reader.get_entries().empty());

    cleanup_test_file(test_filename);
}

TEST_CASE("CameraSequence: Failure - Invalid FStop Value", "[CameraSequence][Failure]")
{
    const std::string test_filename = "invalid_fstop.txt";
    const std::string content = "event_start 0.0 camB.fstop BAD\n"; // Not in hardcoded list
    create_test_file(test_filename, content);

    CameraSequence sequence_reader;
    bool success = sequence_reader.load_from_file(test_filename);
    REQUIRE(success == false);
    REQUIRE(sequence_reader.get_entries().empty());

    cleanup_test_file(test_filename);
}

TEST_CASE("CameraSequence: Failure - Invalid FPS Value (Non-numeric, not 'start'/'stop')", "[CameraSequence][Failure]")
{
    const std::string test_filename = "invalid_fps_value_non_num.txt";
    const std::string content = "event_start 0.0 camC.fps abc\n"; // Neither float nor "start"/"stop"
    create_test_file(test_filename, content);

    CameraSequence sequence_reader;
    bool success = sequence_reader.load_from_file(test_filename);
    REQUIRE(success == false);
    REQUIRE(sequence_reader.get_entries().empty());

    cleanup_test_file(test_filename);
}

TEST_CASE("CameraSequence: Failure - Invalid FPS Value (Malformed Float)", "[CameraSequence][Failure]")
{
    const std::string test_filename = "invalid_fps_value_malformed_float.txt";
    const std::string content = "event_start 0.0 camC.fps 1.2.3\n"; // Malformed float
    create_test_file(test_filename, content);

    CameraSequence sequence_reader;
    bool success = sequence_reader.load_from_file(test_filename);
    REQUIRE(success == false);
    REQUIRE(sequence_reader.get_entries().empty());

    cleanup_test_file(test_filename);
}

TEST_CASE("CameraSequence: Clear Functionality", "[CameraSequence][Clear]")
{
    const std::string test_filename = "clear_test.txt";
    const std::string content = "event_start 0.0 camA.shutter_speed 1/125\n";
    create_test_file(test_filename, content);

    CameraSequence sequence_reader;
    sequence_reader.load_from_file(test_filename);
    REQUIRE(sequence_reader.get_entries().size() == 1);

    sequence_reader.clear();
    REQUIRE(sequence_reader.get_entries().empty());

    cleanup_test_file(test_filename);
}

TEST_CASE("CameraSequence: Multiple Lines, Some Valid, Some Invalid", "[CameraSequence][Mixed]")
{
    const std::string test_filename = "mixed_lines.txt";
    const std::string content = R"(
        event_1        1.0 camA.iso               100
        event_2        2.0 camB.fstop             8.0
        invalid_line_1 3.0 camC.bad_channel       X
        event_3        4.0 camD.fps               30.0
        invalid_line_2 5.0 camE.shutter_speed     NOT_VALID
    )";
    create_test_file(test_filename, content);

    CameraSequence sequence_reader;
    bool success = sequence_reader.load_from_file(test_filename);
    // Should fail because any single error clears the entire sequence
    REQUIRE(success == false);
    REQUIRE(sequence_reader.get_entries().empty()); // No entries should be loaded if any error occurs

    cleanup_test_file(test_filename);
}
