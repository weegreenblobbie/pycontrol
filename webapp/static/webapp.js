function format_string()
{
    let args = arguments,
        format_str = args[0],
        arg_index = 1;
    return format_str.replace(/%((%)|s|d)/g, function(match)
    {
        let value = null;
        if (match[2])
        {
            value = match[2];
        }
        else
        {
            value = args[arg_index];
            switch (match)
            {
                case '%d':
                    value = parseFloat(value);
                    if (isNaN(value))
                    {
                        value = 0;
                    }
                    break;
            }
            arg_index++;
        }
        return value;
    });
}

// --- DOM Element References ---
const gps_connection = document.getElementById("gps-connection");
const gps_mode = document.getElementById("gps-mode");
const gps_mode_time = document.getElementById("gps-mode-time");
const gps_sats = document.getElementById("gps-sats");
const gps_time = document.getElementById("gps-time");
const gps_latitude = document.getElementById("gps-latitude");
const gps_longitude = document.getElementById("gps-longitude");
const gps_altitude = document.getElementById("gps-altitude");

const cameras_table_body = document.getElementById("cameras").tBodies[0];
const rename_modal = document.getElementById('rename_modal');
const rename_input = document.getElementById('rename_input');
const save_rename_button = document.getElementById('save_rename');
const cancel_rename_button = document.getElementById('cancel_rename');
const event_table_body = document.querySelector('#event_table tbody');
const event_info_table_body = document.getElementById("event_info_table").tBodies[0];
const event_selection_modal = document.getElementById('event_selection_modal');
const file_list_element = document.getElementById('file_list');
const event_row_map = new Map();
const load_event_file_button = document.getElementById('load_event_file_button');
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
const load_camera_sequence_button = document.getElementById('load_camera_sequence_button');
const camera_sequence_modal = document.getElementById('camera_sequence_modal');
const camera_sequence_modal_close_button = document.getElementById('camera_sequence_modal_close_button');
const camera_sequence_file_list = document.getElementById('camera_sequence_file_list');
const camera_control_state_value = document.getElementById('camera_control_state_value');
const control_tables_container = document.getElementById('control_tables_container');
const camera_choice_modal = document.getElementById('camera_choice_modal');
const camera_choice_modal_title = document.getElementById('camera_choice_modal_title');
const camera_choice_list = document.getElementById('camera_choice_list');

// --- State Variables ---
let active_cell_info = null;
let current_editable_td = null;
let is_sim_running = false;
let is_event_table_initialized = false;

// --- Data Fetching ---
async function fetch_data(url, options = {})
{
    try
    {
        const response = await fetch(url, options);

        if (!response.ok)
        {
            let error_message = `HTTP error! Status: ${response.status}`;
            try
            {
                const error_data = await response.json();
                if (error_data && error_data.message)
                {
                    error_message = error_data.message;
                }
            }
            catch (json_error)
            {
                console.warn(`Could not parse error response as JSON for ${url}:`, json_error);
                try
                {
                    const text_response = await response.text();
                    if (text_response && text_response.length > 0)
                    {
                        error_message += ` - Response: ${text_response.substring(0, 100)}...`;
                    }
                }
                catch (text_error)
                {
                    console.warn(`Could not read error response as text for ${url}:`, text_error);
                }
            }
            throw new Error(error_message);
        }

        const content_type = response.headers.get("content-type");
        if (content_type && content_type.indexOf("application/json") !== -1)
        {
            const text = await response.text();
            return text ? JSON.parse(text) : {};
        }
        return {};
    }
    catch (error)
    {
        console.error(`Fetch operation failed for ${url}:`, error);

        const error_modal = document.getElementById('error_modal');
        const error_message_p = document.getElementById('error_modal_message');
        const reload_button = document.getElementById('reload_button');

        if (error_modal && error_message_p && reload_button)
        {
            error_message_p.textContent = `Error: ${error.message}. Check the connection and try again.`;
            reload_button.onclick = () => { window.location.reload(); };
            error_modal.style.display = 'flex';
        }
        else
        {
            // Fallback to alert if the modal elements don't exist for some reason
            alert(`Error: ${error.message}`);
        }
		throw error;
	}
}


