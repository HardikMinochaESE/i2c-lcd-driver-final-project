#!/bin/sh

# Paths for thermal data and LCD interface
THERMAL_PATH="/sys/class/thermal/thermal_zone0/temp"
LCD_PATH="/sys/class/LCD162/lcd_device/lcd_data_feed"


# Function to read temperature and update LCD
update_lcd() {
    # Read temperature from thermal sensor
    $(cat "$THERMAL_PATH" > "$LCD_PATH")
}

# Main loop
main() {
    
    # Infinite loop
    while true; do
        update_lcd
        sleep 5  # Update every 5 seconds
    done
}

# Start the main loop
main &  # Run in background

