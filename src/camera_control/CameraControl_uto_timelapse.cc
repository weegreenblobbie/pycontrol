#include <catch2/catch_test_macros.hpp>

#include <camera_control/CameraControl_uto.h>

TEST_CASE("CameraControl", "[CameraControl][timelapse]")
{
    Harness harness;

    //-------------------------------------------------------------------------
    // Add a camera, initially connected.
    //
    auto cam1 = make_test_camera();
    harness.gp2cpp.add_camera(cam1);
    auto data = harness.dispatch_to_next_message();
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.command_response.last_accepted_id == 0 );
    CHECK( data.command_response.last_rejected_id == 0 );
    CHECK( data.command_response.message.empty() );
    REQUIRE( data.detected_cameras.size() == 1 );

    //-------------------------------------------------------------------------
    // Enable timelapse
    //
    harness.cmd_socket.to_recv("1 timelapse_enable");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "timelapse_idle" );
    CHECK( data.command_response.last_accepted_id == 1 );
    CHECK( data.command_response.last_rejected_id == 0 );
    CHECK( data.command_response.message.empty() );

    //-------------------------------------------------------------------------
    // Update - Fail to parse serial
    //
    harness.cmd_socket.to_recv("2 timelapse_update");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "timelapse_idle" );
    CHECK( data.command_response.last_accepted_id == 1 );
    CHECK( data.command_response.last_rejected_id == 2 );
    CHECK( data.command_response.message == "Failed to parse timelapse serial from: '2 timelapse_update'" );

    //-------------------------------------------------------------------------
    // Update - Serial does not exist
    //
    harness.cmd_socket.to_recv("3 timelapse_update 9999");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "timelapse_idle" );
    CHECK( data.command_response.last_accepted_id == 1 );
    CHECK( data.command_response.last_rejected_id == 3 );
    CHECK( data.command_response.message == "serial '9999' does not exist" );

    //-------------------------------------------------------------------------
    // Update - Fail to parse interval
    //
    harness.cmd_socket.to_recv("4 timelapse_update 1234");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "timelapse_idle" );
    CHECK( data.command_response.last_accepted_id == 1 );
    CHECK( data.command_response.last_rejected_id == 4 );
    CHECK( data.command_response.message == "Failed to parse timelapse interval from: '4 timelapse_update 1234'" );

    //-------------------------------------------------------------------------
    // Update - Fail to parse min_shutter
    //
    harness.cmd_socket.to_recv("5 timelapse_update 1234 5.0");
    data = harness.dispatch_to_next_message();

    CHECK( data.command_response.last_rejected_id == 5 );
    CHECK( data.command_response.message == "Failed to parse timelapse min_shutter from: '5 timelapse_update 1234 5.0'" );

    //-------------------------------------------------------------------------
    // Update - Fail to parse max_shutter
    //
    harness.cmd_socket.to_recv("6 timelapse_update 1234 5.0 1/10");
    data = harness.dispatch_to_next_message();

    CHECK( data.command_response.last_rejected_id == 6 );
    CHECK( data.command_response.message == "Failed to parse timelapse max_shutter from: '6 timelapse_update 1234 5.0 1/10'" );

    //-------------------------------------------------------------------------
    // Update - Fail to parse min_iso
    //
    harness.cmd_socket.to_recv("7 timelapse_update 1234 5.0 1/10 1/1000");
    data = harness.dispatch_to_next_message();

    CHECK( data.command_response.last_rejected_id == 7 );
    CHECK( data.command_response.message == "Failed to parse timelapse min_iso from: '7 timelapse_update 1234 5.0 1/10 1/1000'" );

    //-------------------------------------------------------------------------
    // Update - Fail to parse max_iso
    //
    harness.cmd_socket.to_recv("8 timelapse_update 1234 5.0 1/10 1/1000 100");
    data = harness.dispatch_to_next_message();

    CHECK( data.command_response.last_rejected_id == 8 );
    CHECK( data.command_response.message == "Failed to parse timelapse max_iso from: '8 timelapse_update 1234 5.0 1/10 1/1000 100'" );

    //-------------------------------------------------------------------------
    // Update - Fail to parse min_hist_mask
    //
    harness.cmd_socket.to_recv("9 timelapse_update 1234 5.0 1/10 1/1000 100 800");
    data = harness.dispatch_to_next_message();

    CHECK( data.command_response.last_rejected_id == 9 );
    CHECK( data.command_response.message == "Failed to parse timelapse min_hist_mask from: '9 timelapse_update 1234 5.0 1/10 1/1000 100 800'" );

    //-------------------------------------------------------------------------
    // Update - Fail to parse max_hist_mask
    //
    harness.cmd_socket.to_recv("10 timelapse_update 1234 5.0 1/10 1/1000 100 800 10");
    data = harness.dispatch_to_next_message();

    CHECK( data.command_response.last_rejected_id == 10 );
    CHECK( data.command_response.message == "Failed to parse timelapse max_hist_mask from: '10 timelapse_update 1234 5.0 1/10 1/1000 100 800 10'" );

    //-------------------------------------------------------------------------
    // Update - Fail to parse min_deadband
    //
    harness.cmd_socket.to_recv("11 timelapse_update 1234 5.0 1/10 1/1000 100 800 10 245");
    data = harness.dispatch_to_next_message();

    CHECK( data.command_response.last_rejected_id == 11 );
    CHECK( data.command_response.message == "Failed to parse timelapse min_deadband from: '11 timelapse_update 1234 5.0 1/10 1/1000 100 800 10 245'" );

    //-------------------------------------------------------------------------
    // Update - Fail to parse max_deadband
    //
    harness.cmd_socket.to_recv("12 timelapse_update 1234 5.0 1/10 1/1000 100 800 10 245 5");
    data = harness.dispatch_to_next_message();

    CHECK( data.command_response.last_rejected_id == 12 );
    CHECK( data.command_response.message == "Failed to parse timelapse max_deadband from: '12 timelapse_update 1234 5.0 1/10 1/1000 100 800 10 245 5'" );

    //-------------------------------------------------------------------------
    // Update - Fail to parse target_offset
    //
    harness.cmd_socket.to_recv("13 timelapse_update 1234 5.0 1/10 1/1000 100 800 10 245 5 15");
    data = harness.dispatch_to_next_message();

    CHECK( data.command_response.last_rejected_id == 13 );
    CHECK( data.command_response.message == "Failed to parse timelapse target_offset from: '13 timelapse_update 1234 5.0 1/10 1/1000 100 800 10 245 5 15'" );

    //-------------------------------------------------------------------------
    // Update - Fail to parse target_percent
    //
    harness.cmd_socket.to_recv("14 timelapse_update 1234 5.0 1/10 1/1000 100 800 10 245 5 15 0");
    data = harness.dispatch_to_next_message();

    CHECK( data.command_response.last_rejected_id == 14 );
    CHECK( data.command_response.message == "Failed to parse timelapse target_percent from: '14 timelapse_update 1234 5.0 1/10 1/1000 100 800 10 245 5 15 0'" );

    //-------------------------------------------------------------------------
    // Update - Bad target_percent (> 1.0)
    //
    harness.cmd_socket.to_recv("15 timelapse_update 1234 5.0 1/10 1/1000 100 800 10 245 5 15 0 1.5");
    data = harness.dispatch_to_next_message();

    CHECK( data.command_response.last_rejected_id == 15 );
    CHECK( data.command_response.message == "Bad target_percent: 1.5" );

    //-------------------------------------------------------------------------
    // Update - Bad target_percent (< 0.0)
    //
    harness.cmd_socket.to_recv("16 timelapse_update 1234 5.0 1/10 1/1000 100 800 10 245 5 15 0 -0.5");
    data = harness.dispatch_to_next_message();

    CHECK( data.command_response.last_rejected_id == 16 );
    CHECK( data.command_response.message == "Bad target_percent: -0.5" );

    //-------------------------------------------------------------------------
    // Update - Success
    //
    harness.cmd_socket.to_recv("17 timelapse_update 1234 5.0 1/10 1/1000 100 800 10 245 5 15 0 0.5");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "timelapse_idle" );
    CHECK( data.command_response.last_accepted_id == 17 );
    CHECK( data.command_response.last_rejected_id == 16 );
    CHECK( data.command_response.message == "Bad target_percent: -0.5" );

    //-------------------------------------------------------------------------
    // Start timelapse
    //
    harness.cmd_socket.to_recv("18 timelapse_start");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "timelapse_running" );
    CHECK( data.command_response.last_accepted_id == 18 );
    CHECK( data.command_response.last_rejected_id == 16 );

    //-------------------------------------------------------------------------
    // Enable while already running (should remain running)
    //
    harness.cmd_socket.to_recv("19 timelapse_enable");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "timelapse_running" );
    CHECK( data.command_response.last_accepted_id == 19 );
    CHECK( data.command_response.last_rejected_id == 16 );

    //-------------------------------------------------------------------------
    // Stop timelapse
    //
    harness.cmd_socket.to_recv("20 timelapse_stop");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "timelapse_idle" );
    CHECK( data.command_response.last_accepted_id == 20 );
    CHECK( data.command_response.last_rejected_id == 16 );

    //-------------------------------------------------------------------------
    // Disable timelapse
    //
    harness.cmd_socket.to_recv("21 timelapse_disable");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.command_response.last_accepted_id == 21 );
    CHECK( data.command_response.last_rejected_id == 16 );
    CHECK( data.command_response.message == "Bad target_percent: -0.5" );
}