// --- UI Update Functions ---

function update_gps_ui(data)
{
    if (!data.connected)
    {
        gps_connection.src = "/static/gps-disconnected.svg";
        gps_mode.textContent = data.mode || "N/A";
        gps_mode_time.textContent = data.mode_time || "N/A";
        gps_sats.textContent = "N/A";
        gps_time.textContent = "N/A";
        gps_latitude.textContent = "N/A";
        gps_longitude.textContent = "N/A";
        gps_altitude.textContent = "N/A";
        return;
    }

    if (data.mode == "2D FIX")
    {
        gps_connection.src = "/static/gps-degraded.svg";
    }
    else if (data.mode == "3D FIX")
    {
        gps_connection.src = "/static/gps-connected.svg";
    }
    else
    {
        gps_connection.src = "/static/gps-disconnected.svg";
    }

    gps_mode.textContent = data.mode;
    gps_mode_time.textContent = data.mode_time;
    gps_sats.textContent = format_string("%d/%d", data.sats_used, data.sats_seen);
    gps_time.textContent = data.time;
    gps_latitude.textContent = data.lat !== undefined ? data.lat.toFixed(6) : "N/A";
    gps_longitude.textContent = data.long !== undefined ? data.long.toFixed(6) : "N/A";
    gps_altitude.textContent = data.altitude !== undefined ? data.altitude.toFixed(2) : "N/A";

    console.log("gps: " + data);
}

function update_cameras_ui(data)
{
    cameras_table_body.innerHTML = "";

    const num_cameras = data.length;

    if (!data || num_cameras == 0)
    {
        const row = document.createElement('tr');
        const cell = document.createElement('td');
        cell.colSpan = 11;
        cell.textContent = "No cameras detected or data unavailable.";
        cell.style.textAlign = "center";
        row.appendChild(cell);
        cameras_table_body.appendChild(row);
        return;
    }

    for (let cam of data)
    {
        const row = document.createElement('tr');
        row.innerHTML = `
            <td><img src="${(cam.connected == "0" || !cam.connected) ? '/static/cam-disconnected.svg' : '/static/cam-connected.svg'}" width="70" height="50" alt="Camera connection status"></td>
            <td>${cam.serial || "N/A"}</td>
            <td class="editable_td" data-property="description" data-serial="${cam.serial}">${cam.desc || "N/A"}</td>
            <td>${(cam.connected != "0" && cam.connected) ? (cam.batt || "N/A") : "N/A"}</td>
            <td>${(cam.connected != "0" && cam.connected) ? (cam.port || "N/A") : "N/A"}</td>
            <td>${(cam.connected != "0" && cam.connected) ? cam.num_avail : "N/A"}</td>
            <td>${(cam.connected != "0" && cam.connected) ? cam.num_photos : "N/A"}</td>
            <td class="choice-td" data-property="quality" data-serial="${cam.serial}" data-desc="${cam.desc || ''}">${(cam.connected != "0" && cam.connected) ? (cam.quality || "N/A") : "N/A"}</td>
            <td class="choice-td" data-property="mode" data-serial="${cam.serial}" data-desc="${cam.desc || ''}">${(cam.connected != "0" && cam.connected) ? (cam.mode || "N/A") : "N/A"}</td>
            <td class="choice-td" data-property="iso" data-serial="${cam.serial}" data-desc="${cam.desc || ''}">${(cam.connected != "0" && cam.connected) ? (cam.iso || "N/A") : "N/A"}</td>
            <td class="choice-td" data-property="fstop" data-serial="${cam.serial}" data-desc="${cam.desc || ''}">${(cam.connected != "0" && cam.connected) ? (cam.fstop || "N/A") : "N/A"}</td>
            <td class="choice-td" data-property="shutter" data-serial="${cam.serial}" data-desc="${cam.desc || ''}">${(cam.connected != "0" && cam.connected) ? (cam.shutter || "N/A") : "N/A"}</td>
            <td class="choice-td" data-property="burst" data-serial="${cam.serial}" data-desc="${cam.desc || ''}">${(cam.connected != "0" && cam.connected) ? (cam.burst_number || "N/A") : "N/A"}</td>
        `;

        // Create the new cell for the trigger button
        const cell_trigger = row.insertCell();

        // Only add a button if the camera is connected
        if (cam.connected && cam.connected != "0") {
            const trigger_button = document.createElement('button');
            trigger_button.textContent = 'Trigger';
            trigger_button.className = 'trigger-button';
            trigger_button.dataset.serial = cam.serial;
            trigger_button.dataset.desc = cam.desc;
            trigger_button.addEventListener('click', handle_trigger_click);
            cell_trigger.appendChild(trigger_button);
        }

        cameras_table_body.appendChild(row);
    }

    document.querySelectorAll('.editable_td').forEach(cell => {
        cell.addEventListener('click', handle_editable_td_click);
    });
    document.querySelectorAll('.choice-td').forEach(cell => {
        cell.addEventListener('click', handle_choice_td_click);
    });
}

