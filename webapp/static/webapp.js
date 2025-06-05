// Library stuff.
function sprintf() {
    // https://stackoverflow.com/a/4795914/562106
    var args = arguments,
        string = args[0],
        i = 1;
    return string.replace(/%((%)|s|d)/g, function (m) {
        // m is the matched format, e.g. %s, %d
        var val = null;
        if (m[2]) {
            val = m[2];
        } else {
            val = args[i];
            // A switch statement so that the formatter can be extended. Default is %s
            switch (m) {
                case '%d':
                    val = parseFloat(val);
                    if (isNaN(val)) {
                        val = 0;
                    }
                    break;
            }
            i++;
        }
        return val;
    });
}

// === Global GPS Variables ===
let gps_connection = document.getElementById("gps-connection");
let gps_mode = document.getElementById("gps-mode");
let gps_mode_time = document.getElementById("gps-mode-time");
let gps_sats = document.getElementById("gps-sats");
let gps_time = document.getElementById("gps-time");
let gps_latitude = document.getElementById("gps-latitude");
let gps_longitude = document.getElementById("gps-longitude");
let gps_altitude = document.getElementById("gps-altitude");

// --- Helper function for robust fetch and error handling ---
async function fetchData(url, options = {}) {
    try {
        const response = await fetch(url, options);

        if (!response.ok) {
            let error_message = `HTTP error! Status: ${response.status}`;
            try {
                const error_data = await response.json();
                if (error_data && error_data.message) {
                    error_message = error_data.message;
                }
            } catch (json_error) {
                console.warn(`Could not parse error response as JSON for ${url}:`, json_error);
                try { // Try to get text response if JSON parsing fails
                    const text_response = await response.text(); // This consumes the body
                    if (text_response && text_response.length > 0) {
                        error_message += ` - Response: ${text_response.substring(0, 100)}...`;
                    }
                } catch (text_error) {
                    // If reading text also fails, log that error too
                    console.warn(`Could not read error response as text for ${url}:`, text_error);
                }
            }
            throw new Error(error_message);
        }
        // Check for empty response body before trying to parse as JSON
        const content_type = response.headers.get("content-type");
        if (content_type && content_type.indexOf("application/json") !== -1) {
            const text = await response.text(); // Get text first to check if empty
            return text ? JSON.parse(text) : {}; // Parse if not empty, else return empty object
        }
        return {}; // Return empty object if not JSON or no content
    } catch (error) {
        console.error(`Fetch operation failed for ${url}:`, error);
        alert(`Error: ${error.message}`);
        throw error;
    }
}


function fetch_gps() {
    fetchData('/api/gps')
        .then(data => {
            if (!data.connected) {
                gps_connection.src = "/static/gps-disconnected.svg";
                gps_mode.textContent = data.mode || "N/A"; // Add default for safety
                gps_mode_time.textContent = data.mode_time || "N/A";

                gps_sats.textContent = "N/A";
                gps_time.textContent = "N/A";
                gps_latitude.textContent = "N/A";
                gps_longitude.textContent = "N/A";
                gps_altitude.textContent = "N/A";
                return;
            }

            if (data.mode == "2D FIX") {
                gps_connection.src = "/static/gps-degraded.svg";
            } else if (data.mode == "3D FIX") {
                gps_connection.src = "/static/gps-connected.svg";
            } else {
                gps_connection.src = "/static/gps-disconnected.svg"; // Default if mode is unexpected
            }

            gps_mode.textContent = data.mode;
            gps_mode_time.textContent = data.mode_time;
            gps_sats.textContent = sprintf("%d/%d", data.sats_used, data.sats_seen);
            gps_time.textContent = data.time;
            gps_latitude.textContent = data.lat !== undefined ? data.lat.toFixed(4) : "N/A";
            gps_longitude.textContent = data.long !== undefined ? data.long.toFixed(4) : "N/A";
            gps_altitude.textContent = data.altitude !== undefined ? data.altitude.toFixed(1) : "N/A";
        })
        .catch(error => {
             console.error('Error processing GPS data after fetch:', error);
             // Optionally reset GPS fields to N/A if critical fields are missing
        });
}

// === Global Cameras Variables ===
const cameras_table_body = document.getElementById("cameras").tBodies[0]; // Renamed for clarity

