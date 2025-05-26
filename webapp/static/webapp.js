// Library stuff.
function sprintf()
{
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

// GPS.
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
        .then(data => {
            //console.log(data);

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
            }
            else if (data.mode == "3D FIX")
            {
                gps_connection.src = "/static/gps-connected.svg";
            }

            gps_mode.textContent = data.mode;
            gps_mode_time.textContent = data.mode_time;
            gps_sats.textContent = sprintf("%d/%d", data.sats_used, data.sats_seen);
            gps_time.textContent = data.time;
            // Using 4 decimal places for about 10 meteters of precision.
            //     https://gis.stackexchange.com/a/8674
            gps_latitude.textContent = data.lat.toFixed(4);
            gps_longitude.textContent = data.long.toFixed(4);
            gps_altitude.textContent = data.altitude.toFixed(1);
        })
        .catch(error => {
          console.error('Error fetching data:', error);
        });
}

// Cameras.
const cameras = document.getElementById("cameras").tBodies[0];

function fetch_cameras()
{
    fetch('/api/cameras')
        .then(response => response.json())
        .then(data => {
            //console.log(data);

            // Erase.
            cameras.innerHTML = "";

            if (data.num_cameras == 0)
            {
                const row = document.createElement('tr');
                const cell_connected       = document.createElement('td');
                const cell_serial          = document.createElement('td');
                const cell_desc            = document.createElement('td');
                const cell_battery         = document.createElement('td');
                const cell_port            = document.createElement('td');
                const cell_available_shots = document.createElement('td');
                const cell_quality         = document.createElement('td');
                const cell_mode            = document.createElement('td');
                const cell_iso             = document.createElement('td');
                const cell_fstop           = document.createElement('td');
                const cell_shutter         = document.createElement('td');

                cell_connected       .textContent = "N/A";
                cell_serial          .textContent = "N/A";
                cell_desc            .textContent = "N/A";
                cell_battery         .textContent = "N/A";
                cell_port            .textContent = "N/A";
                cell_available_shots .textContent = "N/A";
                cell_quality         .textContent = "N/A";
                cell_mode            .textContent = "N/A";
                cell_iso             .textContent = "N/A";
                cell_fstop           .textContent = "N/A";
                cell_shutter         .textContent = "N/A";

                row.appendChild(cell_connected       );
                row.appendChild(cell_serial          );
                row.appendChild(cell_desc            );
                row.appendChild(cell_battery         );
                row.appendChild(cell_port            );
                row.appendChild(cell_available_shots );
                row.appendChild(cell_quality         );
                row.appendChild(cell_mode            );
                row.appendChild(cell_iso             );
                row.appendChild(cell_fstop           );
                row.appendChild(cell_shutter         );

                cameras.appendChild(row);
                return;
            }

            for (let info of data.detected)
            {
                const row = document.createElement('tr');
                const cell_connected       = document.createElement('td');
                const cell_serial          = document.createElement('td');
                const cell_desc            = document.createElement('td');
                const cell_battery         = document.createElement('td');
                const cell_port            = document.createElement('td');
                const cell_available_shots = document.createElement('td');
                const cell_quality         = document.createElement('td');
                const cell_mode            = document.createElement('td');
                const cell_iso             = document.createElement('td');
                const cell_fstop           = document.createElement('td');
                const cell_shutter         = document.createElement('td');

                cell_serial          .textContent = info.serial;
                cell_desc            .textContent = info.desc;
                cell_battery         .textContent = "N/A";
                cell_port            .textContent = info.port;
                cell_available_shots .textContent = "N/A";
                cell_quality         .textContent = "N/A";
                cell_mode            .textContent = "N/A";
                cell_iso             .textContent = "N/A";
                cell_fstop           .textContent = "N/A";
                cell_shutter         .textContent = "N/A";

                /*
                 * Add the renameable class to the description element.
                 */
                cell_desc.classList.add('editable_td');
                cell_desc.addEventListener('click', handle_editable_td_click);
                // Store the serial number in a data attribute
                cell_desc.dataset.serial = info.serial;

                /*
                 * Image object for visual status of camera connection.
                 */
                const svg_connected = document.createElement('img');
                svg_connected.width = "70";
                svg_connected.height = "50";

                if (info.connected == "0")
                {
                    svg_connected.src = "/static/cam-disconnected.svg";
                }
                else
                {
                    svg_connected.src = "/static/cam-connected.svg";
                    cell_battery         .textContent = info.batt;
                    cell_port            .textContent = info.port;
                    cell_available_shots .textContent = info.num_photos;
                    cell_quality         .textContent = info.quality;
                    cell_mode            .textContent = info.mode;
                    cell_iso             .textContent = info.iso;
                    cell_fstop           .textContent = info.fstop;
                    cell_shutter         .textContent = info.shutter;
                }

                cell_connected.appendChild(svg_connected);

                row.appendChild(cell_connected       );
                row.appendChild(cell_serial          );
                row.appendChild(cell_desc            );
                row.appendChild(cell_battery         );
                row.appendChild(cell_port            );
                row.appendChild(cell_available_shots );
                row.appendChild(cell_quality         );
                row.appendChild(cell_mode            );
                row.appendChild(cell_iso             );
                row.appendChild(cell_fstop           );
                row.appendChild(cell_shutter         );

                cameras.appendChild(row);
            }
        })
        .catch(error => {
          console.error('Error fetching data:', error);
        });
}