function update_events_ui(events_data)
{
    if (events_data && events_data.length > 0)
    {
        for (const event_info of events_data)
        {
            if (event_info && event_info.length === 3)
            {
                const [event_id, event_time, eta] = event_info;
                update_event_table_row(event_id, event_time, eta);
            }
            else
            {
                console.warn(`Malformed event info in list. Expected [id, time, eta]. Received:`, event_info);
            }
        }
    }
    else
    {
        console.warn("Unexpected data format for events. Received:", events_data);
    }
}

function create_control_table_for_camera(camera_data)
{
    const table = document.createElement('table');
    table.classList.add('control-table');

    const head = table.createTHead();
    const header_row = head.insertRow();

    const first_header = document.createElement('th');
    first_header.textContent = `${camera_data.id} (# of ${camera_data.num_events || 'N/A'})`;
    header_row.appendChild(first_header);

    const other_headers = ["ETA", "Event ID", "Offset (HH:MM:SS.sss)", "Channel", "Value"];
    other_headers.forEach(header_text =>
    {
        const th = document.createElement('th');
        th.textContent = header_text;
        header_row.appendChild(th);
    });

    const data_body = table.createTBody();

    camera_data.events.forEach(event_obj =>
        {
            const row = data_body.insertRow();

            row.insertCell().textContent = event_obj.pos || 'N/A';
            row.insertCell().textContent = event_obj.eta || 'N/A';
            row.insertCell().textContent = event_obj.event_id || 'N/A';
            row.insertCell().textContent = event_obj.event_time_offset || 'N/A';
            row.insertCell().textContent = event_obj.channel || 'N/A';
            row.insertCell().textContent = event_obj.value || 'N/A';
        });

    return table;
}

function update_camera_control_ui(camera_control)
{
    if (!camera_control || !camera_control.state)
    {
        camera_control_state_value.textContent = 'unknown';
        control_tables_container.innerHTML = '';
        return;
    }

    camera_control_state_value.textContent = camera_control.state;

    control_tables_container.innerHTML = '';
    camera_control.sequence_state.forEach(camera =>
    {
        const camera_table = create_control_table_for_camera(camera);
        control_tables_container.appendChild(camera_table);
    });
}


// This function is for renaming the description
function handle_editable_td_click()
{
    // Store the info needed to find this cell later
    active_cell_info =
    {
        serial: this.dataset.serial,
        property: this.dataset.property,
        original_text: this.textContent.trim()
    };

    rename_input.value = active_cell_info.original_text;

    // The rest of this function is a robust way to open the modal
    rename_input.disabled = false;
    rename_input.readOnly = false;
    rename_modal.style.display = 'flex';
    setTimeout(() =>
        {
            rename_input.focus();
            rename_input.select();
        },
    0);
}

