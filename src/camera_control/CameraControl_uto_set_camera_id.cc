#include <catch2/catch_test_macros.hpp>

#include <camera_control/CameraControl_uto.h>

TEST_CASE("CameraControl", "[CameraControl][set_camera_id]")
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
    CHECK( obj1.num_photos == 850 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Rename the camera.
    //
    harness.cmd_socket.to_recv("1 set_camera_id 1234 z7");
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
    CHECK( obj1.desc == "z7" );
    CHECK( obj1.mode == "M" );
    CHECK( obj1.shutter == "1/1000" );
    CHECK( obj1.fstop == "F/8" );
    CHECK( obj1.iso == "64" );
    CHECK( obj1.quality == "NEF (Raw)" );
    CHECK( obj1.batt == "100%" );
    CHECK( obj1.num_photos == 850 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Rename the camera, but forget to increment the command id, should be
    // ignored.
    //
    harness.cmd_socket.to_recv("1 set_camera_id 1234 znick");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 2'000 );
    CHECK( data.command_response.id == 1 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 1 );

    // z7
    obj1 = data.detected_cameras[0];
    CHECK( obj1.connected == true );
    CHECK( obj1.serial == "1234" );
    CHECK( obj1.port == "usb:001,001" );
    CHECK( obj1.desc == "z7" );
    CHECK( obj1.mode == "M" );
    CHECK( obj1.shutter == "1/1000" );
    CHECK( obj1.fstop == "F/8" );
    CHECK( obj1.iso == "64" );
    CHECK( obj1.quality == "NEF (Raw)" );
    CHECK( obj1.batt == "100%" );
    CHECK( obj1.num_photos == 850 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Increment the command id, should actually rename the camera.
    //
    harness.cmd_socket.to_recv("2 set_camera_id 1234 znick");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 2'050 );
    CHECK( data.command_response.id == 2 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 1 );

    // z7
    obj1 = data.detected_cameras[0];
    CHECK( obj1.connected == true );
    CHECK( obj1.serial == "1234" );
    CHECK( obj1.port == "usb:001,001" );
    CHECK( obj1.desc == "znick" );
    CHECK( obj1.mode == "M" );
    CHECK( obj1.shutter == "1/1000" );
    CHECK( obj1.fstop == "F/8" );
    CHECK( obj1.iso == "64" );
    CHECK( obj1.quality == "NEF (Raw)" );
    CHECK( obj1.batt == "100%" );
    CHECK( obj1.num_photos == 850 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Rename a non existing camera.
    //
    harness.cmd_socket.to_recv("3 set_camera_id 5678 nothing");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 2'100 );
    CHECK( data.command_response.id == 3 );
    CHECK( data.command_response.success == false );
    CHECK( data.command_response.message == "serial '5678' does not exist" );
    REQUIRE( data.detected_cameras.size() == 1 );

    // z7
    obj1 = data.detected_cameras[0];
    CHECK( obj1.connected == true );
    CHECK( obj1.serial == "1234" );
    CHECK( obj1.port == "usb:001,001" );
    CHECK( obj1.desc == "znick" );
    CHECK( obj1.mode == "M" );
    CHECK( obj1.shutter == "1/1000" );
    CHECK( obj1.fstop == "F/8" );
    CHECK( obj1.iso == "64" );
    CHECK( obj1.quality == "NEF (Raw)" );
    CHECK( obj1.batt == "100%" );
    CHECK( obj1.num_photos == 850 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Add another camerera.
    //
    auto cam2 = make_test_camera("Z 8", "usb:001,002", "5678");
    harness.gp2cpp.add_camera(cam2);
    data = harness.dispatch_to_next_message();
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 4'000 );
    CHECK( data.command_response.id == 3 );
    CHECK( data.command_response.success == false );
    CHECK( data.command_response.message == "serial '5678' does not exist" );
    REQUIRE( data.detected_cameras.size() == 2 );

    // z7
    obj1 = data.detected_cameras[0];
    CHECK( obj1.connected == true );
    CHECK( obj1.serial == "1234" );
    CHECK( obj1.port == "usb:001,001" );
    CHECK( obj1.desc == "znick" );
    CHECK( obj1.mode == "M" );
    CHECK( obj1.shutter == "1/1000" );
    CHECK( obj1.fstop == "F/8" );
    CHECK( obj1.iso == "64" );
    CHECK( obj1.quality == "NEF (Raw)" );
    CHECK( obj1.batt == "100%" );
    CHECK( obj1.num_photos == 850 );

    // z8
    auto obj2 = data.detected_cameras[1];
    CHECK( obj2.connected == true );
    CHECK( obj2.serial == "5678" );
    CHECK( obj2.port == "usb:001,002" );
    CHECK( obj2.desc == "Nikon Corporation Z 8" );
    CHECK( obj2.mode == "M" );
    CHECK( obj2.shutter == "1/1000" );
    CHECK( obj2.fstop == "F/8" );
    CHECK( obj2.iso == "64" );
    CHECK( obj2.quality == "NEF (Raw)" );
    CHECK( obj2.batt == "100%" );
    CHECK( obj2.num_photos == 850 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Rename the Z 8 to znick, which should fail.
    //
    harness.cmd_socket.to_recv("4 set_camera_id 5678 znick");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 4'050 );
    CHECK( data.command_response.id == 4 );
    CHECK( data.command_response.success == false );
    CHECK( data.command_response.message == "id 'znick' already exists" );
    REQUIRE( data.detected_cameras.size() == 2 );

    // z7
    obj1 = data.detected_cameras[0];
    CHECK( obj1.connected == true );
    CHECK( obj1.serial == "1234" );
    CHECK( obj1.port == "usb:001,001" );
    CHECK( obj1.desc == "znick" );
    CHECK( obj1.mode == "M" );
    CHECK( obj1.shutter == "1/1000" );
    CHECK( obj1.fstop == "F/8" );
    CHECK( obj1.iso == "64" );
    CHECK( obj1.quality == "NEF (Raw)" );
    CHECK( obj1.batt == "100%" );
    CHECK( obj1.num_photos == 850 );

    // z8
    obj2 = data.detected_cameras[1];
    CHECK( obj2.connected == true );
    CHECK( obj2.serial == "5678" );
    CHECK( obj2.port == "usb:001,002" );
    CHECK( obj2.desc == "Nikon Corporation Z 8" );
    CHECK( obj2.mode == "M" );
    CHECK( obj2.shutter == "1/1000" );
    CHECK( obj2.fstop == "F/8" );
    CHECK( obj2.iso == "64" );
    CHECK( obj2.quality == "NEF (Raw)" );
    CHECK( obj2.batt == "100%" );
    CHECK( obj2.num_photos == 850 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Rename the Z 8 to z8.
    //
    harness.cmd_socket.to_recv("5 set_camera_id 5678 z8");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 4'100 );
    CHECK( data.command_response.id == 5 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 2 );

    // z7
    obj1 = data.detected_cameras[0];
    CHECK( obj1.connected == true );
    CHECK( obj1.serial == "1234" );
    CHECK( obj1.port == "usb:001,001" );
    CHECK( obj1.desc == "znick" );
    CHECK( obj1.mode == "M" );
    CHECK( obj1.shutter == "1/1000" );
    CHECK( obj1.fstop == "F/8" );
    CHECK( obj1.iso == "64" );
    CHECK( obj1.quality == "NEF (Raw)" );
    CHECK( obj1.batt == "100%" );
    CHECK( obj1.num_photos == 850 );

    // z8
    obj2 = data.detected_cameras[1];
    CHECK( obj2.connected == true );
    CHECK( obj2.serial == "5678" );
    CHECK( obj2.port == "usb:001,002" );
    CHECK( obj2.desc == "z8" );
    CHECK( obj2.mode == "M" );
    CHECK( obj2.shutter == "1/1000" );
    CHECK( obj2.fstop == "F/8" );
    CHECK( obj2.iso == "64" );
    CHECK( obj2.quality == "NEF (Raw)" );
    CHECK( obj2.batt == "100%" );
    CHECK( obj2.num_photos == 850 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Rename znick back to z7.
    //
    harness.cmd_socket.to_recv("6 set_camera_id 1234 z7");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 4'150 );
    CHECK( data.command_response.id == 6 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 2 );

    // z7
    obj1 = data.detected_cameras[0];
    CHECK( obj1.connected == true );
    CHECK( obj1.serial == "1234" );
    CHECK( obj1.port == "usb:001,001" );
    CHECK( obj1.desc == "z7" );
    CHECK( obj1.mode == "M" );
    CHECK( obj1.shutter == "1/1000" );
    CHECK( obj1.fstop == "F/8" );
    CHECK( obj1.iso == "64" );
    CHECK( obj1.quality == "NEF (Raw)" );
    CHECK( obj1.batt == "100%" );
    CHECK( obj1.num_photos == 850 );

    // z8
    obj2 = data.detected_cameras[1];
    CHECK( obj2.connected == true );
    CHECK( obj2.serial == "5678" );
    CHECK( obj2.port == "usb:001,002" );
    CHECK( obj2.desc == "z8" );
    CHECK( obj2.mode == "M" );
    CHECK( obj2.shutter == "1/1000" );
    CHECK( obj2.fstop == "F/8" );
    CHECK( obj2.iso == "64" );
    CHECK( obj2.quality == "NEF (Raw)" );
    CHECK( obj2.batt == "100%" );
    CHECK( obj2.num_photos == 850 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );
}
