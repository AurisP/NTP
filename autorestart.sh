#!/bin/bash

# Function to perform cleanup tasks
cleanup() {
    echo "Cleaning up before exit..."
    # Kill the program and all its threads
    sudo pkill -TERM -P $program_pid
    # Exit the script
    exit 0
}

# Trap the Ctrl+C signal
trap cleanup SIGINT

# Initialize filename counter
filename_counter=1

# Run the loop indefinitely
while true; do
    # Set program file name and run time in seconds
    program_file="memory_receive_rt"
    output_file="test_rasp12_gps4_memshell.txt"
    run_time=100  # 300 seconds = 5 minutes, adjust as needed

    # Start the program, redirect output to a file using a subshell
    (sudo ./$program_file | tee -a "$output_file") &

    # Capture the PID of the program
    program_pid=$!

    # Wait for the specified run time
    sleep $run_time

    # Kill the program and all its threads
    sudo pkill -TERM -P $program_pid

    # Increment the filename counter
    ((filename_counter++))

    # Restart the Raspberry Pi
    
done

