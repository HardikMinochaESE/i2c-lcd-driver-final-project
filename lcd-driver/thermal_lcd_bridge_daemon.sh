#!/bin/sh

# Paths for thermal data and LCD interface
THERMAL_PATH="/sys/class/thermal/thermal_zone0/temp"
LCD_PATH="/sys/class/LCD162/lcd_device/lcd_data_feed"

# Function to check if files exist
check_paths() {
    if [ ! -f "$THERMAL_PATH" ]; then
        echo "Error: Thermal sensor file not found at $THERMAL_PATH"
        exit 1
    fi
    if [ ! -f "$LCD_PATH" ]; then
        echo "Error: LCD interface file not found at $LCD_PATH"
        exit 1
    fi
}

# Function to read temperature and update LCD
update_lcd() {
    # Read temperature from thermal sensor
    temp=$(cat "$THERMAL_PATH")
    if [ $? -ne 0 ]; then
        echo "Error: Failed to read temperature"
        return 1
    fi

    # Write to LCD
    echo "$temp" > "$LCD_PATH"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to write to LCD"
        return 1
    fi
}

# Main loop
main() {
    echo "Starting thermal LCD bridge daemon..."
    
    # Check if paths exist
    check_paths
    
    # Infinite loop
    while true; do
        update_lcd
        sleep 5  # Update every 5 seconds
    done

}

# Start the main loop
main

