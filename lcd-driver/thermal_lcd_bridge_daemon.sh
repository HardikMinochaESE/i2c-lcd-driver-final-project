#!/bin/sh

# Paths for thermal data and LCD interface
THERMAL_PATH="/sys/class/thermal/thermal_zone0/temp"
LCD_PATH="/sys/class/LCD162/lcd_device/lcd_data_feed"
PWM_PATH="/sys/class/LCD162/lcd_device/pwm_duty_cycle"
PID_FILE="/var/run/thermal_bridge_pid"

# Temperature range for fan control (in millidegrees Celsius)
MIN_TEMP=35000
MAX_TEMP=60000

# Function to check if files exist
check_paths() {
    if [ ! -f "$THERMAL_PATH" ]; then
        echo "Error: Thermal path not found"
        exit 1
    fi
    if [ ! -f "$LCD_PATH" ]; then
        echo "Error: LCD path not found"
        exit 1
    fi
    if [ ! -f "$PWM_PATH" ]; then
        echo "Error: PWM path not found"
        exit 1
    fi
}

# Function to map temperature to fan speed (0-9)
# Linear mapping from MIN_TEMP to MAX_TEMP
map_temp_to_speed() {
    local temp=$1
    local speed
    
    # Ensure temperature is within bounds
    if [ "$temp" -lt "$MIN_TEMP" ]; then
        temp=$MIN_TEMP
    elif [ "$temp" -gt "$MAX_TEMP" ]; then
        temp=$MAX_TEMP
    fi
    
    # Calculate speed (0-9) using linear mapping
    # speed = ((temp - MIN_TEMP) * 9) / (MAX_TEMP - MIN_TEMP)
    speed=$(( (temp - MIN_TEMP) * 9 / (MAX_TEMP - MIN_TEMP) ))
    echo "$speed"
}

# Function to read temperature and update LCD and fan
update_system() {
    # Read temperature from thermal sensor
    temp=$(cat "$THERMAL_PATH")
    if [ $? -ne 0 ]; then
        return 1
    fi

    # Write to LCD
    echo "$temp" > "$LCD_PATH"
    if [ $? -ne 0 ]; then
        return 1
    fi

    # Calculate and set fan speed
    speed=$(map_temp_to_speed "$temp")
    echo "$speed" > "$PWM_PATH"
    if [ $? -ne 0 ]; then
        return 1
    fi
}

# Main loop
main() {
    # Check if paths exist
    check_paths
    
    # Infinite loop
    while true; do
        update_system
        sleep 1  # Update every 1 second
    done
}

# Start the main loop in background and capture its PID
main &
echo $! > "$PID_FILE" 