// This function is for quality, mode, iso, etc.
async function handle_choice_td_click(event)
{
    const cell = event.currentTarget;
    const serial = cell.dataset.serial;
    const property = cell.dataset.property;
    const desc = cell.dataset.desc;

    if (!serial || !property)
    {
        console.error("Cell is missing data-serial or data-property attribute.");
        return;
    }

    // Use the description for the title, falling back to the serial number if the description is blank.
    camera_choice_modal_title.textContent = `Set ${property} for camera '${desc || serial}'`;
    camera_choice_list.innerHTML = '<li>Loading choices...</li>';
    camera_choice_modal.style.display = 'flex';

    try
    {
        const choices = await fetch_data('/api/camera/read_choices',
        {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                serial: serial,
                property: property
            })
        });
        populate_camera_choice_modal(choices, serial, property);
    }
    catch (error)
    {
        camera_choice_list.innerHTML = '<li>Error loading choices.</li>';
    }
}

async function handle_trigger_click(event)
{
    const button = event.currentTarget;
    const serial = button.dataset.serial;

    if (!serial) return;

    console.log(`Triggering camera: ${serial}`);
    button.disabled = true;
    button.textContent = '...';

    try
    {
        await fetch_data('/api/camera/trigger',
        {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ serial: serial })
        });
    } catch (error)
    {
        button.disabled = false;
        button.textContent = 'Trigger';
    }
}

// This function no longer needs the 'cell' argument
function populate_camera_choice_modal(choices, serial, property)
{
    camera_choice_list.innerHTML = '';
    if (!choices || !Array.isArray(choices) || choices.length === 0)
    {
        camera_choice_list.innerHTML = '<li>No choices available.</li>';
        return;
    }

    choices.forEach(choice =>
    {
        const li = document.createElement('li');
        li.textContent = choice;
        li.addEventListener('click', async () =>
        {
            const value = choice;
            camera_choice_modal.style.display = 'none';

            // Find the live cell and optimistically update it
            const live_cell = document.querySelector(`td[data-serial="${serial}"][data-property="${property}"]`);
            if (live_cell)
            {
                live_cell.textContent = value;
            }

            calculating_modal.style.display = 'flex';
            try
            {
                await fetch_data('/api/camera/set_choice',
                {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(
                    {
                        serial: serial,
                        property: property,
                        value: value
                    })
                });
                // On success, re-find the cell and add the class
                const cell_to_highlight = document.querySelector(`td[data-serial="${serial}"][data-property="${property}"]`);
                if (cell_to_highlight)
                {
                    cell_to_highlight.classList.add('cell-pending-update');
                }
            }
            catch (error)
            {
                // On failure, the UI will be corrected on the next dashboard update
            }
            finally
            {
                calculating_modal.style.display = 'none';
                active_cell_info = null; // Clear state
            }
        });
        camera_choice_list.appendChild(li);
    });
}

function handle_rename_modal_close_button_click()
{
    rename_modal.style.display = 'none';
}

function handle_cancel_rename_click()
{
    rename_modal.style.display = 'none';
}

