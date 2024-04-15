#!/bin/bash

pps_filename="pps_offset_1.txt"
loop_count=5

while true; do
    # Start chrony
    sudo systemctl start chrony
    
    # Wait for 2 minutes
    sleep 2m
    
    # Stop chrony
    sudo systemctl stop chronyd
    
    # Run ppstest for 15 minutes and save output to file
    sudo ppstest /dev/pps0 | tee "$pps_filename" &
    ppstest_pid=$!
    sleep 15m
    sudo kill $ppstest_pid
    
    # Increment filename
    loop_count=$((loop_count + 1))
    pps_filename="pps_output_$loop_count.txt"
    
  done

