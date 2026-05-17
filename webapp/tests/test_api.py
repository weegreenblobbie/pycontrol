import json
import pytest
from webapp import webapp

@pytest.fixture
def client(app):
    return app.test_client()

def test_api_root(client):
    response = client.get("/")
    assert response.status_code == 200

def test_api_dashboard_update(client):
    response = client.get("/api/dashboard_update")
    assert response.status_code == 200
    data = response.get_json()
    assert "gps" in data
    assert "detected_cameras" in data
    assert "camera_control" in data

def test_api_camera_update_description(client):
    # Success case
    payload = {"serial": "12345", "description": "new_alias"}
    response = client.post("/api/camera/update_description", 
                           data=json.dumps(payload),
                           content_type='application/json')
    assert response.status_code == 200
    
    # Missing fields
    payload = {"serial": "12345"}
    response = client.post("/api/camera/update_description", 
                           data=json.dumps(payload),
                           content_type='application/json')
    assert response.status_code == 400

    # Non-JSON
    response = client.post("/api/camera/update_description", 
                           data="not json")
    assert response.status_code == 400

def test_api_event_list(client):
    response = client.get("/api/event_list")
    assert response.status_code == 200
    data = response.get_json()
    assert isinstance(data, list)
    assert "spain-2026.event" in data

def test_api_event_load(client):
    # Success
    payload = {"filename": "spain-2026.event"}
    response = client.post("/api/event_load",
                           data=json.dumps(payload),
                           content_type='application/json')
    assert response.status_code == 200

    # Missing filename
    payload = {}
    response = client.post("/api/event_load",
                           data=json.dumps(payload),
                           content_type='application/json')
    assert response.status_code == 400

    # Non-existent file
    payload = {"filename": "nonexistent.event"}
    response = client.post("/api/event_load",
                           data=json.dumps(payload),
                           content_type='application/json')
    assert response.status_code == 500

def test_api_camera_sequence_list(client):
    response = client.get("/api/camera_sequence_list")
    assert response.status_code == 200
    data = response.get_json()
    assert isinstance(data, list)
    assert "s1.seq" in data

def test_api_camera_sequence_load(client):
    # Success
    payload = {"filename": "s1.seq"}
    response = client.post("/api/camera_sequence_load",
                           data=json.dumps(payload),
                           content_type='application/json')
    assert response.status_code == 200
    
    # Missing filename
    payload = {}
    response = client.post("/api/camera_sequence_load",
                           data=json.dumps(payload),
                           content_type='application/json')
    assert response.status_code == 400

    # Not found
    payload = {"filename": "nonexistent.seq"}
    response = client.post("/api/camera_sequence_load",
                           data=json.dumps(payload),
                           content_type='application/json')
    assert response.status_code == 500

def test_api_run_sim_defaults(client):
    response = client.get("/api/run_sim/defaults")
    assert response.status_code == 200
    data = response.get_json()
    assert "gps_latitude" in data

def test_api_run_sim_lifecycle(client):
    # Load event first
    payload = {"filename": "spain-2026.event"}
    client.post("/api/event_load",
                data=json.dumps(payload),
                content_type='application/json')

    # Start sim (GET with query parameters)
    params = {
        "gps_latitude": 40.0,
        "gps_longitude": -3.0,
        "gps_altitude": 500.0,
        "event_id": "c2",
        "event_time_offset": -10.0
    }
    response = client.get("/api/run_sim", query_string=params)
    assert response.status_code == 200
    
    # Missing parameters
    params = {"gps_latitude": 40.0}
    response = client.get("/api/run_sim", query_string=params)
    assert response.status_code == 400

    # Invalid types
    params = {
        "gps_latitude": "not a float",
        "gps_longitude": -3.0,
        "gps_altitude": 500.0
    }
    response = client.get("/api/run_sim", query_string=params)
    assert response.status_code == 400

    # Stop sim
    response = client.get("/api/run_sim/stop")
    assert response.status_code == 200

def test_api_camera_read_choices(client):
    # Standard prop
    payload = {"serial": "12345", "property": "iso"}
    response = client.post("/api/camera/read_choices",
                           data=json.dumps(payload),
                           content_type='application/json')
    assert response.status_code == 200
    
    # Mapped prop (JS_TO_PROP)
    payload = {"serial": "12345", "property": "shutter"}
    response = client.post("/api/camera/read_choices",
                           data=json.dumps(payload),
                           content_type='application/json')
    assert response.status_code == 200

    # Special case: burstnumber
    payload = {"serial": "12345", "property": "burst"}
    response = client.post("/api/camera/read_choices",
                           data=json.dumps(payload),
                           content_type='application/json')
    assert response.status_code == 200
    data = response.get_json()
    assert data == list(range(1, 11))

    # Missing serial
    payload = {"property": "iso"}
    response = client.post("/api/camera/read_choices",
                           data=json.dumps(payload),
                           content_type='application/json')
    assert response.status_code == 400

def test_api_camera_set_choice(client):
    # Success
    payload = {"serial": "12345", "property": "iso", "value": "800"}
    response = client.post("/api/camera/set_choice",
                           data=json.dumps(payload),
                           content_type='application/json')
    assert response.status_code == 200

    # Missing fields
    payload = {"serial": "12345", "property": "iso"}
    response = client.post("/api/camera/set_choice",
                           data=json.dumps(payload),
                           content_type='application/json')
    assert response.status_code == 400

def test_api_camera_trigger(client):
    # Success
    payload = {"serial": "12345"}
    response = client.post("/api/camera/trigger",
                           data=json.dumps(payload),
                           content_type='application/json')
    assert response.status_code == 200

    # Missing serial
    payload = {}
    response = client.post("/api/camera/trigger",
                           data=json.dumps(payload),
                           content_type='application/json')
    assert response.status_code == 400

def test_api_timelapse_actions(client):
    actions = ["enable", "disable", "update", "start", "stop"]
    for action in actions:
        payload = {"serial": "12345"} if action in ["start", "stop", "update"] else {}
        if action == "update":
            payload.update({
                "interval": 2.0, "min_shutter": "1/1000", "max_shutter": "1s",
                "min_iso": "100", "max_iso": "3200", "min_hist_mask": 0,
                "max_hist_mask": 255, "min_deadband": 5, "max_deadband": 10,
                "target_offset": 0, "target_percent": 0.5
            })
        response = client.post(f"/api/timelapse/{action}",
                               data=json.dumps(payload),
                               content_type='application/json')
        assert response.status_code == 200

    # Unknown action
    response = client.post("/api/timelapse/invalid",
                           data=json.dumps({}),
                           content_type='application/json')
    assert response.status_code == 400

def test_api_temp_file(client):
    # Create a dummy file in /tmp
    import os
    with open("/tmp/test_file.txt", "w") as f:
        f.write("test content")
    
    response = client.get("/tmp/test_file.txt")
    assert response.status_code == 200
    assert response.data == b"test content"
    os.remove("/tmp/test_file.txt")

def test_catch_all(client):
    response = client.get("/invalid_path")
    assert response.status_code == 302
    assert response.location.endswith("/")