async function send_camera_description_to_backend(serial, description)
{
    return fetch_data('/api/camera/update_description',
    {
        method: 'POST',
        headers:
        {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(
        {
            serial: serial,
            description: description
        })
    });
}

async function handle_save_rename_click()
{
    if (active_cell_info)
    {
        const new_description = rename_input.value.trim();
        rename_modal.style.display = 'none';

        // Find the live cell right before we modify it
        const live_cell = document.querySelector(`td[data-serial="${active_cell_info.serial}"][data-property="${active_cell_info.property}"]`);
        if (live_cell)
        {
            live_cell.textContent = new_description; // Optimistic UI update
        }

        try
        {
            await send_camera_description_to_backend(
                active_cell_info.serial,
                new_description
            );
            // On success, find the live cell AGAIN and add the class
            const cell_to_highlight = document.querySelector(`td[data-serial="${active_cell_info.serial}"][data-property="${active_cell_info.property}"]`);
            if (cell_to_highlight)
            {
                cell_to_highlight.classList.add('cell-pending-update');
            }
        }
        catch (error)
        {
            console.error('Failed to save camera description. Reverting UI.', error);
            // On failure, find the live cell and revert its text
            const cell_to_revert = document.querySelector(`td[data-serial="${active_cell_info.serial}"][data-property="${active_cell_info.property}"]`);
            if (cell_to_revert)
            {
                cell_to_revert.textContent = active_cell_info.original_text;
            }
        }
        finally
        {
            active_cell_info = null; // Always clear the active cell info
        }
    }
}

function handle_rename_input_keypress(event)
{
    if (event.key === 'Enter')
    {
        save_rename_button.click();
    }
}

function get_or_create_pre_element(cell, class_name)
{
    let pre_element = cell.querySelector(`pre.${class_name}`);
    if (!pre_element)
    {
        pre_element = document.createElement('pre');
        pre_element.className = class_name;
        cell.innerHTML = '';
        cell.appendChild(pre_element);
    }
    return pre_element;
}

function update_event_table_row(event_id, event_time, eta = 'N/A')
{
    let row = event_row_map.get(event_id);

    if (!row)
    {
        row = event_table_body.insertRow();
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

async function populate_static_event_details(event_file, seq_file)
{
	// Clear both tables
	event_info_table_body.innerHTML = '';
	event_row_map.clear();
	is_event_table_initialized = false;

	try
	{
		const event_data = await fetch_data('/api/event_load', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ filename: event_file })
        });

        // Populate the new info table
        const info_row = event_info_table_body.insertRow();
        info_row.insertCell().textContent = event_file || 'N/A';
        info_row.insertCell().textContent = event_data.type || 'N/A';
        info_row.insertCell().textContent = seq_file || 'N/A';

		// Populate the main event list table
		if (event_data.events && Array.isArray(event_data.events))
		{
			event_data.events.forEach(event_id =>
			{
				update_event_table_row(event_id, 'N/A', 'Loading...');
			});
		}
		return true;
	}
	catch (error)
	{
		console.error(`Error fetching static event details for ${event_fil}:`, error);
		const error_row = event_info_table_body.insertRow();
        const cell = error_row.insertCell();
        cell.colSpan = 3;
        cell.textContent = `Error loading ${event_file}.`;
		return false;
	}
}

async function fetch_files_for_modal()
{
    file_list_element.innerHTML = '<li>Loading files...</li>';
    try
    {
        const file_list = await fetch_data("/api/event_list");
        file_list_element.innerHTML = '';

        if (!file_list || file_list.length === 0)
        {
            file_list_element.innerHTML = '<li>No files found.</li>';
            return;
        }

        file_list.forEach(file_name =>
        {
            const list_item = document.createElement('li');
            list_item.textContent = file_name;
            list_item.addEventListener('click', async () =>
            {
                event_selection_modal.style.display = 'none';
                const success = await populate_static_event_details(file_name);
                load_event_file_button.classList.toggle('loaded', success);
            });
            file_list_element.appendChild(list_item);
        });
    }
    catch (error)
    {
        console.error("Error fetching files for event modal:", error);
        file_list_element.innerHTML = `<li>Error loading files: ${error.message}</li>`;
    }
}

function update_run_sim_button_state()
{
    if (is_sim_running)
    {
        run_sim_button.textContent = "Stop Sim";
        run_sim_button.classList.add('running');
        run_sim_button.classList.remove('stopped');
    }
    else
    {
        run_sim_button.textContent = "Run Sim";
        run_sim_button.classList.add('stopped');
        run_sim_button.classList.remove('running');
    }
}