/*
 * Editing camera names.
 */
// === Global Modal Variables (Accessible by all functions) ===
const RENAME_MODAL = document.getElementById('rename_modal');
const RENAME_INPUT = document.getElementById('rename_input');
const SAVE_RENAME_BUTTON = document.getElementById('save_rename');
const CANCEL_RENAME_BUTTON = document.getElementById('cancel_rename');
const CLOSE_BUTTON = document.querySelector('.close_button');

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
function handle_close_button_click()
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
            // Convert your JavaScript object to a JSON string.
            serial: serial,
            description: description
        })
    })
    .then(response => {
        if (!response.ok) {
            // If response status is not 2xx, throw an error
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        return response.json(); // Parse the JSON response from Flask
    })
    .then(data => {
        console.log('Backend response:', data);
        if (data.status === 'success') {
            console.log('Camera description updated successfully!');
            // Optionally, you could show a temporary success message to the user
        } else {
            console.error('Backend reported an error:', data.message);
            // Optionally, show a temporary error message to the user
            // And revert the text on the TD if needed (though immediate update is generally preferred)
        }
    })
    .catch(error => {
        console.error('Error sending data to backend:', error);
        // Inform the user about the network error
        alert('Failed to save description. Please check your connection and try again.');
        // Revert the text on the TD if the update failed
        // CURRENT_EDITABLE_TD.textContent = oldDescription; // You'd need to store oldDescription
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
        )
    }
    RENAME_MODAL.style.display = 'none';
}

// Handler for clicking outside the modal
function handle_window_click(event)
{
    if (event.target === RENAME_MODAL)
    {
        RENAME_MODAL.style.display = 'none';
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

// === Initial Setup for Modal Controls (Not the TDs) ===
// These listeners only need to be attached once, regardless of dynamic TD creation.
document.addEventListener('DOMContentLoaded', function()
{
    // Attach event listeners to the modal controls
    CLOSE_BUTTON.addEventListener('click', handle_close_button_click);
    CANCEL_RENAME_BUTTON.addEventListener('click', handle_cancel_rename_click);
    SAVE_RENAME_BUTTON.addEventListener('click', handle_save_rename_click);
    RENAME_INPUT.addEventListener('keypress', handle_rename_input_keypress);
    window.addEventListener('click', handle_window_click);
});


// Run once and schedule every 1 seconds.
let interval = 1000;

fetch_gps();
setInterval(fetch_gps, interval);

fetch_cameras();
setInterval(fetch_cameras, interval);