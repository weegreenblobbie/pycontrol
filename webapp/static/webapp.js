// Library stuff.
function sprintf()
{
    // https://stackoverflow.com/a/4795914/562106
    var args = arguments,
        string = args[0],
        i = 1;
    return string.replace(/%((%)|s|d)/g, function (m)
    {
        // m is the matched format, e.g. %s, %d
        var val = null;
        if (m[2])
        {
            val = m[2];
        } else
        {
            val = args[i];
            // A switch statement so that the formatter can be extended. Default is %s
            switch (m)
            {
                case '%d':
                    val = parseFloat(val);
                    if (isNaN(val))
                    {
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

function fetch_gps()
{
    fetch('/api/gps')
        .then(response => response.json())
        .then(data =>
        {
            if (!data.connected)
            {
                gps_connection.src = "/static/gps-disconnected.svg";
                gps_mode.textContent = data.mode;
                gps_mode_time.textContent = data.mode_time;

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
            } else if (data.mode == "3D FIX")
            {
                gps_connection.src = "/static/gps-connected.svg";
            }

            gps_mode.textContent = data.mode;
            gps_mode_time.textContent = data.mode_time;
            gps_sats.textContent = sprintf("%d/%d", data.sats_used, data.sats_seen);
            gps_time.textContent = data.time;
            gps_latitude.textContent = data.lat.toFixed(4);
            gps_longitude.textContent = data.long.toFixed(4);
            gps_altitude.textContent = data.altitude.toFixed(1);
        })
        .catch(error =>
        {
            console.error('Error fetching GPS data:', error);
        });
}

// === Global Cameras Variables ===
const cameras = document.getElementById("cameras").tBodies[0];

function fetch_cameras()
{
    fetch('/api/cameras')
        .then(response => response.json())
        .then(data =>
        {
            cameras.innerHTML = ""; // Erase existing rows

            if (data.num_cameras == 0)
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

                cell_connected.textContent = "N/A";
                cell_serial.textContent = "N/A";
                cell_desc.textContent = "N/A";
                cell_battery.textContent = "N/A";
                cell_port.textContent = "N/A";
                cell_available_shots.textContent = "N/A";
                cell_quality.textContent = "N/A";
                cell_mode.textContent = "N/A";
                cell_iso.textContent = "N/A";
                cell_fstop.textContent = "N/A";
                cell_shutter.textContent = "N/A";

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

                cameras.appendChild(row);
                return;
            }

            for (let info of data.detected)
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

                cell_serial.textContent = info.serial;
                cell_desc.textContent = info.desc;
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

                if (info.connected == "0")
                {
                    svg_connected.src = "/static/cam-disconnected.svg";
                } else
                {
                    svg_connected.src = "/static/cam-connected.svg";
                    cell_battery.textContent = info.batt;
                    cell_port.textContent = info.port;
                    cell_available_shots.textContent = info.num_photos;
                    cell_quality.textContent = info.quality;
                    cell_mode.textContent = info.mode;
                    cell_iso.textContent = info.iso;
                    cell_fstop.textContent = info.fstop;
                    cell_shutter.textContent = info.shutter;
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

                cameras.appendChild(row);
            }
        })
        .catch(error =>
        {
            console.error('Error fetching camera data:', error);
        });
}

/*
 * Camera Renaming Modal.
 */
// === Global Modal Variables (Accessible by all functions) ===
const RENAME_MODAL = document.getElementById('rename_modal');
const RENAME_INPUT = document.getElementById('rename_input');
const SAVE_RENAME_BUTTON = document.getElementById('save_rename');
const CANCEL_RENAME_BUTTON = document.getElementById('cancel_rename');
// RENAME_MODAL_CLOSE_BUTTON will be initialized inside DOMContentLoaded

let CURRENT_EDITABLE_TD = null; // To keep track of which td is being edited

// === Modal Event Handler Functions ===

// Handler for opening the modal when an editable TD is clicked
function handle_editable_td_click()
{
    CURRENT_EDITABLE_TD = this; // 'this' refers to the clicked td
    RENAME_INPUT.value = CURRENT_EDITABLE_TD.textContent.trim(); // Populate input with current text
    RENAME_MODAL.style.display = 'flex'; // Show the modal
    RENAME_INPUT.focus(); // Focus on the input field
}

// Handler for closing the modal via the 'X' button
function handle_rename_modal_close_button_click()
{
    RENAME_MODAL.style.display = 'none';
}

// Handler for canceling the rename operation
function handle_cancel_rename_click()
{
    RENAME_MODAL.style.display = 'none';
}

function send_camera_description_to_backend(serial, description)
{
    fetch('/api/camera/update_description', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    serial: serial,
                    description: description
                })
            })
        .then(response =>
        {
            if (!response.ok)
            {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            return response.json(); // Parse the JSON response from Flask
        })
        .then(data =>
        {
            console.log('Backend response:', data);
            if (data.status === 'success')
            {
                console.log('Camera description updated successfully!');
            } else
            {
                console.error('Backend reported an error:', data.message);
            }
        })
        .catch(error =>
        {
            console.error('Error sending data to backend:', error);
            alert('Failed to save description. Please check your connection and try again.');
        });
}

// Handler for saving the new name
function handle_save_rename_click()
{
    if (CURRENT_EDITABLE_TD)
    {
        const new_description = RENAME_INPUT.value.trim();
        CURRENT_EDITABLE_TD.textContent = new_description;
        console.log("new name: " + new_description);
        send_camera_description_to_backend(
            CURRENT_EDITABLE_TD.dataset.serial,
            new_description
        );
    }
    RENAME_MODAL.style.display = 'none';
}

// Handler for clicking outside the modal
function handle_window_click(event)
{
    if (event.target === RENAME_MODAL)
    {
        RENAME_MODAL.style.display = 'none';
    } else if (event.target === event_selection_modal)
    { // Check for event modal as well
        event_selection_modal.style.display = 'none';
    }
}

// Handler for 'Enter' key press in the input field
function handle_rename_input_keypress(event)
{
    if (event.key === 'Enter')
    {
        SAVE_RENAME_BUTTON.click(); // Simulate a click on the save button
    }
}


/*
 * Event table handling.
 */
// DOM elements for the event table and its modal
const event_table_body = document.querySelector('#event_table tbody');
const event_selection_modal = document.getElementById('event_selection_modal');
const file_list_element = document.getElementById('file_list');

let event_file_name_pre_element = null; // To store the <pre> element for the event filename

// Map to keep track of event rows by their event_id
const event_row_map = new Map(); // Maps event_id -> row_element

// Helper function to get or create a <pre> element within a cell
function get_or_create_pre_element(cell, className) {
    let pre_element = cell.querySelector(`pre.${className}`);
    if (!pre_element) {
        pre_element = document.createElement('pre');
        pre_element.className = className;
        // Clear any existing text content directly on the cell before appending <pre>
        cell.innerHTML = '';
        cell.appendChild(pre_element);
    }
    return pre_element;
}


// --- Function to create or update a row in the event table ---
function update_event_table_row(event_id, event_time, eta = 'N/A') {
    let row = event_row_map.get(event_id);
    let isNewRow = false;

    if (!row) {
        row = event_table_body.insertRow();
        row.id = `event_row_${event_id.replace(/\s+/g, '_').replace(/[^a-zA-Z0-9_]/g, '')}`; // Sanitize ID for DOM
        event_row_map.set(event_id, row);
        isNewRow = true;

        // Ensure all cells are created when the row is new
        row.insertCell(0); // Event ID cell
        row.insertCell(1); // Time cell
        row.insertCell(2); // ETA cell
    }

    // Always ensure the content is updated, and <pre> tags are managed
    row.cells[0].textContent = event_id; // Event ID remains regular text

    // Get or create <pre> elements for Time and ETA cells
    const time_pre = get_or_create_pre_element(row.cells[1], 'event-time-pre');
    const eta_pre = get_or_create_pre_element(row.cells[2], 'event-eta-pre');

    // Set text content for the <pre> elements
    time_pre.textContent = event_time;
    eta_pre.textContent = eta;
}

// --- Function to clear all event data rows (leaving the file selection row) ---
function clear_event_data_rows() {
    // Keep the first row (the "Event File:" row)
    while (event_table_body.rows.length > 1) {
        event_table_body.deleteRow(1); // Always delete the second row until only one remains
    }
    event_row_map.clear(); // Clear the map too
}

// --- Function to populate event table with static details from a loaded file ---
async function populate_static_event_details(filename) {
    clear_event_data_rows(); // Clear previous event data

    // Add a temporary loading row
    const loading_row = event_table_body.insertRow();
    const loading_cell = loading_row.insertCell(0);
    loading_cell.colSpan = 3;
    loading_cell.textContent = `Loading details for ${filename}...`;
    loading_row.id = 'loading_data_row';

    try {
        const response = await fetch(`/api/event_load/${filename}`);

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const event_data = await response.json();

        // Remove the loading row
        const existing_loading_row = document.getElementById('loading_data_row');
        if (existing_loading_row) {
            existing_loading_row.remove();
        }

        // Display 'type' as a static detail
        update_event_table_row('Type', event_data.type || 'N/A', ''); // ETA column will be empty for Type

        // Populate table with initial event IDs, times and ETAs will be filled by update_dynamic_events
        if (event_data.events && Array.isArray(event_data.events)) {
            event_data.events.forEach(event_id => {
                // Initialize with 'N/A' as time and ETA will come from /api/events
                update_event_table_row(event_id, 'N/A', 'Loading...');
            });
        } else {
            console.warn("Event data missing 'events' list or malformed:", event_data);
            update_event_table_row('Events', 'No events defined or format error', '');
        }

        // Trigger an immediate dynamic update to get initial ETAs
        update_dynamic_events();

    } catch (error) {
        console.error(`Error fetching data for ${filename}:`, error);
        // Remove loading row and display error
        const existing_loading_row = document.getElementById('loading_data_row');
        if (existing_loading_row) {
            existing_loading_row.remove();
        }
        // Display an error row at the bottom
        update_event_table_row('Error', `Failed to load ${filename}`, error.message);
    }
}

// --- Function to fetch files from API and populate the list for the modal ---
async function fetch_files_for_modal() {
    file_list_element.innerHTML = '<li>Loading files...</li>';

    try {
        const response = await fetch("/api/event_list");

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const files = await response.json();

        file_list_element.innerHTML = '';

        if (files.length === 0) {
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
                populate_static_event_details(filename); // Load static details
            });
            file_list_element.appendChild(li);
        });

    } catch (error) {
        console.error("Error fetching files for modal:", error);
        file_list_element.innerHTML = `<li>Error loading files: ${error.message}</li>`;
    }
}


