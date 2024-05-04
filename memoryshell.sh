#!/bin/bash

# Check if filename argument is provided
if [ $# -eq 0 ]; then
    echo "Usage: $0 <filename_prefix>"
    exit 1
fi

filename_prefix=$1

poll_values=(0 1 2 3 4 5)

# Function to increase the poll argument in chrony.conf
increase_poll() {
    local new_poll_value="$1"
    sudo sed -i 's/\(^refclock PPS \/dev\/pps0 lock GPS poll \)[0-9]\+/\1'"$new_poll_value"'/' /etc/chrony/chrony.conf
    sudo systemctl restart chronyd
}

# Function to start memory_receive_rt for 55 minutes and perform cleanup
start_memory_receive_rt() {
    sleep 5m # Adjust to start exactly at the beginning of the hour
    sudo ./memory_receive_rt | tee "${filename_prefix}_receive_poll${1}.txt" &
    memory_receive_rt_pid=$!
    sleep 54m # Run for 55 minutes
    sudo kill $memory_receive_rt_pid # Stop memory_receive_rt 1 minute before the full hour
    sudo cp /var/log/chrony/tracking.log receivelog_poll${1}.log
    sudo truncate -s 0 /var/log/chrony/tracking.log
}

# Run four loops
for ((i = 0; i < ${#poll_values[@]}; i++)); do
    # Wait until the full hour
    current_minute=$(date +%M)
    until [ "$(date +%M)" -eq "19" ]; do
        sleep 1
    done

    increase_poll "${poll_values[i]}"
    start_memory_receive_rt "${poll_values[i]}"
done