async function handle_run_sim_button_click()
{
    if (is_sim_running)
    {
        calculating_modal.style.display = 'flex';
        try
        {
            await fetch_data('/api/run_sim/stop');
            is_sim_running = false;
            update_run_sim_button_state();
        }
        catch (error)
        {
            console.error('Error stopping simulation:', error);
        }
        finally
        {
            calculating_modal.style.display = 'none';
        }
    }
    else
    {
        try
        {
            const defaults = await fetch_data('/api/run_sim/defaults');
            sim_latitude_input.value = defaults.gps_latitude || '';
            sim_longitude_input.value = defaults.gps_longitude || '';
            sim_altitude_input.value = defaults.gps_altitude || '';
            sim_event_id_input.value = defaults.event_id || '';
            sim_time_offset_input.value = defaults.event_time_offset !== undefined ? defaults.event_time_offset : '0';
            run_sim_modal.style.display = 'flex';
            sim_latitude_input.focus();
        }
        catch (error)
        {
            console.error('Error fetching simulation defaults:', error);
        }
    }
}

async function handle_sim_okay_click()
{
    const lat_str = sim_latitude_input.value.trim();
    const lon_str = sim_longitude_input.value.trim();
    const alt_str = sim_altitude_input.value.trim();
    const offset_str = sim_time_offset_input.value.trim();
    const event_id = sim_event_id_input.value.trim();

    if (!lat_str || !lon_str || !alt_str) {
        alert("Please provide values for GPS Latitude, Longitude, and Altitude.");
        return;
    }

    const gps_latitude = parseFloat(lat_str);
    const gps_longitude = parseFloat(lon_str);
    const gps_altitude = parseFloat(alt_str);

    if (isNaN(gps_latitude) || isNaN(gps_longitude) || isNaN(gps_altitude)) {
        alert("Please ensure GPS Latitude, Longitude, and Altitude are valid numbers.");
        return;
    }

    let event_time_offset = "";
    if (offset_str !== '') {
        event_time_offset = parseFloat(offset_str);
        if (isNaN(event_time_offset)) {
            alert("The Event Time Offset must be a valid number if provided.");
            return;
        }
    }

    const params = {
        gps_latitude,
        gps_longitude,
        gps_altitude,
        event_id,
        event_time_offset
    };

    run_sim_modal.style.display = 'none';
    calculating_modal.style.display = 'flex';

    try
    {
        const query_params = new URLSearchParams(params).toString();
        await fetch_data(`/api/run_sim?${query_params}`);
        is_sim_running = true;
        update_run_sim_button_state();
    }
    catch (error)
    {
        console.error('Error starting simulation:', error);
        is_sim_running = false;
        update_run_sim_button_state();
    }
    finally
    {
        calculating_modal.style.display = 'none';
    }
}

function handle_sim_cancel_click()
{
    run_sim_modal.style.display = 'none';
}

function handle_sim_input_keypress(event)
{
    if (event.key === 'Enter')
    {
        sim_okay_button.click();
    }
}

function show_camera_sequence_modal()
{
    camera_sequence_modal.style.display = 'flex';
}

function hide_camera_sequence_modal()
{
    camera_sequence_modal.style.display = 'none';
}

function populate_camera_sequence_modal(files_array)
{
    camera_sequence_file_list.innerHTML = '';
    if (files_array && files_array.length > 0)
    {
        files_array.forEach(file_name =>
        {
            const li = document.createElement('li');
            li.textContent = file_name;
            li.addEventListener('click', async () => {
                const selected_file_name = file_name;
                hide_camera_sequence_modal();
                calculating_modal.style.display = 'flex';
                try {
                    await fetch_data('/api/camera_sequence_load', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ filename: selected_file_name })
                    });
                    load_camera_sequence_button.classList.add('loaded');
                } catch (error) {
                    load_camera_sequence_button.classList.remove('loaded');
                } finally {
                    calculating_modal.style.display = 'none';
                }
            });
            camera_sequence_file_list.appendChild(li);
        });
    }
    else
    {
        camera_sequence_file_list.innerHTML = '<li>No sequence files found.</li>';
    }
}

async function handle_load_camera_sequence_click()
{
    try
    {
        camera_sequence_file_list.innerHTML = '<li>Loading...</li>';
        show_camera_sequence_modal();
        const file_list = await fetch_data('/api/camera_sequence_list');
        populate_camera_sequence_modal(file_list);
    }
    catch (error)
    {
        console.error('Error fetching camera sequence list:', error);
        populate_camera_sequence_modal([]);
    }
}

