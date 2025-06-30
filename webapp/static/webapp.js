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
let current_editable_td = null;
let is_sim_running = false;

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
		alert(`Error: ${error.message}`);
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
            <td>${(cam.connected != "0" && cam.connected) ? (cam.num_photos || "N/A") : "N/A"}</td>
            <td class="choice-td" data-property="quality" data-serial="${cam.serial}">${(cam.connected != "0" && cam.connected) ? (cam.quality || "N/A") : "N/A"}</td>
            <td class="choice-td" data-property="mode" data-serial="${cam.serial}">${(cam.connected != "0" && cam.connected) ? (cam.mode || "N/A") : "N/A"}</td>
            <td class="choice-td" data-property="iso" data-serial="${cam.serial}">${(cam.connected != "0" && cam.connected) ? (cam.iso || "N/A") : "N/A"}</td>
            <td class="choice-td" data-property="fstop" data-serial="${cam.serial}">${(cam.connected != "0" && cam.connected) ? (cam.fstop || "N/A") : "N/A"}</td>
            <td class="choice-td" data-property="shutter" data-serial="${cam.serial}">${(cam.connected != "0" && cam.connected) ? (cam.shutter || "N/A") : "N/A"}</td>
        `;
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
	if (typeof events_data === 'object' && events_data !== null)
	{
		for (const event_id in events_data)
		{
			if (events_data.hasOwnProperty(event_id))
			{
				const event_info = events_data[event_id];
				if (Array.isArray(event_info) && event_info.length === 2)
				{
					const [event_time, eta] = event_info;
					update_event_table_row(event_id, event_time, eta);
				}
				else
				{
					console.warn(`Malformed event info for ${event_id}. Received:`, event_info);
				}
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


// --- Main Application Logic & Event Handlers ---

function handle_editable_td_click()
{
	current_editable_td = this;
	rename_input.value = current_editable_td.textContent.trim();
	rename_modal.style.display = 'flex';
	rename_input.focus();
}

async function handle_choice_td_click(event) {
    const cell = event.currentTarget;
    const serial = cell.dataset.serial;
    const property = cell.dataset.property;
    const desc = cell.dataset.desc;

    if (!serial || !property) {
        console.error("Cell is missing data-serial or data-property attribute.");
        return;
    }

    camera_choice_modal_title.textContent = `Set ${property} for ${desc}`;
    camera_choice_list.innerHTML = '<li>Loading choices...</li>';
    camera_choice_modal.style.display = 'flex';

    try {
        const choices = await fetch_data('/api/camera/read_choices', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(
			{
				serial: serial,
				property: property
			})
        });
        populate_camera_choice_modal(choices, serial, property);
    } catch (error) {
        camera_choice_list.innerHTML = '<li>Error loading choices.</li>';
    }
}

function populate_camera_choice_modal(choices, serial, property) {
    camera_choice_list.innerHTML = '';
    if (!choices || !Array.isArray(choices) || choices.length === 0) {
        camera_choice_list.innerHTML = '<li>No choices available.</li>';
        return;
    }

    choices.forEach(choice => {
        const li = document.createElement('li');
        li.textContent = choice;
        li.addEventListener('click', async () => {
            const value = choice;
            camera_choice_modal.style.display = 'none';
            calculating_modal.style.display = 'flex';

            try {
                await fetch_data('/api/camera/set_choice', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(
                    {
                        serial: serial,
                        property: property,
                        value: value
                    })
                });
                await update_dashboard();
            } catch (error) {
                // Error is already alerted by fetch_data
            } finally {
                calculating_modal.style.display = 'none';
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
	try
	{
		await fetch_data('/api/camera/update_description',
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
		console.log('Camera description updated successfully.');
	}
	catch (error)
	{
		console.error('Error sending data to backend:', error);
	}
}

function handle_save_rename_click()
{
	if (current_editable_td)
	{
		const new_description = rename_input.value.trim();
		current_editable_td.textContent = new_description;
		send_camera_description_to_backend(
			current_editable_td.dataset.serial,
			new_description
		);
	}
	rename_modal.style.display = 'none';
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

function clear_event_data_rows()
{
	event_table_body.innerHTML = '';
	event_row_map.clear();
}

async function populate_static_event_details(file_name)
{
	clear_event_data_rows();

	const file_display_row = event_table_body.insertRow(0);
	const file_label_cell = file_display_row.insertCell(0);
	const file_name_cell = file_display_row.insertCell(1);

	file_label_cell.textContent = "Event File:";
	file_label_cell.classList.add('column_1');
	file_name_cell.colSpan = 2;
	file_name_cell.appendChild(document.createElement('pre')).textContent = file_name;

	const loading_row = event_table_body.insertRow(1);
	const loading_cell = loading_row.insertCell(0);
	loading_cell.colSpan = 3;
	loading_cell.textContent = `Loading details for ${file_name}...`;

	try
	{
		const event_data = await fetch_data('/api/event_load', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ filename: file_name })
        });
		loading_row.remove();

		update_event_table_row('Type', event_data.type || 'N/A', '');

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
		console.error(`Error fetching static event details for ${file_name}:`, error);
		clear_event_data_rows();
		update_event_table_row('Error', `Failed to load ${file_name}.`, error.message.substring(0, 100));
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
	try
	{
		const data = await fetch_data('/api/dashboard_update');

		if (data.gps)
		{
			update_gps_ui(data.gps);
		}
		if (data.detected_cameras)
		{
			update_cameras_ui(data.detected_cameras);
		}
		if (data.events && event_row_map.size > 0)
		{
			update_events_ui(data.events);
		}
		if (data.camera_control)
		{
			update_camera_control_ui(data.camera_control);
		}
	}
	catch (error)
	{
		console.error("Failed to update dashboard:", error);
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
