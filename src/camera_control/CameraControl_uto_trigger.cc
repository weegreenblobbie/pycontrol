#include <camera_control/CameraControl_uto.h>

TEST_CASE("CameraControl", "[CameraControl][trigger]")
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
    CHECK( data.time == 1'000 );
    CHECK( data.command_response.id == 0 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 1 );

    // z7
    auto obj1 = data.detected_cameras[0];
    CHECK( obj1.connected == true );
    CHECK( obj1.serial == "1234" );
    CHECK( obj1.port == "usb:001,001" );
    CHECK( obj1.desc == "Nikon Corporation Z 7" );
    CHECK( obj1.mode == "M" );
    CHECK( obj1.shutter == "1/1000" );
    CHECK( obj1.fstop == "F/8" );
    CHECK( obj1.iso == "64" );
    CHECK( obj1.quality == "NEF (Raw)" );
    CHECK( obj1.batt == "100%" );
    CHECK( obj1.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    CHECK( cam1->trigger_count == 0 );

    //-------------------------------------------------------------------------
    // Trigger the camera.
    //
    harness.cmd_socket.to_recv("1 trigger 1234");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 1'050 );
    CHECK( data.command_response.id == 1 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 1 );

    // z7
    obj1 = data.detected_cameras[0];
    CHECK( obj1.connected == true );
    CHECK( obj1.serial == "1234" );
    CHECK( obj1.port == "usb:001,001" );
    CHECK( obj1.desc == "Nikon Corporation Z 7" );
    CHECK( obj1.mode == "M" );
    CHECK( obj1.shutter == "1/1000" );
    CHECK( obj1.fstop == "F/8" );
    CHECK( obj1.iso == "64" );
    CHECK( obj1.quality == "NEF (Raw)" );
    CHECK( obj1.batt == "100%" );
    CHECK( obj1.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    CHECK( cam1->trigger_count == 1 );
}
