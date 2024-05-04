#!/bin/bash

# Hardcoded folder name
folder_name="/home/auris/ntp/rebootfiles"

# Start time measurement
start_time=$SECONDS

# Function to get current time in seconds since start time
get_current_time() {
    echo "$((SECONDS - start_time))"
}

# Function to check if a file exists and generate a unique filename
generate_unique_filename() {
    local filename="$1"
    local index=1
    local new_filename="${filename}"

    # Append a number until a unique filename is found
    while [ -e "${new_filename}" ]; do
        new_filename="${filename}_${index}"
        ((index++))
    done

    echo "${new_filename}"
}
sudo truncate -s 0 /var/log/chrony/tracking.log

# Wait for Chrony to synchronize with GPS
while true; do
    # Check if 'chronyc sources' has an asterisk (*) in any line
    if chronyc sources | grep -q "\*"; then
        # Measure elapsed time
        sync_duration=$(get_current_time)
        echo "Synchronized with NTP after $sync_duration seconds"

        # Create base filename using synchronization time
        base_filename="${folder_name}/tracking_ntp128home22_sync_${sync_duration}"

        # Generate unique filename
        filename=$(generate_unique_filename "${base_filename}.log")
        echo "Filename: ${filename}"

        # Wait for additional 10 minutes for synchronization
        sleep 150
	sudo systemctl stop chrony
        # Copy tracking file to specified filename
        sudo cp /var/log/chrony/tracking.log "$filename"

        # Empty original tracking file
        sudo truncate -s 0 /var/log/chrony/tracking.log
	
        break
    else
        # Wait for a moment before checking again
        sleep 1
    fi
done

sudo reboot now -h
