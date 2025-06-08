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
const camera_sequence_file_select = document.getElementById('camera_sequence_file_select');
const confirm_camera_sequence_button = document.getElementById('confirm_camera_sequence_button');
const cancel_camera_sequence_button = document.getElementById('cancel_camera_sequence_button');
const camera_control_state_value = document.getElementById('camera_control_state_value');
const control_tables_container = document.getElementById('control_tables_container');

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
	gps_latitude.textContent = data.lat !== undefined ? data.lat.toFixed(4) : "N/A";
	gps_longitude.textContent = data.long !== undefined ? data.long.toFixed(4) : "N/A";
	gps_altitude.textContent = data.altitude !== undefined ? data.altitude.toFixed(1) : "N/A";
}

function update_cameras_ui(data)
{
	cameras_table_body.innerHTML = "";

	if (!data || data.num_cameras == 0 || !data.detected)
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

	for (let camera_info of data.detected)
	{
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

		cell_serial.textContent = camera_info.serial || "N/A";
		cell_desc.textContent = camera_info.desc || "N/A";
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
		cell_desc.dataset.serial = camera_info.serial;

		const svg_connected = document.createElement('img');
		svg_connected.width = "70";
		svg_connected.height = "50";

		if (camera_info.connected == "0" || !camera_info.connected)
		{
			svg_connected.src = "/static/cam-disconnected.svg";
			svg_connected.alt = "Camera Disconnected";
		}
		else
		{
			svg_connected.src = "/static/cam-connected.svg";
			svg_connected.alt = "Camera Connected";
			cell_battery.textContent = camera_info.batt || "N/A";
			cell_port.textContent = camera_info.port || "N/A";
			cell_available_shots.textContent = camera_info.num_photos || "N/A";
			cell_quality.textContent = camera_info.quality || "N/A";
			cell_mode.textContent = camera_info.mode || "N/A";
			cell_iso.textContent = camera_info.iso || "N/A";
			cell_fstop.textContent = camera_info.fstop || "N/A";
			cell_shutter.textContent = camera_info.shutter || "N/A";
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

// *** THIS FUNCTION IS UPDATED FOR THE NEW DATA FORMAT ***
function create_control_table_for_camera(camera_data)
{
	const table = document.createElement('table');
	table.classList.add('control-table');

	const title_body = table.createTBody();
	const title_row = title_body.insertRow();
	const title_cell = title_row.insertCell();
	title_cell.colSpan = 4;
	title_cell.className = 'control-table-title';
	title_cell.textContent = `${camera_data.name} (${camera_data.position} / ${camera_data.num_events})`;

	const head = table.createTHead();
	const header_row = head.insertRow();
	const headers = ["Event ID", "ETA", "Channel", "Value"];
	headers.forEach(header_text =>
	{
		const th = document.createElement('th');
		th.textContent = header_text;
		header_row.appendChild(th);
	});

	const data_body = table.createTBody();
    // The loop is removed. We now create just one row with the flattened data.
	const row = data_body.insertRow();
	row.insertCell().textContent = camera_data.event_id || 'N/A';
	row.insertCell().textContent = camera_data.ETA || 'N/A';
	row.insertCell().textContent = camera_data.channel || 'N/A';
	row.insertCell().textContent = camera_data.value || 'N/A';

	return table;
}

function update_camera_control_ui(control_data)
{
	if (!control_data || !control_data.state || control_data.state === "idle" || !control_data.cameras)
	{
		camera_control_state_value.textContent = 'idle';
		control_tables_container.innerHTML = '';
		return;
	}

	camera_control_state_value.textContent = control_data.state;

	control_tables_container.innerHTML = '';
	control_data.cameras.forEach(camera =>
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
		else
		{
			console.warn("Event data missing 'events' list or malformed:", event_data);
			update_event_table_row('Events', 'No events defined or format error', '');
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
				if (success)
				{
					load_event_file_button.classList.add('loaded');
				}
				else
				{
					load_event_file_button.classList.remove('loaded');
				}
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
			const result = await fetch_data('/api/run_sim/stop');
			is_sim_running = false;
			update_run_sim_button_state();
			console.log('Simulation stop signal sent. Backend response:', result);
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
	const params = {
		gps_latitude: parseFloat(sim_latitude_input.value),
		gps_longitude: parseFloat(sim_longitude_input.value),
		gps_altitude: parseFloat(sim_altitude_input.value),
		event_id: sim_event_id_input.value,
		event_time_offset: parseFloat(sim_time_offset_input.value)
	};

	if (isNaN(params.gps_latitude) || isNaN(params.gps_longitude) || isNaN(params.gps_altitude) || isNaN(params.event_time_offset) || !params.event_id)
	{
		alert("Please enter valid numbers for GPS coordinates/offset and a valid Event ID.");
		return;
	}

	run_sim_modal.style.display = 'none';
	calculating_modal.style.display = 'flex';

	try
	{
		const query_params = new URLSearchParams(params).toString();
		const result = await fetch_data(`/api/run_sim?${query_params}`);
		is_sim_running = true;
		update_run_sim_button_state();
		console.log('Simulation start signal sent with params. Backend response:', params, result);
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
	camera_sequence_file_select.innerHTML = '';
	if (files_array && files_array.length > 0)
	{
		files_array.forEach(file_name =>
		{
			const option = document.createElement('option');
			option.value = file_name;
			option.textContent = file_name;
			camera_sequence_file_select.appendChild(option);
		});
	}
	else
	{
		const option = document.createElement('option');
		option.value = "";
		option.textContent = "No sequence files found.";
		camera_sequence_file_select.appendChild(option);
	}
}

async function handle_load_camera_sequence_click()
{
	try
	{
		camera_sequence_file_select.innerHTML = '<option value="">Loading...</option>';
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

function handle_camera_sequence_modal_close_click()
{
	hide_camera_sequence_modal();
}

async function handle_confirm_camera_sequence_click()
{
	const selected_file_name = camera_sequence_file_select.value;

	if (!selected_file_name)
	{
		alert('Please select a camera sequence file.');
		return;
	}

	calculating_modal.style.display = 'flex';
	hide_camera_sequence_modal();

	try
	{
		await fetch_data('/api/camera_sequence_load', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ filename: selected_file_name })
        });
		console.log(`Camera sequence file "${selected_file_name}" loaded successfully.`);
		load_camera_sequence_button.classList.add('loaded');
	}
	catch (error)
	{
		console.error(`Error loading camera sequence file "${selected_file_name}":`, error);
		load_camera_sequence_button.classList.remove('loaded');
	}
	finally
	{
		calculating_modal.style.display = 'none';
	}
}

function handle_cancel_camera_sequence_click()
{
	hide_camera_sequence_modal();
}

function handle_window_click(event)
{
	if (event.target === rename_modal)
	{
		rename_modal.style.display = 'none';
	}
	else if (event.target === event_selection_modal)
	{
		event_selection_modal.style.display = 'none';
	}
	else if (event.target === run_sim_modal)
	{
		run_sim_modal.style.display = 'none';
	}
	else if (event.target === camera_sequence_modal)
	{
		camera_sequence_modal.style.display = 'none';
	}
}

// --- Main Dashboard Update Loop ---
async function update_dashboard()
{
	try
	{
		const dashboard_data = await fetch_data('/api/dashboard_update');

		if (dashboard_data.gps)
		{
			update_gps_ui(dashboard_data.gps);
		}
		if (dashboard_data.cameras)
		{
			update_cameras_ui(dashboard_data.cameras);
		}
		if (dashboard_data.events && event_row_map.size > 0)
		{
			update_events_ui(dashboard_data.events);
		}
		if (dashboard_data.camera_control)
		{
			update_camera_control_ui(dashboard_data.camera_control);
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
	const rename_modal_close_button = rename_modal.querySelector('.close_button');
	const event_modal_close_button = event_selection_modal.querySelector('.close_button');

	if (rename_modal_close_button)
	{
		rename_modal_close_button.addEventListener('click', handle_rename_modal_close_button_click);
	}
	if (cancel_rename_button)
	{
		cancel_rename_button.addEventListener('click', handle_cancel_rename_click);
	}
	if (save_rename_button)
	{
		save_rename_button.addEventListener('click', handle_save_rename_click);
	}
	if (rename_input)
	{
		rename_input.addEventListener('keypress', handle_rename_input_keypress);
	}

	if (load_event_file_button)
	{
		load_event_file_button.addEventListener('click', () =>
		{
			fetch_files_for_modal();
			event_selection_modal.style.display = 'flex';
		});
	}

	if (event_modal_close_button)
	{
		event_modal_close_button.addEventListener('click', () =>
		{
			event_selection_modal.style.display = 'none';
		});
	}

	if (run_sim_button)
	{
		run_sim_button.addEventListener('click', handle_run_sim_button_click);
	}
	if (run_sim_modal_close_button)
	{
		run_sim_modal_close_button.addEventListener('click', handle_sim_cancel_click);
	}
	if (sim_okay_button)
	{
		sim_okay_button.addEventListener('click', handle_sim_okay_click);
	}
	if (sim_cancel_button)
	{
		sim_cancel_button.addEventListener('click', handle_sim_cancel_click);
	}

	if (sim_latitude_input) sim_latitude_input.addEventListener('keypress', handle_sim_input_keypress);
	if (sim_longitude_input) sim_longitude_input.addEventListener('keypress', handle_sim_input_keypress);
	if (sim_altitude_input) sim_altitude_input.addEventListener('keypress', handle_sim_input_keypress);
	if (sim_event_id_input) sim_event_id_input.addEventListener('keypress', handle_sim_input_keypress);
	if (sim_time_offset_input) sim_time_offset_input.addEventListener('keypress', handle_sim_input_keypress);

	if (load_camera_sequence_button)
	{
		load_camera_sequence_button.addEventListener('click', handle_load_camera_sequence_click);
	}
	if (camera_sequence_modal_close_button)
	{
		camera_sequence_modal_close_button.addEventListener('click', handle_camera_sequence_modal_close_click);
	}
	if (confirm_camera_sequence_button)
	{
		confirm_camera_sequence_button.addEventListener('click', handle_confirm_camera_sequence_click);
	}
	if (cancel_camera_sequence_button)
	{
		cancel_camera_sequence_button.addEventListener('click', handle_cancel_camera_sequence_click);
	}

	window.addEventListener('click', handle_window_click);

	update_run_sim_button_state();

	// Setup the single, consolidated update loop
	const update_interval = 1000;
	update_dashboard(); // Initial fetch
	setInterval(update_dashboard, update_interval);
});
