#include <camera_control/CameraControl_uto.h>

TEST_CASE("CameraControl", "[CameraControl][auto_detect]")
{
    Harness harness;

    //-------------------------------------------------------------------------
    // Add a camera, initially connected.
    //
    auto cam1 = make_test_camera();
    harness.gp2cpp.add_camera(cam1);
    auto data = harness.dispatch_to_next_message();

    CHECK( data.state == "scan" );
    CHECK( data.time == 0 );
    CHECK( data.command_response.id == 0 );
    CHECK( data.command_response.success == true );
    CHECK( data.detected_cameras.empty() );
    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

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
    CHECK( obj1.num_avail == 0 );
    CHECK( obj1.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Disconnect the camera.
    //
    cam1->connected = false;
    data = harness.dispatch_to_next_message();
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 3'000 );
    CHECK( data.command_response.id == 0 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 1 );

    // z7
    obj1 = data.detected_cameras[0];
    CHECK( obj1.connected == false );
    CHECK( obj1.serial == "1234" );
    CHECK( obj1.port == "usb:001,001" );
    CHECK( obj1.desc == "Nikon Corporation Z 7" );
    CHECK( obj1.mode == "N/A" );
    CHECK( obj1.shutter == "N/A" );
    CHECK( obj1.fstop == "N/A" );
    CHECK( obj1.iso == "N/A" );
    CHECK( obj1.quality == "N/A" );
    CHECK( obj1.batt == "N/A" );
    CHECK( obj1.num_avail == 0 );
    CHECK( obj1.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Plug the camera back in.
    //
    cam1->connected = true;
    cam1->port = "usb:001,002";
    data = harness.dispatch_to_next_message();
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 5'000 );
    CHECK( data.command_response.id == 0 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 1 );

    // z7
    obj1 = data.detected_cameras[0];
    CHECK( obj1.connected == true );
    CHECK( obj1.serial == "1234" );
    CHECK( obj1.port == "usb:001,002" );
    CHECK( obj1.desc == "Nikon Corporation Z 7" );
    CHECK( obj1.mode == "M" );
    CHECK( obj1.shutter == "1/1000" );
    CHECK( obj1.fstop == "F/8" );
    CHECK( obj1.iso == "64" );
    CHECK( obj1.quality == "NEF (Raw)" );
    CHECK( obj1.batt == "100%" );
    CHECK( obj1.num_avail == 0 );
    CHECK( obj1.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Plug another camera in.
    //
    auto cam2 = make_test_camera("Z 8", "usb:001,003", "5678");
    harness.gp2cpp.add_camera(cam2);
    data = harness.dispatch_to_next_message();
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 7'000 );
    CHECK( data.command_response.id == 0 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 2 );

    // z7
    obj1 = data.detected_cameras[0];
    CHECK( obj1.connected == true );
    CHECK( obj1.serial == "1234" );
    CHECK( obj1.port == "usb:001,002" );
    CHECK( obj1.desc == "Nikon Corporation Z 7" );
    CHECK( obj1.mode == "M" );
    CHECK( obj1.shutter == "1/1000" );
    CHECK( obj1.fstop == "F/8" );
    CHECK( obj1.iso == "64" );
    CHECK( obj1.quality == "NEF (Raw)" );
    CHECK( obj1.batt == "100%" );
    CHECK( obj1.num_avail == 0 );
    CHECK( obj1.num_photos == 0 );

    // z8
    auto obj2 = data.detected_cameras[1];
    CHECK( obj2.connected == true );
    CHECK( obj2.serial == "5678" );
    CHECK( obj2.port == "usb:001,003" );
    CHECK( obj2.desc == "Nikon Corporation Z 8" );
    CHECK( obj2.mode == "M" );
    CHECK( obj2.shutter == "1/1000" );
    CHECK( obj2.fstop == "F/8" );
    CHECK( obj2.iso == "64" );
    CHECK( obj2.quality == "NEF (Raw)" );
    CHECK( obj2.batt == "100%" );
    CHECK( obj2.num_avail == 0 );
    CHECK( obj2.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Unplug the z7 again.
    //
    cam1->connected = false;
    data = harness.dispatch_to_next_message();
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 9'000 );
    CHECK( data.command_response.id == 0 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 2 );

    // z7
    obj1 = data.detected_cameras[0];
    CHECK( obj1.connected == false );
    CHECK( obj1.serial == "1234" );
    CHECK( obj1.port == "usb:001,002" );
    CHECK( obj1.desc == "Nikon Corporation Z 7" );
    CHECK( obj1.mode == "N/A" );
    CHECK( obj1.shutter == "N/A" );
    CHECK( obj1.fstop == "N/A" );
    CHECK( obj1.iso == "N/A" );
    CHECK( obj1.quality == "N/A" );
    CHECK( obj1.batt == "N/A" );
    CHECK( obj1.num_avail == 0 );
    CHECK( obj1.num_photos == 0 );

    // z8
    obj2 = data.detected_cameras[1];
    CHECK( obj2.connected == true );
    CHECK( obj2.serial == "5678" );
    CHECK( obj2.port == "usb:001,003" );
    CHECK( obj2.desc == "Nikon Corporation Z 8" );
    CHECK( obj2.mode == "M" );
    CHECK( obj2.shutter == "1/1000" );
    CHECK( obj2.fstop == "F/8" );
    CHECK( obj2.iso == "64" );
    CHECK( obj2.quality == "NEF (Raw)" );
    CHECK( obj2.batt == "100%" );
    CHECK( obj2.num_avail == 0 );
    CHECK( obj2.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Unplug the z8.
    //
    cam2->connected = false;
    data = harness.dispatch_to_next_message();
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 11'000 );
    CHECK( data.command_response.id == 0 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 2 );

    // z7
    obj1 = data.detected_cameras[0];
    CHECK( obj1.connected == false );
    CHECK( obj1.serial == "1234" );
    CHECK( obj1.port == "usb:001,002" );
    CHECK( obj1.desc == "Nikon Corporation Z 7" );
    CHECK( obj1.mode == "N/A" );
    CHECK( obj1.shutter == "N/A" );
    CHECK( obj1.fstop == "N/A" );
    CHECK( obj1.iso == "N/A" );
    CHECK( obj1.quality == "N/A" );
    CHECK( obj1.batt == "N/A" );
    CHECK( obj1.num_avail == 0 );
    CHECK( obj1.num_photos == 0 );

    // z8
    obj2 = data.detected_cameras[1];
    CHECK( obj2.connected == false );
    CHECK( obj2.serial == "5678" );
    CHECK( obj2.port == "usb:001,003" );
    CHECK( obj2.desc == "Nikon Corporation Z 8" );
    CHECK( obj2.mode == "N/A" );
    CHECK( obj2.shutter == "N/A" );
    CHECK( obj2.fstop == "N/A" );
    CHECK( obj2.iso == "N/A" );
    CHECK( obj2.quality == "N/A" );
    CHECK( obj2.batt == "N/A" );
    CHECK( obj2.num_avail == 0 );
    CHECK( obj2.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Fiddle with the dials and plug both back in.
    cam1->connected = true;
    cam1->port = "usb:001,004";
    cam1->fstop = "F/4";
    cam1->iso = "400";
    cam1->shutter = "1/400";
    cam1->quality = "JPEG Basic";
    cam1->num_avail = 3456;

    cam2->connected = true;
    cam2->port = "usb:001,005";
    cam2->fstop = "F/8";
    cam2->iso = "800";
    cam2->shutter = "1/800";
    cam2->quality = "TIFF";
    cam2->num_avail = 411;

    data = harness.dispatch_to_next_message();
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 13'000 );
    CHECK( data.command_response.id == 0 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 2 );

    // z7
    obj1 = data.detected_cameras[0];
    CHECK( obj1.connected == true );
    CHECK( obj1.serial == "1234" );
    CHECK( obj1.port == "usb:001,004" );
    CHECK( obj1.desc == "Nikon Corporation Z 7" );
    CHECK( obj1.mode == "M" );
    CHECK( obj1.shutter == "1/400" );
    CHECK( obj1.fstop == "F/4" );
    CHECK( obj1.iso == "400" );
    CHECK( obj1.quality == "JPEG Basic" );
    CHECK( obj1.batt == "100%" );
//~    CHECK( obj1.num_avail == 3'456 );
    CHECK( obj1.num_photos == 0 );

    // z8
    obj2 = data.detected_cameras[1];
    CHECK( obj2.connected == true );
    CHECK( obj2.serial == "5678" );
    CHECK( obj2.port == "usb:001,005" );
    CHECK( obj2.desc == "Nikon Corporation Z 8" );
    CHECK( obj2.mode == "M" );
    CHECK( obj2.shutter == "1/800" );
    CHECK( obj2.fstop == "F/8" );
    CHECK( obj2.iso == "800" );
    CHECK( obj2.quality == "TIFF" );
    CHECK( obj2.batt == "100%" );
//~    CHECK( obj1.num_avail == 411 );
    CHECK( obj1.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );
}
