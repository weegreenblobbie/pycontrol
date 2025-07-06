#include <catch2/catch_test_macros.hpp>

#include <camera_control/CameraControl_uto.h>

TEST_CASE("CameraControl", "[CameraControl][load_sequence][reset_sequence]")
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
    // Set a non-existing sequence.
    //
    harness.cmd_socket.to_recv("1 load_sequence /nope/does-not-exist.seq ");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 50 );
    CHECK( data.command_response.id == 1 );
    CHECK( data.command_response.success == false );
    CHECK( data.command_response.message == "failed to parse camera sequence file '/nope/does-not-exist.seq'" );
    CHECK( data.detected_cameras.empty() );
    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );

    //-------------------------------------------------------------------------
    // Command an existing sequence.
    //
    auto seq1 = TempFile(
        "test.seq",
        R"(
            e1 -10.0 z7.trigger 1
            e1  -9.0 z7.trigger 1
        )"
    );

    harness.cmd_socket.to_recv("2 load_sequence " + seq1.path.string());
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 100 );
    CHECK( data.command_response.id == 2 );
    CHECK( data.command_response.success == true );
    CHECK( data.detected_cameras.empty() );
    CHECK( data.events.empty() );
    CHECK( data.sequence == seq1.path.string() );

    REQUIRE( data.sequence_state.size() == 1 );

    auto seq_state = data.sequence_state[0];
    CHECK( seq_state.num_events == 2 );
    CHECK( seq_state.id == "z7" );

    REQUIRE( seq_state.events.size() == 2 );

    auto event1 = seq_state.events[0];
    CHECK( event1.pos == 1 );
    CHECK( event1.event_id == "e1" );
    CHECK( event1.event_time_offset == "-00:00:10.000" );
    CHECK( event1.eta == "N/A" );
    CHECK( event1.channel == "z7.trigger" );
    CHECK( event1.value == "1" );

    auto event2 = seq_state.events[1];
    CHECK( event2.pos == 2 );
    CHECK( event2.event_id == "e1" );
    CHECK( event2.event_time_offset == "-00:00:09.000" );
    CHECK( event2.eta == "N/A" );
    CHECK( event2.channel == "z7.trigger" );
    CHECK( event2.value == "1" );

    //-------------------------------------------------------------------------
    // Command another sequence
    //
    auto seq2 = TempFile(
        "test.seq",
        R"(
            e1 -11.0 z7.trigger 1
            e1 -10.0 z7.trigger 1
            e1  -9.0 z7.trigger 1
        )"
    );

    harness.cmd_socket.to_recv("3 load_sequence " + seq2.path.string());
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 150 );
    CHECK( data.command_response.id == 3 );
    CHECK( data.command_response.success == true );
    CHECK( data.detected_cameras.empty() );
    CHECK( data.events.empty() );
    CHECK( data.sequence == seq2.path.string() );

    REQUIRE(data.sequence_state.size() == 1);

    seq_state = data.sequence_state[0];
    CHECK( seq_state.num_events == 3 );
    CHECK( seq_state.id == "z7" );

    REQUIRE( seq_state.events.size() == 3 );

    event1 = seq_state.events[0];
    CHECK( event1.pos == 1 );
    CHECK( event1.event_id == "e1" );
    CHECK( event1.event_time_offset == "-00:00:11.000" );
    CHECK( event1.eta == "N/A" );
    CHECK( event1.channel == "z7.trigger" );
    CHECK( event1.value == "1" );

    event2 = seq_state.events[1];
    CHECK( event2.pos == 2 );
    CHECK( event2.event_id == "e1" );
    CHECK( event2.event_time_offset == "-00:00:10.000" );
    CHECK( event2.eta == "N/A" );
    CHECK( event2.channel == "z7.trigger" );
    CHECK( event2.value == "1" );

    auto event3 = seq_state.events[2];
    CHECK( event3.pos == 3 );
    CHECK( event3.event_id == "e1" );
    CHECK( event3.event_time_offset == "-00:00:09.000" );
    CHECK( event3.eta == "N/A" );
    CHECK( event3.channel == "z7.trigger" );
    CHECK( event3.value == "1" );

    //-------------------------------------------------------------------------
    // Command the event times and show ETA update.
    //
    harness.cmd_socket.to_recv("4 set_events e1 20000");
    data = harness.dispatch_to_next_message();
    CHECK( data.time == 200 );
    data = harness.dispatch_to_next_message();

    REQUIRE( 1'000 == harness.cc.control_time() );
    auto e1_eta = 20'000 - harness.cc.control_time();
    REQUIRE( e1_eta == 19'000 );

    // Now applying event time offsets below in the sequence should make sense.
    // 19 - 11 -> 8 seconds eta
    // 19 - 10 -> 9 seconds eta
    // 19 -  9 -> 10 seconds eta
    CHECK( data.state == "monitor" );
    CHECK( data.time == 1000 );
    CHECK( data.command_response.id == 4 );
    CHECK( data.command_response.success == true );
    CHECK( data.detected_cameras.empty() );

    REQUIRE( data.events.size() == 1 );
    CHECK( data.events["e1"] == 20'000 );

    CHECK( data.sequence == seq2.path.string() );

    REQUIRE( data.sequence_state.size() == 1 );

    seq_state = data.sequence_state[0];
    CHECK( seq_state.num_events == 3 );
    CHECK( seq_state.id == "z7" );

    REQUIRE( seq_state.events.size() == 3 );

    event1 = seq_state.events[0];
    CHECK( event1.pos == 1 );
    CHECK( event1.event_id == "e1" );
    CHECK( event1.event_time_offset == "-00:00:11.000" );
    CHECK( event1.eta == " 00:00:08.000" );
    CHECK( event1.channel == "z7.trigger" );
    CHECK( event1.value == "1" );

    event2 = seq_state.events[1];
    CHECK( event2.pos == 2 );
    CHECK( event2.event_id == "e1" );
    CHECK( event2.event_time_offset == "-00:00:10.000" );
    CHECK( event2.eta == " 00:00:09.000" );
    CHECK( event2.channel == "z7.trigger" );
    CHECK( event2.value == "1" );

    event3 = seq_state.events[2];
    CHECK( event3.pos == 3 );
    CHECK( event3.event_id == "e1" );
    CHECK( event3.event_time_offset == "-00:00:09.000" );
    CHECK( event3.eta == " 00:00:10.000" );
    CHECK( event3.channel == "z7.trigger" );
    CHECK( event3.value == "1" );

    //-------------------------------------------------------------------------
    // Plugin a camera.
    //
    auto cam1 = make_test_camera();
    harness.gp2cpp.add_camera(cam1);
    data = harness.dispatch_to_next_message();
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "monitor" );
    CHECK( data.time == 3'000 );
    CHECK( data.command_response.id == 4 );
    CHECK( data.command_response.success == true );

    REQUIRE( data.detected_cameras.size() == 1 );

    auto cam_data = data.detected_cameras[0];
    CHECK( cam_data.connected == true );
    CHECK( cam_data.serial == "1234" );
    CHECK( cam_data.port == "usb:001,001" );
    CHECK( cam_data.desc == "Nikon Corporation Z 7" );
    CHECK( cam_data.mode == "M" );
    CHECK( cam_data.shutter == "1/1000" );
    CHECK( cam_data.fstop == "F/8" );
    CHECK( cam_data.iso == "64" );
    CHECK( cam_data.quality == "NEF (Raw)" );
    CHECK( cam_data.batt == "100%" );
    CHECK( cam_data.num_photos == 0 );

    REQUIRE( data.events.size() == 1 );
    CHECK( data.events["e1"] == 20'000 );

    CHECK( data.sequence == seq2.path.string() );

    REQUIRE( data.sequence_state.size() == 1 );

    seq_state = data.sequence_state[0];
    CHECK( seq_state.num_events == 3 );
    CHECK( seq_state.id == "z7" );

    REQUIRE( seq_state.events.size() == 3 );

    event1 = seq_state.events[0];
    CHECK( event1.pos == 1 );
    CHECK( event1.event_id == "e1" );
    CHECK( event1.event_time_offset == "-00:00:11.000" );
    CHECK( event1.eta == " 00:00:06.000" );
    CHECK( event1.channel == "z7.trigger" );
    CHECK( event1.value == "1" );

    event2 = seq_state.events[1];
    CHECK( event2.pos == 2 );
    CHECK( event2.event_id == "e1" );
    CHECK( event2.event_time_offset == "-00:00:10.000" );
    CHECK( event2.eta == " 00:00:07.000" );
    CHECK( event2.channel == "z7.trigger" );
    CHECK( event2.value == "1" );

    event3 = seq_state.events[2];
    CHECK( event3.pos == 3 );
    CHECK( event3.event_id == "e1" );
    CHECK( event3.event_time_offset == "-00:00:09.000" );
    CHECK( event3.eta == " 00:00:08.000" );
    CHECK( event3.channel == "z7.trigger" );
    CHECK( event3.value == "1" );

    //-------------------------------------------------------------------------
    // Rename the camera.
    harness.cmd_socket.to_recv("5 set_camera_id 1234 z7");
    data = harness.dispatch_to_next_message();
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "executing" );
    CHECK( data.time == 4'000 );
    CHECK( data.command_response.id == 5 );
    CHECK( data.command_response.success == true );

    REQUIRE( data.detected_cameras.size() == 1 );

    cam_data = data.detected_cameras[0];
    CHECK( cam_data.connected == true );
    CHECK( cam_data.serial == "1234" );
    CHECK( cam_data.port == "usb:001,001" );
    CHECK( cam_data.desc == "z7" );
    CHECK( cam_data.mode == "M" );
    CHECK( cam_data.shutter == "1/1000" );
    CHECK( cam_data.fstop == "F/8" );
    CHECK( cam_data.iso == "64" );
    CHECK( cam_data.quality == "NEF (Raw)" );
    CHECK( cam_data.batt == "100%" );
    CHECK( cam_data.num_photos == 0 );

    REQUIRE( data.events.size() == 1 );
    CHECK( data.events["e1"] == 20'000 );

    CHECK( data.sequence == seq2.path.string() );
    REQUIRE( data.sequence_state.size() == 1 );

    seq_state = data.sequence_state[0];
    CHECK( seq_state.num_events == 3 );
    CHECK( seq_state.id == "z7" );

    REQUIRE( seq_state.events.size() == 3 );

    event1 = seq_state.events[0];
    CHECK( event1.pos == 1 );
    CHECK( event1.event_id == "e1" );
    CHECK( event1.event_time_offset == "-00:00:11.000" );
    CHECK( event1.eta == " 00:00:05.000" );
    CHECK( event1.channel == "z7.trigger" );
    CHECK( event1.value == "1" );

    event2 = seq_state.events[1];
    CHECK( event2.pos == 2 );
    CHECK( event2.event_id == "e1" );
    CHECK( event2.event_time_offset == "-00:00:10.000" );
    CHECK( event2.eta == " 00:00:06.000" );
    CHECK( event2.channel == "z7.trigger" );
    CHECK( event2.value == "1" );

    event3 = seq_state.events[2];
    CHECK( event3.pos == 3 );
    CHECK( event3.event_id == "e1" );
    CHECK( event3.event_time_offset == "-00:00:09.000" );
    CHECK( event3.eta == " 00:00:07.000" );
    CHECK( event3.channel == "z7.trigger" );
    CHECK( event3.value == "1" );

    //-------------------------------------------------------------------------
    // 5 seconds away from executing the first event in the sequence.
    // Now that the state is 'executing', we stop monitoring camera changes and
    // emit telemetry at 4 Hz.  Dispatch for 5 seconds such that the first
    // event is executed.
    for (int i = 0; i < 5 * 4; ++i)
    {
        data = harness.dispatch_to_next_message();
    }

    CHECK( data.state == "executing" );
    CHECK( data.time == 9'000 );
    CHECK( data.command_response.id == 5 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 1 );

    cam_data = data.detected_cameras[0];
    CHECK( cam_data.connected == true );
    CHECK( cam_data.serial == "1234" );
    CHECK( cam_data.port == "usb:001,001" );
    CHECK( cam_data.desc == "z7" );
    CHECK( cam_data.mode == "M" );
    CHECK( cam_data.shutter == "1/1000" );
    CHECK( cam_data.fstop == "F/8" );
    CHECK( cam_data.iso == "64" );
    CHECK( cam_data.quality == "NEF (Raw)" );
    CHECK( cam_data.batt == "100%" );
    CHECK( cam_data.num_photos == 0 );

    REQUIRE( data.events.size() == 1 );
    CHECK( data.events["e1"] == 20'000 );
    CHECK( data.sequence == seq2.path.string() );
    REQUIRE( data.sequence_state.size() == 1 );

    seq_state = data.sequence_state[0];
    CHECK( seq_state.num_events == 3 );
    CHECK( seq_state.id == "z7" );
    REQUIRE( seq_state.events.size() == 2 );

    event2 = seq_state.events[0];
    CHECK( event2.pos == 2 );
    CHECK( event2.event_id == "e1" );
    CHECK( event2.event_time_offset == "-00:00:10.000" );
    CHECK( event2.eta == " 00:00:01.000" );
    CHECK( event2.channel == "z7.trigger" );
    CHECK( event2.value == "1" );

    event3 = seq_state.events[1];
    CHECK( event3.pos == 3 );
    CHECK( event3.event_id == "e1" );
    CHECK( event3.event_time_offset == "-00:00:09.000" );
    CHECK( event3.eta == " 00:00:02.000" );
    CHECK( event3.channel == "z7.trigger" );
    CHECK( event3.value == "1" );

    REQUIRE( cam1->trigger_count == 1 );

    //-------------------------------------------------------------------------
    // Command the event times and show ETA update.
    //
    harness.cmd_socket.to_recv("6 set_events e1 30000");
    data = harness.dispatch_to(10'000);

    REQUIRE( 10'000 == harness.cc.control_time() );
    e1_eta = 30'000 - harness.cc.control_time();
    REQUIRE( e1_eta == 20'000 );
    // 20 - 10 => 10 seconds
    // 20 -  9 => 11 seconds

    CHECK( data.state == "executing" );
    CHECK( data.time == 10'000 );
    CHECK( data.command_response.id == 6 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 1 );

    cam_data = data.detected_cameras[0];
    CHECK( cam_data.connected == true );
    CHECK( cam_data.serial == "1234" );
    CHECK( cam_data.port == "usb:001,001" );
    CHECK( cam_data.desc == "z7" );
    CHECK( cam_data.mode == "M" );
    CHECK( cam_data.shutter == "1/1000" );
    CHECK( cam_data.fstop == "F/8" );
    CHECK( cam_data.iso == "64" );
    CHECK( cam_data.quality == "NEF (Raw)" );
    CHECK( cam_data.batt == "100%" );
    CHECK( cam_data.num_photos == 0 );

    REQUIRE( data.events.size() == 1 );
    CHECK( data.events["e1"] == 30000 );
    CHECK( data.sequence == seq2.path.string() );
    REQUIRE( data.sequence_state.size() == 1 );

    seq_state = data.sequence_state[0];
    CHECK( seq_state.num_events == 3 );
    CHECK( seq_state.id == "z7" );
    REQUIRE( seq_state.events.size() == 2 );

    event2 = seq_state.events[0];
    CHECK( event2.pos == 2 );
    CHECK( event2.event_id == "e1" );
    CHECK( event2.event_time_offset == "-00:00:10.000" );
    CHECK( event2.eta == " 00:00:10.000" );
    CHECK( event2.channel == "z7.trigger" );
    CHECK( event2.value == "1" );

    event3 = seq_state.events[1];
    CHECK( event3.pos == 3 );
    CHECK( event3.event_id == "e1" );
    CHECK( event3.event_time_offset == "-00:00:09.000" );
    CHECK( event3.eta == " 00:00:11.000" );
    CHECK( event3.channel == "z7.trigger" );
    CHECK( event3.value == "1" );

    //-------------------------------------------------------------------------
    // Command the sequence to reset.  We should get 3 events back.
    harness.cmd_socket.to_recv("7 reset_sequence");
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "executing" );
    CHECK( data.time == 10'050 );
    CHECK( data.command_response.id == 7 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 1 );

    cam_data = data.detected_cameras[0];
    CHECK( cam_data.connected == true );
    CHECK( cam_data.serial == "1234" );
    CHECK( cam_data.port == "usb:001,001" );
    CHECK( cam_data.desc == "z7" );
    CHECK( cam_data.mode == "M" );
    CHECK( cam_data.shutter == "1/1000" );
    CHECK( cam_data.fstop == "F/8" );
    CHECK( cam_data.iso == "64" );
    CHECK( cam_data.quality == "NEF (Raw)" );
    CHECK( cam_data.batt == "100%" );
    CHECK( cam_data.num_photos == 0 );

    REQUIRE( data.events.size() == 1 );
    CHECK( data.events["e1"] == 30000 );
    CHECK( data.sequence == seq2.path.string() );

    REQUIRE( data.sequence_state.size() == 1 );
    seq_state = data.sequence_state[0];
    CHECK( seq_state.num_events == 3 );

    event1 = seq_state.events[0];
    CHECK( event1.pos == 1 );
    CHECK( event1.event_id == "e1" );
    CHECK( event1.event_time_offset == "-00:00:11.000" );
    CHECK( event1.eta == " 00:00:08.950" );
    CHECK( event1.channel == "z7.trigger" );
    CHECK( event1.value == "1" );

    event2 = seq_state.events[1];
    CHECK( event2.pos == 2 );
    CHECK( event2.event_id == "e1" );
    CHECK( event2.event_time_offset == "-00:00:10.000" );
    CHECK( event2.eta == " 00:00:09.950" );
    CHECK( event2.channel == "z7.trigger" );
    CHECK( event2.value == "1" );

    event3 = seq_state.events[2];
    CHECK( event3.pos == 3 );
    CHECK( event3.event_id == "e1" );
    CHECK( event3.event_time_offset == "-00:00:09.000" );
    CHECK( event3.eta == " 00:00:10.950" );
    CHECK( event3.channel == "z7.trigger" );
    CHECK( event3.value == "1" );

    //-------------------------------------------------------------------------
    // Update event time to align to a whole number of seconds.
    harness.cmd_socket.to_recv("8 set_events e1 30250");
    data = harness.dispatch_to_next_message();
    data = harness.dispatch_to_next_message();

    CHECK( data.state == "executing" );
    CHECK( data.time == 10'250 );
    CHECK( data.command_response.id == 8 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 1 );

    cam_data = data.detected_cameras[0];
    CHECK( cam_data.connected == true );
    CHECK( cam_data.serial == "1234" );
    CHECK( cam_data.port == "usb:001,001" );
    CHECK( cam_data.desc == "z7" );
    CHECK( cam_data.mode == "M" );
    CHECK( cam_data.shutter == "1/1000" );
    CHECK( cam_data.fstop == "F/8" );
    CHECK( cam_data.iso == "64" );
    CHECK( cam_data.quality == "NEF (Raw)" );
    CHECK( cam_data.batt == "100%" );
    CHECK( cam_data.num_photos == 0 );

    REQUIRE( data.events.size() == 1 );
    CHECK( data.events["e1"] == 30250 );
    CHECK( data.sequence == seq2.path.string() );

    REQUIRE( data.sequence_state.size() == 1 );
    seq_state = data.sequence_state[0];
    REQUIRE( seq_state.events.size() == 3 );

    event1 = seq_state.events[0];
    CHECK( event1.pos == 1 );
    CHECK( event1.event_id == "e1" );
    CHECK( event1.event_time_offset == "-00:00:11.000" );
    CHECK( event1.eta == " 00:00:09.000" );
    CHECK( event1.channel == "z7.trigger" );
    CHECK( event1.value == "1" );

    event2 = seq_state.events[1];
    CHECK( event2.pos == 2 );
    CHECK( event2.event_id == "e1" );
    CHECK( event2.event_time_offset == "-00:00:10.000" );
    CHECK( event2.eta == " 00:00:10.000" );
    CHECK( event2.channel == "z7.trigger" );
    CHECK( event2.value == "1" );

    event3 = seq_state.events[2];
    CHECK( event3.pos == 3 );
    CHECK( event3.event_id == "e1" );
    CHECK( event3.event_time_offset == "-00:00:09.000" );
    CHECK( event3.eta == " 00:00:11.000" );
    CHECK( event3.channel == "z7.trigger" );
    CHECK( event3.value == "1" );

    //-------------------------------------------------------------------------
    // Dispatch until the first trigger event executes.
    for (int i = 0; i < 9 * 4; ++i)
    {
        data = harness.dispatch_to_next_message();
    }

    CHECK( data.state == "executing" );
    CHECK( data.time == 19'250 );
    CHECK( data.command_response.id == 8 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 1 );

    cam_data = data.detected_cameras[0];
    CHECK( cam_data.connected == true );
    CHECK( cam_data.serial == "1234" );
    CHECK( cam_data.port == "usb:001,001" );
    CHECK( cam_data.desc == "z7" );
    CHECK( cam_data.mode == "M" );
    CHECK( cam_data.shutter == "1/1000" );
    CHECK( cam_data.fstop == "F/8" );
    CHECK( cam_data.iso == "64" );
    CHECK( cam_data.quality == "NEF (Raw)" );
    CHECK( cam_data.batt == "100%" );
    CHECK( cam_data.num_photos == 0 );

    REQUIRE( data.events.size() == 1 );
    CHECK( data.events["e1"] == 30250 );
    CHECK( data.sequence == seq2.path.string() );
    REQUIRE( data.sequence_state.size() == 1 );

    seq_state = data.sequence_state[0];
    CHECK( seq_state.num_events == 3 );
    CHECK( seq_state.id == "z7" );
    REQUIRE( seq_state.events.size() >= 2 );

    event1 = seq_state.events[0];
    CHECK( event1.pos == 2 );
    CHECK( event1.event_id == "e1" );
    CHECK( event1.event_time_offset == "-00:00:10.000" );
    CHECK( event1.eta == " 00:00:01.000" );
    CHECK( event1.channel == "z7.trigger" );
    CHECK( event1.value == "1" );

    event2 = seq_state.events[1];
    CHECK( event2.pos == 3 );
    CHECK( event2.event_id == "e1" );
    CHECK( event2.event_time_offset == "-00:00:09.000" );
    CHECK( event2.eta == " 00:00:02.000" );
    CHECK( event2.channel == "z7.trigger" );
    CHECK( event2.value == "1" );
    REQUIRE( cam1->trigger_count == 2 );

    //-------------------------------------------------------------------------
    // Dispatch until the next trigger event executes.
    for (int i = 0; i < 4; ++i)
    {
        data = harness.dispatch_to_next_message();
    }

    CHECK( data.state == "executing" );
    CHECK( data.time == 20'250 );
    CHECK( data.command_response.id == 8 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 1 );

    cam_data = data.detected_cameras[0];
    CHECK( cam_data.connected == true );
    CHECK( cam_data.serial == "1234" );
    CHECK( cam_data.port == "usb:001,001" );
    CHECK( cam_data.desc == "z7" );
    CHECK( cam_data.mode == "M" );
    CHECK( cam_data.shutter == "1/1000" );
    CHECK( cam_data.fstop == "F/8" );
    CHECK( cam_data.iso == "64" );
    CHECK( cam_data.quality == "NEF (Raw)" );
    CHECK( cam_data.batt == "100%" );
    CHECK( cam_data.num_photos == 0 );

    REQUIRE( data.events.size() == 1 );
    CHECK( data.events["e1"] == 30250 );
    CHECK( data.sequence == seq2.path.string() );

    REQUIRE( data.sequence_state.size() == 1 );

    seq_state = data.sequence_state[0];
    CHECK( seq_state.num_events == 3 );
    CHECK( seq_state.id == "z7" );
    REQUIRE( seq_state.events.size() == 1 );

    event1 = seq_state.events[0];
    CHECK( event1.pos == 3 );
    CHECK( event1.event_id == "e1" );
    CHECK( event1.event_time_offset == "-00:00:09.000" );
    CHECK( event1.eta == " 00:00:01.000" );
    CHECK( event1.channel == "z7.trigger" );
    CHECK( event1.value == "1" );
    REQUIRE( cam1->trigger_count == 3 );

    //-------------------------------------------------------------------------
    // Dispatch until the last trigger event executes.
    // With no upcoming events in the sequence, state transition to
    // execute_ready.
    for (int i = 0; i < 4; ++i)
    {
        data = harness.dispatch_to_next_message();
    }

    CHECK( data.state == "execute_ready" );
    CHECK( data.time == 21'250 );
    CHECK( data.command_response.id == 8 );
    CHECK( data.command_response.success == true );
    REQUIRE( data.detected_cameras.size() == 1 );

    cam_data = data.detected_cameras[0];
    CHECK( cam_data.connected == true );
    CHECK( cam_data.serial == "1234" );
    CHECK( cam_data.port == "usb:001,001" );
    CHECK( cam_data.desc == "z7" );
    CHECK( cam_data.mode == "M" );
    CHECK( cam_data.shutter == "1/1000" );
    CHECK( cam_data.fstop == "F/8" );
    CHECK( cam_data.iso == "64" );
    CHECK( cam_data.quality == "NEF (Raw)" );
    CHECK( cam_data.batt == "100%" );
    CHECK( cam_data.num_photos == 0 );

    REQUIRE( data.events.size() == 1 );
    CHECK( data.events["e1"] == 30250 );
    CHECK( data.sequence == seq2.path.string() );

    REQUIRE( data.sequence_state.size() == 1 );
    seq_state = data.sequence_state[0];

    CHECK( seq_state.num_events == 3 );
    CHECK( seq_state.id == "z7" );
    CHECK( seq_state.events.empty() );

    REQUIRE( cam1->trigger_count == 4 );
}
