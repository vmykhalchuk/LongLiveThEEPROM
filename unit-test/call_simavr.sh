#!/bin/bash

# Configuration
HEX_FILE="$1"
MCU="atmega328p"
FREQ="16000000"
LOG_FILE="$2"
EXIT_STR="$3"
FAIL_STR="$4"

terminate_sim() {
  local message=$1
  local exit_code=$2
  local count=0
  local warmTimeout=5
  local timeout=30

  echo "$message"
  
  # 1. Check if self terminates
  count=0
  while [ $count -lt $warmTimeout ]; do
    sleep 1
    if ! kill -0 "$SIM_PID" 2>/dev/null; then
      exit "$exit_code"
    fi
    ((count++))
  done

  # 2. Asking to terminate gracefully
  echo "Sending SIGTERM to SIMAVR: $SIM_PID..."
  kill "$SIM_PID"  # Default signal is SIGTERM (graceful)

  count=0
  while [ $count -lt $timeout ]; do
    sleep 1
    if ! kill -0 "$SIM_PID" 2>/dev/null; then
      exit "$exit_code"
    fi
    ((count++))
    echo -n "."
  done

  # 3. Still alive => force kill
  echo -e "\nTimeout reached! Sending SIGKILL to PID $SIM_PID..."
  kill -9 "$SIM_PID"

  exit $exit_code
}

# 1. Start simavr in the background
simavr -m $MCU -f $FREQ "$HEX_FILE" > "$LOG_FILE" 2>&1 &
SIM_PID=$!

echo "Simulation started (PID: $SIM_PID)..."

# 2. Tail the log file and wait for success or failure strings
# We use a process substitution approach to avoid subshell scoping issues if needed, 
# but for a simple kill-and-exit, the following logic works:

while read -r LINE; do

  # Check for Failure
  if [[ "$LINE" == *"$FAIL_STR"* ]]; then
    echo "FAILURE detected: $FAIL_STR. Terminating."
    terminate_sim "FAILURE detected. Shutting down." 1
  fi

  # Check for Success (the original EXIT_STR)
  if [[ "$LINE" == *"$EXIT_STR"* ]]; then
    terminate_sim "Success!" 0
  fi

done < <(tail -f --pid=$SIM_PID "$LOG_FILE")