function fetch_cameras() {
    fetchData('/api/cameras')
        .then(data => {
            cameras_table_body.innerHTML = ""; // Erase existing rows

            if (!data || data.num_cameras == 0 || !data.detected) { // Added check for data and data.detected
                const row = document.createElement('tr');
                const cell = document.createElement('td');
                cell.colSpan = 11; // Number of columns in camera table
                cell.textContent = "No cameras detected or data unavailable.";
                cell.style.textAlign = "center";
                row.appendChild(cell);
                cameras_table_body.appendChild(row);
                return;
            }

            for (let info of data.detected) {
                const row = document.createElement('tr');
                const cell_connected = document.createElement('td');
                const cell_serial = document.createElement('td');
                const cell_desc = document.createElement('td');
                const cell_battery = document.createElement('td');
                const cell_port = document.createElement('td');
                const cell_available_shots = document.createElement('td');
                const cell_quality = document.createElement('td');
                const cell_mode = document.createElement('td');
                const cell_iso = document.createElement('td');
                const cell_fstop = document.createElement('td');
                const cell_shutter = document.createElement('td');

                cell_serial.textContent = info.serial || "N/A";
                cell_desc.textContent = info.desc || "N/A";
                cell_battery.textContent = "N/A";
                cell_port.textContent = "N/A";
                cell_available_shots.textContent = "N/A";
                cell_quality.textContent = "N/A";
                cell_mode.textContent = "N/A";
                cell_iso.textContent = "N/A";
                cell_fstop.textContent = "N/A";
                cell_shutter.textContent = "N/A";

                cell_desc.classList.add('editable_td');
                cell_desc.addEventListener('click', handle_editable_td_click);
                cell_desc.dataset.serial = info.serial;

                const svg_connected = document.createElement('img');
                svg_connected.width = "70";
                svg_connected.height = "50";

                if (info.connected == "0" || !info.connected) { // Treat undefined as not connected
                    svg_connected.src = "/static/cam-disconnected.svg";
                    svg_connected.alt = "Camera Disconnected";
                } else {
                    svg_connected.src = "/static/cam-connected.svg";
                    svg_connected.alt = "Camera Connected";
                    cell_battery.textContent = info.batt || "N/A";
                    cell_port.textContent = info.port || "N/A";
                    cell_available_shots.textContent = info.num_photos || "N/A";
                    cell_quality.textContent = info.quality || "N/A";
                    cell_mode.textContent = info.mode || "N/A";
                    cell_iso.textContent = info.iso || "N/A";
                    cell_fstop.textContent = info.fstop || "N/A";
                    cell_shutter.textContent = info.shutter || "N/A";
                }

                cell_connected.appendChild(svg_connected);

                row.appendChild(cell_connected);
                row.appendChild(cell_serial);
                row.appendChild(cell_desc);
                row.appendChild(cell_battery);
                row.appendChild(cell_port);
                row.appendChild(cell_available_shots);
                row.appendChild(cell_quality);
                row.appendChild(cell_mode);
                row.appendChild(cell_iso);
                row.appendChild(cell_fstop);
                row.appendChild(cell_shutter);

                cameras_table_body.appendChild(row);
            }
        })
        .catch(error => {
            console.error('Error processing camera data after fetch:', error);
            cameras_table_body.innerHTML = ""; // Clear potentially partial data
            const row = document.createElement('tr');
            const cell = document.createElement('td');
            cell.colSpan = 11;
            cell.textContent = "Error loading camera data.";
            cell.style.textAlign = "center";
            cell.style.color = "red";
            row.appendChild(cell);
            cameras_table_body.appendChild(row);
        });
}

/*
 * Camera Renaming Modal.
 */
const rename_modal = document.getElementById('rename_modal');
const rename_input = document.getElementById('rename_input');
const save_rename_button = document.getElementById('save_rename');
const cancel_rename_button = document.getElementById('cancel_rename');
let current_editable_td = null;

function handle_editable_td_click() {
    current_editable_td = this;
    rename_input.value = current_editable_td.textContent.trim();
    rename_modal.style.display = 'flex';
    rename_input.focus();
}

function handle_rename_modal_close_button_click() {
    rename_modal.style.display = 'none';
}

function handle_cancel_rename_click() {
    rename_modal.style.display = 'none';
}

