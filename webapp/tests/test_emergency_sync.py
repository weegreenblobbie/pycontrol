import pytest
import json
import time
from unittest.mock import patch

def test_emergency_override_endpoint(client):
    """Test the /api/emergency-override endpoint."""
    
    # Define a mock payload
    phone_time = time.time() - 5.0 # Simulate 5 seconds latency (should be cold boot case)
    payload = {
        "latitude": 45.0,
        "longitude": -90.0,
        "altitude": 500.0,
        "timestamp": phone_time
    }
    
    # Send POST request
    with patch('webapp.webapp.os.system') as mock_system:
        response = client.post('/api/emergency-override', 
                               data=json.dumps(payload),
                               content_type='application/json')
        
        assert response.status_code == 200
        data = response.get_json()
        
        # Verify sudo date was called with the exact expected string
        expected_cmd = f"sudo date -s '@{data['synced_to']}'"
        mock_system.assert_called_once_with(expected_cmd)
    
    # Assert response
    assert data['status'] == 'success'
    assert 'synced_to' in data
    assert 'delta_seconds' in data
    
    # Verify GpsReader state (accessing through the app instance)
    # The 'client' fixture in Flask-Pytest usually provides access to app via client.application
    gps_data = client.application.pycontrol_app._gps_reader.read()
    assert gps_data['mode'] == 'Emergency Sync'
    assert gps_data['lat'] == 45.0
    assert gps_data['long'] == -90.0
    assert gps_data['altitude'] == 500.0
    # delta_t is formatted as string or HMS in read(), let's check internal state or just that it's not N/A
    assert gps_data['delta_t'] != 'N/A'

def test_emergency_override_warm_sync(client):
    """Test the /api/emergency-override endpoint with warm sync (< 2s latency)."""
    
    # Define a mock payload with small latency
    phone_time = time.time() - 1.0 
    payload = {
        "latitude": 30.0,
        "longitude": -80.0,
        "timestamp": phone_time
    }
    
    # Send POST request
    with patch('webapp.webapp.os.system') as mock_system:
        response = client.post('/api/emergency-override', 
                               data=json.dumps(payload),
                               content_type='application/json')
        
        assert response.status_code == 200
        data = response.get_json()

        # Verify sudo date was called with the exact expected string
        expected_cmd = f"sudo date -s '@{data['synced_to']}'"
        mock_system.assert_called_once_with(expected_cmd)
    
    # Warm sync formula: target_time = phone_time + (latency / 2)
    # latency = pi_now - phone_time
    # Here pi_now is roughly time.time()
    # So target_time should be roughly phone_time + 0.5
    
    expected_target = phone_time + ((data['delta_seconds']) / 2)
    assert abs(data['synced_to'] - expected_target) < 0.1