// --- Main Dashboard Update Loop ---
async function update_dashboard()
{
    let dashboard_data;

	try
	{
		dashboard_data = await fetch_data('/api/dashboard_update');
	}
	catch (error)
	{
		console.error("Failed to update dashboard:", error);
		return;
	}

	if (typeof dashboard_data === 'undefined' || dashboard_data === null)
	{
	    console.log("dashboard_data isn't defined");
	    return;
	}

    const event_file = dashboard_data.event_filename;
    const seq_file = dashboard_data.sequence_filename;

    if (dashboard_data.gps)
    {
        update_gps_ui(dashboard_data.gps);
    }
    if (dashboard_data.detected_cameras)
    {
        update_cameras_ui(dashboard_data.detected_cameras);
    }

    if (!is_event_table_initialized && event_file && seq_file)
    {
        console.log('Initial load detected for files:');
        console.log('    ' + event_file);
        console.log('    ' + seq_file);
        await populate_static_event_details(event_file, seq_file);
        load_event_file_button.classList.add('loaded');
        is_event_table_initialized = true;
    }
    if (seq_file)
    {
        load_camera_sequence_button.classList.add('loaded');
    }
    else
    {
        load_camera_sequence_button.classList.remove('loaded');
    }

    if (dashboard_data.events)
    {
        update_events_ui(dashboard_data.events);
    }

    if (dashboard_data.camera_control)
    {
        update_camera_control_ui(dashboard_data.camera_control);
    }
}


// --- DOM Ready - Initial Setup ---
document.addEventListener('DOMContentLoaded', function()
{
    // All getElementById calls are now at the top of the file

    // Rename Modal Listeners
    if (rename_modal) {
        rename_modal.querySelector('.close_button').addEventListener('click', handle_rename_modal_close_button_click);
        cancel_rename_button.addEventListener('click', handle_cancel_rename_click);
        save_rename_button.addEventListener('click', handle_save_rename_click);
        rename_input.addEventListener('keypress', handle_rename_input_keypress);
    }

    // Event File Modal Listeners
    if (load_event_file_button) {
        load_event_file_button.addEventListener('click', () => {
            fetch_files_for_modal();
            event_selection_modal.style.display = 'flex';
        });
    }

    // Run Sim Modal Listeners
    if (run_sim_button) run_sim_button.addEventListener('click', handle_run_sim_button_click);
    if (sim_okay_button) sim_okay_button.addEventListener('click', handle_sim_okay_click);
    if (sim_cancel_button) sim_cancel_button.addEventListener('click', handle_sim_cancel_click);
    [sim_latitude_input, sim_longitude_input, sim_altitude_input, sim_event_id_input, sim_time_offset_input].forEach(input => {
        if(input) input.addEventListener('keypress', handle_sim_input_keypress);
    });

    // Camera Sequence Modal Listeners
    if (load_camera_sequence_button)
    {
        load_camera_sequence_button.addEventListener('click', handle_load_camera_sequence_click);
    }

    // Robust modal closing logic for all modals
    const all_modals = document.querySelectorAll('.modal');
    all_modals.forEach(modal => {
        let is_mouse_down_on_background = false;

        const close_button = modal.querySelector('.close_button');
        if (close_button) {
            close_button.addEventListener('click', () => {
                modal.style.display = 'none';
            });
        }

        modal.addEventListener('mousedown', (event) => {
            if (event.target === modal) {
                is_mouse_down_on_background = true;
            }
        });
        modal.addEventListener('mouseup', (event) => {
            if (is_mouse_down_on_background && event.target === modal) {
                modal.style.display = 'none';
            }
            is_mouse_down_on_background = false;
        });
    });

    update_run_sim_button_state();

    // Setup the single, consolidated update loop
    const update_interval = 1000;
    update_dashboard(); // Initial fetch
    setInterval(update_dashboard, update_interval);
});