async function send_camera_description_to_backend(serial, description) {
    try {
        const data = await fetchData('/api/camera/update_description', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                serial: serial,
                description: description
            })
        });
        // Assuming fetchData now returns empty object for non-JSON or empty success responses
        // And throws error for non-ok responses.
        console.log('Camera description updated successfully (or backend acknowledged). Backend response:', data);

    } catch (error) {
        // Error already handled and alerted by fetchData
        console.error('Error sending data to backend (caught by caller in send_camera_description_to_backend):', error);
        // No need to re-alert, fetchData does it.
    }
}

function handle_save_rename_click() {
    if (current_editable_td) {
        const new_description = rename_input.value.trim();
        current_editable_td.textContent = new_description;
        send_camera_description_to_backend(
            current_editable_td.dataset.serial,
            new_description
        );
    }
    rename_modal.style.display = 'none';
}


function handle_rename_input_keypress(event) {
    if (event.key === 'Enter') {
        save_rename_button.click();
    }
}

/*
 * Event table handling.
 */
const event_table_body = document.querySelector('#event_table tbody');
const event_selection_modal = document.getElementById('event_selection_modal');
const file_list_element = document.getElementById('file_list');
let event_file_name_pre_element = null;
const event_row_map = new Map();

function get_or_create_pre_element(cell, class_name) {
    let pre_element = cell.querySelector(`pre.${class_name}`);
    if (!pre_element) {
        pre_element = document.createElement('pre');
        pre_element.className = class_name;
        cell.innerHTML = '';
        cell.appendChild(pre_element);
    }
    return pre_element;
}

function update_event_table_row(event_id, event_time, eta = 'N/A') {
    let row = event_row_map.get(event_id);

    if (!row) {
        row = event_table_body.insertRow(); // Appends to the end by default if no index
        row.id = `event_row_${event_id.replace(/\s+/g, '_').replace(/[^a-zA-Z0-9_]/g, '')}`;
        event_row_map.set(event_id, row);

        row.insertCell(0);
        row.insertCell(1);
        row.insertCell(2);
    }

    row.cells[0].textContent = event_id;
    const time_pre = get_or_create_pre_element(row.cells[1], 'event-time-pre');
    const eta_pre = get_or_create_pre_element(row.cells[2], 'event-eta-pre');
    time_pre.textContent = event_time;
    eta_pre.textContent = eta;
}

function clear_event_data_rows() {
    // Start from 1 to keep the "Event File:" selection row if it's always first.
    // Or, if "Event File:" row is managed outside data rows, iterate carefully.
    // Assuming all data rows are after the static ones.
    // This clears ALL rows from the map and visually from the table body except those excluded.
    const rows_to_delete = [];
    for (let i = 0; i < event_table_body.rows.length; i++) {
        if (event_table_body.rows[i].id !== 'event_file_selection_row') { // Keep the selection row
            rows_to_delete.push(event_table_body.rows[i]);
        }
    }
    rows_to_delete.forEach(row => row.remove());
    event_row_map.clear();
}


async function populate_static_event_details(filename) {
    clear_event_data_rows();

    const loading_row_id = 'loading_data_row';
    let loading_row = document.getElementById(loading_row_id);
    if (!loading_row) { // Only add if not already there (e.g. from previous failed load)
        loading_row = event_table_body.insertRow(); // Appends to end
        loading_row.id = loading_row_id;
        const loading_cell = loading_row.insertCell(0);
        loading_cell.colSpan = 3;
        loading_cell.textContent = `Loading details for ${filename}...`;
    }


    try {
        const event_data = await fetchData(`/api/event_load/${filename}`);
        const existing_loading_row = document.getElementById(loading_row_id);
        if (existing_loading_row) {
            existing_loading_row.remove();
        }

        update_event_table_row('Type', event_data.type || 'N/A', '');

        if (event_data.events && Array.isArray(event_data.events)) {
            event_data.events.forEach(event_id => {
                update_event_table_row(event_id, 'N/A', 'Loading...');
            });
        } else {
            console.warn("Event data missing 'events' list or malformed:", event_data);
            update_event_table_row('Events', 'No events defined or format error', '');
        }
        update_dynamic_events(); // Trigger immediate update
    } catch (error) {
        console.error(`Error fetching static event details for ${filename}:`, error);
        const existing_loading_row = document.getElementById(loading_row_id);
        if (existing_loading_row) {
            existing_loading_row.remove();
        }
        update_event_table_row('Error', `Failed to load ${filename}.`, error.message.substring(0, 100)); // Show snippet
    }
}

