const apiUrl = '/api'; // Base URL for API requests

// Function to fetch sensor data and update the UI
async function fetchSensorData() {
    try {
        // Show the loading spinners
        document.querySelectorAll('.loading-spinner').forEach(spinner => {
            spinner.classList.add('visible');
        });
        
        const response = await fetch(`${apiUrl}/sensor-data`, {
            credentials: 'same-origin',
            cache: 'no-cache'
        });
        
        const data = await response.json();
        updateUI(data);
        
        // Hide the loading spinners after data is loaded
        document.querySelectorAll('.loading-spinner').forEach(spinner => {
            spinner.classList.remove('visible');
        });
    } catch (error) {
        console.error('Error fetching sensor data:', error);
        // Hide spinners on error
        document.querySelectorAll('.loading-spinner').forEach(spinner => {
            spinner.classList.remove('visible');
        });
    }
}

// Function to force a sensor reading and update the UI
async function forceSensorReadNow() {
    try {
        console.log("Sending read-now request...");
        
        // Show the loading spinners
        document.querySelectorAll('.loading-spinner').forEach(spinner => {
            spinner.classList.add('visible');
        });
        
        // First request new readings from the sensor
        const response = await fetch(`${apiUrl}/read-now`, { 
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            credentials: 'same-origin',
            cache: 'no-cache'
        });
        
        console.log("Response status:", response.status);
        
        if (response.ok) {
            console.log("Sensor reading requested successfully");
            // Then fetch the newly updated data
            await fetchSensorData();
        } else {
            console.error("Error response:", response.status, response.statusText);
            document.querySelectorAll('.loading-spinner').forEach(spinner => {
                spinner.classList.remove('visible');
            });
        }
    } catch (error) {
        console.error('Error triggering sensor update:', error);
        // Hide spinners on error
        document.querySelectorAll('.loading-spinner').forEach(spinner => {
            spinner.classList.remove('visible');
        });
    }
}

// Function to update the UI with sensor data
function updateUI(data) {
    document.getElementById('temperature').innerText = `${data.temperature} °C`;
    document.getElementById('humidity').innerText = `${data.humidity} %`;
    document.getElementById('pressure').innerText = `${data.pressure} hPa`;
    document.getElementById('heat-index').innerText = `${data.heat_index} °C`;
    document.getElementById('soil-moisture').innerText = `${data.soil_moisture} %`;
    document.getElementById('device-name').innerText = data.deviceName;
    document.getElementById('current-time').innerText = data.time_str;
    
    // Update relay status indicators if they exist
    if (data.relays && Array.isArray(data.relays)) {
        for (let i = 0; i < data.relays.length; i++) {
            const relayElement = document.getElementById(`relay${i+1}-status`);
            if (relayElement) {
                relayElement.innerText = data.relays[i] ? 'ON' : 'OFF';
                relayElement.className = data.relays[i] ? 'status-on' : 'status-off';
            }
        }
    }
    
    // Update watering status if available
    const wateringStatus = document.getElementById('watering-status');
    if (wateringStatus) {
        if (data.watering_active) {
            wateringStatus.innerText = `Active (${data.watering_remaining}s remaining)`;
            wateringStatus.className = 'status-on';
        } else {
            wateringStatus.innerText = 'Idle';
            wateringStatus.className = 'status-off';
        }
    }
}

// Function to control the relay
async function controlRelay(relayId, state) {
    try {
        const response = await fetch(`${apiUrl}/relay`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ id: relayId-1, state: state ? 1 : 0 }),
            credentials: 'same-origin'
        });
        const result = await response.json();
        console.log('Relay control response:', result);
        
        // Refresh data to show updated status
        fetchSensorData();
    } catch (error) {
        console.error('Error controlling relay:', error);
    }
}

// Function to trigger watering
async function waterNow() {
    try {
        console.log("Sending water-now request...");
        const response = await fetch(`${apiUrl}/water-now`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            credentials: 'same-origin'
        });
        
        console.log("Water now response:", response.status);
        
        // Refresh data to show updated status
        fetchSensorData();
    } catch (error) {
        console.error('Error starting watering:', error);
    }
}

// Set up event listeners when the document is loaded
document.addEventListener('DOMContentLoaded', function() {
    console.log("Initializing Garden Monitor interface");
    
    // Event listeners for relay toggle switches
    const relay1Toggle = document.getElementById('relay1-toggle');
    if (relay1Toggle) {
        relay1Toggle.addEventListener('click', () => {
            const state = relay1Toggle.checked;
            controlRelay(1, state);
        });
    }
    
    const relay2Toggle = document.getElementById('relay2-toggle');
    if (relay2Toggle) {
        relay2Toggle.addEventListener('click', () => {
            const state = relay2Toggle.checked;
            controlRelay(2, state);
        });
    }
    
    const relay3Toggle = document.getElementById('relay3-toggle');
    if (relay3Toggle) {
        relay3Toggle.addEventListener('click', () => {
            const state = relay3Toggle.checked;
            controlRelay(3, state);
        });
    }
    
    const relay4Toggle = document.getElementById('relay4-toggle');
    if (relay4Toggle) {
        relay4Toggle.addEventListener('click', () => {
            const state = relay4Toggle.checked;
            controlRelay(4, state);
        });
    }
    
    // Event listener for "Read Now" button
    const readNowButton = document.getElementById('read-now');
    if (readNowButton) {
        console.log("Adding Read Now button listener");
        readNowButton.addEventListener('click', forceSensorReadNow);
    } else {
        console.warn("Read Now button not found");
    }
    
    // Event listener for "Water Now" button
    const waterNowButton = document.getElementById('water-now');
    if (waterNowButton) {
        waterNowButton.addEventListener('click', waterNow);
    }
    
    // Initial fetch of sensor data
    fetchSensorData();
    
    // Fetch sensor data automatically every 5 seconds
    const autoRefreshInterval = 5000; // Check every 5 seconds
    setInterval(fetchSensorData, autoRefreshInterval);
});