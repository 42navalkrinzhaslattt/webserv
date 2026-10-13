#!/bin/bash

# Default port is 8080
PORT=${1:-8080}

# Find process using the port
PID=$(lsof -ti :$PORT)

if [ -z "$PID" ]; then
    echo "No process found running on port $PORT"
    exit 0
fi

echo "Found process(es) running on port $PORT:"
lsof -i :$PORT

# Try graceful shutdown first
echo "Attempting graceful shutdown..."
kill $PID 2>/dev/null

# Wait a moment to see if it worked
sleep 1

# Check if process still exists
if kill -0 $PID 2>/dev/null; then
    echo "Process still running. Forcing termination..."
    kill -9 $PID 2>/dev/null
    echo "Process forcefully terminated."
else
    echo "Process terminated gracefully."
fi

# Verify port is now free
if lsof -i :$PORT >/dev/null 2>&1; then
    echo "Warning: Port $PORT is still in use!"
else
    echo "Port $PORT is now free."
fi