async function fetch_files_for_modal() {
    file_list_element.innerHTML = '<li>Loading files...</li>';
    try {
        const files = await fetchData("/api/event_list");
        file_list_element.innerHTML = '';

        if (!files || files.length === 0) {
            file_list_element.innerHTML = '<li>No files found.</li>';
            return;
        }

        files.forEach(filename => {
            const li = document.createElement('li');
            li.textContent = filename;
            li.addEventListener('click', () => {
                if (event_file_name_pre_element) {
                    event_file_name_pre_element.textContent = filename;
                }
                event_selection_modal.style.display = 'none';
                populate_static_event_details(filename);
            });
            file_list_element.appendChild(li);
        });
    } catch (error) {
        console.error("Error fetching files for event modal:", error);
        file_list_element.innerHTML = `<li>Error loading files: ${error.message}</li>`;
    }
}

async function update_dynamic_events() {
    // Only proceed if there are events loaded (i.e., event_row_map is not empty, or a file is selected)
    if (!event_file_name_pre_element || event_file_name_pre_element.textContent === 'Click to Select File') {
        return; // No event file loaded, so no dynamic events to fetch
    }

    try {
        const dynamic_events_map = await fetchData('/api/events');
        if (typeof dynamic_events_map === 'object' && dynamic_events_map !== null) {
            for (const event_id in dynamic_events_map) {
                if (dynamic_events_map.hasOwnProperty(event_id)) {
                    const event_info = dynamic_events_map[event_id];
                    if (Array.isArray(event_info) && event_info.length === 2) {
                        const [event_time, eta] = event_info;
                        update_event_table_row(event_id, event_time, eta);
                    } else {
                        console.warn(`Malformed event info for ${event_id} from /api/events/. Expected [time, eta]. Received:`, event_info);
                    }
                }
            }
        } else {
            console.warn("Unexpected data format from /api/events/. Expected an object (map). Received:", dynamic_events_map);
        }
    } catch (error) {
        console.error('Error fetching dynamic events:', error);
    }
}

/*
 * Run Simulation Functionality.
 */
const run_sim_button = document.getElementById('run_sim_button');
const run_sim_modal = document.getElementById('run_sim_modal');
const run_sim_modal_close_button = document.getElementById('run_sim_modal_close_button');
const sim_latitude_input = document.getElementById('sim_latitude_input');
const sim_longitude_input = document.getElementById('sim_longitude_input');
const sim_altitude_input = document.getElementById('sim_altitude_input');
const sim_event_id_input = document.getElementById('sim_event_id_input');
const sim_time_offset_input = document.getElementById('sim_time_offset_input');
const sim_okay_button = document.getElementById('sim_okay_button');
const sim_cancel_button = document.getElementById('sim_cancel_button');
const calculating_modal = document.getElementById('calculating_modal');
let is_sim_running = false;

function update_run_sim_button_state() {
    if (is_sim_running) {
        run_sim_button.textContent = "Stop Sim";
        run_sim_button.classList.add('running');
        run_sim_button.classList.remove('stopped');
    } else {
        run_sim_button.textContent = "Run Sim";
        run_sim_button.classList.add('stopped');
        run_sim_button.classList.remove('running');
    }
}

async function handle_run_sim_button_click() {
    if (is_sim_running) {
        calculating_modal.style.display = 'flex';
        try {
            const result = await fetchData('/api/run_sim/stop');
            // Assuming result.status or similar indicates success from backend for 200 OK
            // If fetchData throws for non-ok, this part is only for successful responses
            // or where backend sends a JSON body with a status field.
            // For now, if fetchData doesn't throw, we assume it's a success.
            is_sim_running = false;
            update_run_sim_button_state();
            console.log('Simulation stop signal sent. Backend response:', result);
        } catch (error) {
            console.error('Error stopping simulation:', error);
            // Alert already handled by fetchData
        } finally {
            calculating_modal.style.display = 'none';
        }
    } else {
        try {
            const defaults = await fetchData('/api/run_sim/defaults');
            sim_latitude_input.value = defaults.gps_latitude || '';
            sim_longitude_input.value = defaults.gps_longitude || '';
            sim_altitude_input.value = defaults.gps_altitude || '';
            sim_event_id_input.value = defaults.event_id || '';
            sim_time_offset_input.value = defaults.event_time_offset !== undefined ? defaults.event_time_offset : '0';
            run_sim_modal.style.display = 'flex';
            sim_latitude_input.focus();
        } catch (error) {
            console.error('Error fetching simulation defaults:', error);
        }
    }
}

