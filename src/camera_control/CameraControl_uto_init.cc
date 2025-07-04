#include <camera_control/CameraControl_uto.h>

TEST_CASE("CameraControl", "[CameraControl][init]")
{
    Harness harness;
    CHECK(harness.tlm_socket.from_send().empty());
    CHECK(harness.dispatch() == result::success);
    CHECK(harness.tlm_socket.from_send().size() == 1);

    auto data = parse_telem(harness.tlm_socket.from_send()[0]);
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
    CHECK( data.detected_cameras.empty() );
    CHECK( data.events.empty() );
    CHECK( data.sequence.empty() );
    CHECK( data.sequence_state.empty() );
}
