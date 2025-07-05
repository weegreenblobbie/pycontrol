#include <catch2/catch_test_macros.hpp>

#include <camera_control/CameraControl_uto.h>

TEST_CASE("CameraControl", "[CameraControl][set_events]")
{
    Harness harness;

    //-------------------------------------------------------------------------
    // Inital message.
    //
    auto data = harness.dispatch_to_next_message();

    CHECK( data.state == "scan" );
    CHECK( data.time == 0 );
    CHECK( data.command_response.id == 0 );
    CHECK( data.command_response.success == true );
    CHECK( data.detected_cameras.empty() );
    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Set some events.
    //
    harness.cmd_socket.to_recv("1 set_events e1 1000 e2 2000 e3 3000");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 50 );
    CHECK( data.command_response.id == 1 );
    CHECK( data.command_response.success == true );
    CHECK( data.detected_cameras.empty() );
    REQUIRE( data.events.size() == 3 );

    CHECK( data.events["e1"] == 1'000 );
    CHECK( data.events["e2"] == 2'000 );
    CHECK( data.events["e3"] == 3'000 );

    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Omitting a complete pair, should produce an error.
    //
    harness.cmd_socket.to_recv("2 set_events e1 4000 e2 5000 e3");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 100 );
    CHECK( data.command_response.id == 2 );
    CHECK( data.command_response.success == false );
    CHECK( data.command_response.message == "failed to parse events from '2 set_events e1 4000 e2 5000 e3'" );
    CHECK( data.detected_cameras.empty() );
    REQUIRE( data.events.size() == 3 );

    CHECK( data.events["e1"] == 1'000 );
    CHECK( data.events["e2"] == 2'000 );
    CHECK( data.events["e3"] == 3'000 );

    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // No events?  Clears the exting events.
    //
    harness.cmd_socket.to_recv("3 set_events ");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 150 );
    CHECK( data.command_response.id == 3 );
    CHECK( data.command_response.success == true );
    CHECK( data.detected_cameras.empty() );
    REQUIRE( data.events.size() == 0 );

    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Extra whitespace should be ignored.
    //
    harness.cmd_socket.to_recv("4 set_events  e1 4000  e3  6000  e2 5000 ");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 200 );
    CHECK( data.command_response.id == 4 );
    CHECK( data.command_response.success == true );
    CHECK( data.command_response.message.empty() );
    CHECK( data.detected_cameras.empty() );
    REQUIRE( data.events.size() == 3 );

    CHECK( data.events["e1"] == 4'000 );
    CHECK( data.events["e2"] == 5'000 );
    CHECK( data.events["e3"] == 6'000 );

    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );
}