async function handle_sim_okay_click() {
    const params = {
        gps_latitude: parseFloat(sim_latitude_input.value),
        gps_longitude: parseFloat(sim_longitude_input.value),
        gps_altitude: parseFloat(sim_altitude_input.value),
        event_id: sim_event_id_input.value,
        event_time_offset: parseFloat(sim_time_offset_input.value)
    };

    if (isNaN(params.gps_latitude) || isNaN(params.gps_longitude) || isNaN(params.gps_altitude) || isNaN(params.event_time_offset) || !params.event_id) {
        alert("Please enter valid numbers for GPS coordinates/offset and a valid Event ID.");
        return;
    }

    run_sim_modal.style.display = 'none';
    calculating_modal.style.display = 'flex';

    try {
        const query_params = new URLSearchParams(params).toString();
        const result = await fetchData(`/api/run_sim?${query_params}`);
        // Same assumption as stop sim: if fetchData doesn't throw, it's a success from our perspective
        is_sim_running = true;
        update_run_sim_button_state();
        console.log('Simulation start signal sent with params. Backend response:', params, result);

    } catch (error) {
        console.error('Error starting simulation:', error);
        is_sim_running = false; // Ensure state is correct on failure
        update_run_sim_button_state();
    } finally {
        calculating_modal.style.display = 'none';
    }
}

function handle_sim_cancel_click() {
    run_sim_modal.style.display = 'none';
}

function handle_sim_input_keypress(event) {
    if (event.key === 'Enter') {
        sim_okay_button.click();
    }
}

// === NEW: Camera Sequence Functionality ===
const load_camera_sequence_button = document.getElementById('load_camera_sequence_button');
const camera_sequence_modal = document.getElementById('camera_sequence_modal');
const camera_sequence_modal_close_button = document.getElementById('camera_sequence_modal_close_button');
const camera_sequence_file_select = document.getElementById('camera_sequence_file_select');
const confirm_camera_sequence_button = document.getElementById('confirm_camera_sequence_button');
const cancel_camera_sequence_button = document.getElementById('cancel_camera_sequence_button');

function show_camera_sequence_modal() {
    camera_sequence_modal.style.display = 'flex';
}

function hide_camera_sequence_modal() {
    camera_sequence_modal.style.display = 'none';
}

function populate_camera_sequence_modal(files_array) {
    camera_sequence_file_select.innerHTML = ''; // Clear existing options
    if (files_array && files_array.length > 0) {
        files_array.forEach(file_name => {
            const option = document.createElement('option');
            option.value = file_name;
            option.textContent = file_name;
            camera_sequence_file_select.appendChild(option);
        });
    } else {
        const option = document.createElement('option');
        option.value = "";
        option.textContent = "No sequence files found.";
        camera_sequence_file_select.appendChild(option);
    }
}

async function handle_load_camera_sequence_click() {
    try {
        camera_sequence_file_select.innerHTML = '<option value="">Loading...</option>'; // Show loading message in select
        show_camera_sequence_modal(); // Show modal immediately
        const file_list = await fetchData('/api/camera_sequence_list');
        populate_camera_sequence_modal(file_list);
    } catch (error) {
        console.error('Error fetching camera sequence list:', error);
        populate_camera_sequence_modal([]); // Show "No sequence files found" or error
        // alert is already handled by fetchData
    }
}

function handle_camera_sequence_modal_close_click() {
    hide_camera_sequence_modal();
}

async function handle_confirm_camera_sequence_click() {
    const selected_file_name = camera_sequence_file_select.value;

    if (!selected_file_name) {
        alert('Please select a camera sequence file.');
        return;
    }

    // Show calculating modal while loading
    calculating_modal.style.display = 'flex';
    hide_camera_sequence_modal(); // Hide the selection modal

    try {
        // Assuming the API returns a success status or throws an error via fetchData
        await fetchData(`/api/camera_sequence_load/${selected_file_name}`);
        console.log(`Camera sequence file "${selected_file_name}" loaded successfully.`);
        load_camera_sequence_button.classList.add('loaded');
        // load_camera_sequence_button.style.backgroundColor = 'green'; // Direct style or class
    } catch (error) {
        console.error(`Error loading camera sequence file "${selected_file_name}":`, error);
        load_camera_sequence_button.classList.remove('loaded'); // Revert button color on failure
        // load_camera_sequence_button.style.backgroundColor = 'red'; // Revert to red
        // alert is already handled by fetchData
    } finally {
        calculating_modal.style.display = 'none'; // Always hide calculating modal
    }
}