// --- Function to periodically update event ETAs from /api/events/ ---
// This function is now updated to expect a map (object) from event_id to [event_time, eta]
async function update_dynamic_events() {
    try {
        const response = await fetch('/api/events');
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        const dynamic_events_map = await response.json(); // Expected: map from event_id to [event_time, eta]

        // Check if the received data is an object (map)
        if (typeof dynamic_events_map === 'object' && dynamic_events_map !== null) {
            // Iterate over the keys (event_ids) in the received map
            for (const event_id in dynamic_events_map) {
                if (dynamic_events_map.hasOwnProperty(event_id)) {
                    const event_info = dynamic_events_map[event_id];

                    // Ensure event_info is an array/list of length 2
                    if (Array.isArray(event_info) && event_info.length === 2) {
                        const [event_time, eta] = event_info;

                        // Call update_event_table_row, which now handles the <pre> tags correctly
                        // This will either create the row if it's new (though it shouldn't be if event_load was successful),
                        // or update the existing row's <pre> content.
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
        // Optionally, you could set ETAs to 'Error' for all currently displayed events
        // that failed to update due to this error, if desired.
    }
}


// === All Initial Setup on DOMContentLoaded ===
document.addEventListener('DOMContentLoaded', function()
{
    // Correctly initialize RENAME_MODAL_CLOSE_BUTTON here, after DOM is loaded.
    const RENAME_MODAL_CLOSE_BUTTON = RENAME_MODAL.querySelector('.close_button');
    const event_modal_close_button = event_selection_modal.querySelector('.close_button');

    // === Camera Renaming Modal Event Listeners ===
    RENAME_MODAL_CLOSE_BUTTON.addEventListener('click', handle_rename_modal_close_button_click);
    CANCEL_RENAME_BUTTON.addEventListener('click', handle_cancel_rename_click);
    SAVE_RENAME_BUTTON.addEventListener('click', handle_save_rename_click);
    RENAME_INPUT.addEventListener('keypress', handle_rename_input_keypress);

    // === Event Table Initial Setup ===
    // Create the first row for file selection
    const file_selection_row = event_table_body.insertRow(0); // Insert at the very top
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
    event_file_name_pre_element = filename_pre; // Store reference

    file_selection_row.insertCell(2); // Empty third cell for alignment

    // Add click listener to the filename cell to open the modal
    filename_cell.addEventListener('click', () => {
        fetch_files_for_modal();
        event_selection_modal.style.display = 'flex';
    });

    // === Event Selection Modal Close Logic ===
    event_modal_close_button.addEventListener('click', () =>
    {
        event_selection_modal.style.display = 'none';
    });

    // Close any modal if user clicks outside of it (unified handler)
    window.addEventListener('click', handle_window_click);

    // === Initial Data Fetches and Intervals ===
    let interval = 1000; // 1 second

    fetch_gps();
    setInterval(fetch_gps, interval);

    fetch_cameras();
    setInterval(fetch_cameras, interval);

    // --- NEW: Periodically fetch and update dynamic events ---
    setInterval(update_dynamic_events, interval); // This will start updating ETAs as soon as the table has rows
});