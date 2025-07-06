#include <camera_control/CameraControl_uto.h>

TEST_CASE("CameraControl", "[CameraControl][read_choices][set_choice]")
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
    // Plugin a camera.
    //
    auto cam1 = make_test_camera();
    harness.gp2cpp.add_camera(cam1);
    data = harness.dispatch_to_next_message();
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 2'000 );
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
    // Reading from a non-existing camera should fail.
    //
    harness.cmd_socket.to_recv("1 read_choices 9999 imagequality");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 2'050 );
    CHECK( data.command_response.id == 1 );
    CHECK( data.command_response.success == false );
    CHECK( data.command_response.message == "serial '9999' does not exist" );
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
    CHECK( obj1.num_avail == 0 );
    CHECK( obj1.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Reading a non-existing property should fail.
    //
    harness.cmd_socket.to_recv("2 read_choices 1234 wow-factor");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 2'100 );
    CHECK( data.command_response.id == 2 );
    CHECK( data.command_response.success == false );
    CHECK( data.command_response.message == "property 'wow-factor' does not exist" );
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
    CHECK( obj1.num_avail == 0 );
    CHECK( obj1.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Reading exposure program.
    //
    harness.cmd_socket.to_recv("3 read_choices 1234 expprogram");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 2'150 );
    CHECK( data.command_response.id == 3 );
    CHECK( data.command_response.success == true );
    CHECK( data.command_response.message.empty() );
    CHECK( data.command_response.data == str_vec({"M", "A", "S", "P"}) );
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
    CHECK( obj1.num_avail == 0 );
    CHECK( obj1.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Reading f stop.
    //
    harness.cmd_socket.to_recv("4 read_choices 1234 f-number");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 2'200 );
    CHECK( data.command_response.id == 4 );
    CHECK( data.command_response.success == true );
    CHECK( data.command_response.message.empty() );
    CHECK( data.command_response.data == str_vec({"f/8","f/5.6","f/4"}) );
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
    CHECK( obj1.num_avail == 0 );
    CHECK( obj1.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Reading image quality.
    //
    harness.cmd_socket.to_recv("5 read_choices 1234 imagequality");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 2'250 );
    CHECK( data.command_response.id == 5 );
    CHECK( data.command_response.success == true );
    CHECK( data.command_response.message.empty() );
    CHECK( data.command_response.data == str_vec{"NEF (Raw)","JPEG Basic","JPEG Fine"} );
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
    CHECK( obj1.num_avail == 0 );
    CHECK( obj1.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Reading iso.
    //
    harness.cmd_socket.to_recv("6 read_choices 1234 iso");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 2'300 );
    CHECK( data.command_response.id == 6 );
    CHECK( data.command_response.success == true );
    CHECK( data.command_response.message.empty() );
    CHECK( data.command_response.data == str_vec{"64","100","200","500"} );
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
    CHECK( obj1.num_avail == 0 );
    CHECK( obj1.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Reading shutter speed.
    //
    harness.cmd_socket.to_recv("7 read_choices 1234 shutterspeed2");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 2'350 );
    CHECK( data.command_response.id == 7 );
    CHECK( data.command_response.success == true );
    CHECK( data.command_response.message.empty() );
    CHECK( data.command_response.data == str_vec{"1/1000","1/500","1/250"} );
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
    CHECK( obj1.num_avail == 0 );
    CHECK( obj1.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // set_choice - missing value
    //
    harness.cmd_socket.to_recv("8 set_choice 999 shutterspeed2");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 2'400 );
    CHECK( data.command_response.id == 8 );
    CHECK( data.command_response.success == false );
    CHECK( data.command_response.message == "Failed to parse set_choice command: '8 set_choice 999 shutterspeed2'" );
    CHECK( data.command_response.data.empty());
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
    CHECK( obj1.num_avail == 0 );
    CHECK( obj1.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // set_choice - bad serial
    //
    harness.cmd_socket.to_recv("9 set_choice 999 shutterspeed2 1/500");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 2'450 );
    CHECK( data.command_response.id == 9 );
    CHECK( data.command_response.success == false );
    CHECK( data.command_response.message == "serial '999' does not exist" );
    CHECK( data.command_response.data.empty());
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
    CHECK( obj1.num_avail == 0 );
    CHECK( obj1.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // set_choice - bad property
    //
    harness.cmd_socket.to_recv("10 set_choice 1234 wow-factor 5000");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 2'500 );
    CHECK( data.command_response.id == 10 );
    CHECK( data.command_response.success == false );
    CHECK( data.command_response.message == "property 'wow-factor' does not exist" );
    CHECK( data.command_response.data.empty());
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
    CHECK( obj1.num_avail == 0 );
    CHECK( obj1.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // set_choice - good
    //
    harness.cmd_socket.to_recv("11 set_choice 1234 shutterspeed2 1/500");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 2'550 );
    CHECK( data.command_response.id == 11 );
    CHECK( data.command_response.success == true );
    CHECK( data.command_response.message.empty() );
    CHECK( data.command_response.data.empty());
    REQUIRE( data.detected_cameras.size() == 1 );

    // z7
    obj1 = data.detected_cameras[0];
    CHECK( obj1.connected == true );
    CHECK( obj1.serial == "1234" );
    CHECK( obj1.port == "usb:001,001" );
    CHECK( obj1.desc == "Nikon Corporation Z 7" );
    CHECK( obj1.mode == "M" );
    CHECK( obj1.shutter == "1/500" );
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
    // set_choice - value containing a space.
    //
    harness.cmd_socket.to_recv("12 set_choice 1234 imagequality  jpeg fine*  ");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 2'600 );
    CHECK( data.command_response.id == 12 );
    CHECK( data.command_response.success == true );
    CHECK( data.command_response.message.empty() );
    CHECK( data.command_response.data.empty());
    REQUIRE( data.detected_cameras.size() == 1 );

    // z7
    obj1 = data.detected_cameras[0];
    CHECK( obj1.connected == true );
    CHECK( obj1.serial == "1234" );
    CHECK( obj1.port == "usb:001,001" );
    CHECK( obj1.desc == "Nikon Corporation Z 7" );
    CHECK( obj1.mode == "M" );
    CHECK( obj1.shutter == "1/500" );
    CHECK( obj1.fstop == "F/8" );
    CHECK( obj1.iso == "64" );
    CHECK( obj1.quality == "jpeg fine*" );
    CHECK( obj1.batt == "100%" );
    CHECK( obj1.num_avail == 0 );
    CHECK( obj1.num_photos == 0 );

    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );
}