function handle_cancel_camera_sequence_click() {
    hide_camera_sequence_modal();
}

// Unified click handler for closing modals
function handle_window_click(event) {
    if (event.target === rename_modal) {
        rename_modal.style.display = 'none';
    } else if (event.target === event_selection_modal) {
        event_selection_modal.style.display = 'none';
    } else if (event.target === run_sim_modal) {
        run_sim_modal.style.display = 'none';
    } else if (event.target === camera_sequence_modal) { // NEW: Add camera sequence modal
        camera_sequence_modal.style.display = 'none';
    }
    // calculating_modal should not close on outside click
}


// === All Initial Setup on DOMContentLoaded ===
document.addEventListener('DOMContentLoaded', function() {
    const rename_modal_close_button_el = rename_modal.querySelector('.close_button'); // Unique var name
    const event_modal_close_button_el = event_selection_modal.querySelector('.close_button'); // Unique var name

    if(rename_modal_close_button_el) rename_modal_close_button_el.addEventListener('click', handle_rename_modal_close_button_click);
    if(cancel_rename_button) cancel_rename_button.addEventListener('click', handle_cancel_rename_click);
    if(save_rename_button) save_rename_button.addEventListener('click', handle_save_rename_click);
    if(rename_input) rename_input.addEventListener('keypress', handle_rename_input_keypress);

    const file_selection_row = event_table_body.insertRow(0);
    file_selection_row.id = 'event_file_selection_row';
    const label_cell = file_selection_row.insertCell(0);
    label_cell.textContent = 'Event File:';
    label_cell.classList.add('column_1');
    const filename_cell = file_selection_row.insertCell(1);
    filename_cell.classList.add('event_filename_cell');
    filename_cell.style.cursor = 'pointer';
    const filename_pre = document.createElement('pre');
    filename_pre.textContent = 'Click to Select File';
    filename_cell.appendChild(filename_pre);
    event_file_name_pre_element = filename_pre;
    file_selection_row.insertCell(2);

    filename_cell.addEventListener('click', () => {
        fetch_files_for_modal();
        event_selection_modal.style.display = 'flex';
    });

    if(event_modal_close_button_el) event_modal_close_button_el.addEventListener('click', () => {
        event_selection_modal.style.display = 'none';
    });

    if(run_sim_button) run_sim_button.addEventListener('click', handle_run_sim_button_click);
    if(run_sim_modal_close_button) run_sim_modal_close_button.addEventListener('click', handle_sim_cancel_click);
    if(sim_okay_button) sim_okay_button.addEventListener('click', handle_sim_okay_click);
    if(sim_cancel_button) sim_cancel_button.addEventListener('click', handle_sim_cancel_click);

    if(sim_latitude_input) sim_latitude_input.addEventListener('keypress', handle_sim_input_keypress);
    if(sim_longitude_input) sim_longitude_input.addEventListener('keypress', handle_sim_input_keypress);
    if(sim_altitude_input) sim_altitude_input.addEventListener('keypress', handle_sim_input_keypress);
    if(sim_event_id_input) sim_event_id_input.addEventListener('keypress', handle_sim_input_keypress);
    if(sim_time_offset_input) sim_time_offset_input.addEventListener('keypress', handle_sim_input_keypress);

    // === NEW: Camera Sequence Event Listeners ===
    if(load_camera_sequence_button) load_camera_sequence_button.addEventListener('click', handle_load_camera_sequence_click);
    if(camera_sequence_modal_close_button) camera_sequence_modal_close_button.addEventListener('click', handle_camera_sequence_modal_close_click);
    if(confirm_camera_sequence_button) confirm_camera_sequence_button.addEventListener('click', handle_confirm_camera_sequence_click);
    if(cancel_camera_sequence_button) cancel_camera_sequence_button.addEventListener('click', handle_cancel_camera_sequence_click);


    window.addEventListener('click', handle_window_click);

    update_run_sim_button_state();

    let interval = 1000;
    fetch_gps();
    setInterval(fetch_gps, interval);
    fetch_cameras();
    setInterval(fetch_cameras, interval);
    setInterval(update_dynamic_events, interval);
});
