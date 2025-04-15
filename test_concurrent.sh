#!/bin/bash

# Function to make request and measure time
make_request() {
    local id=$1
    echo "Request $id starting..."
    time curl -s "http://localhost:8080/test$id" > /dev/null
    echo "Request $id completed"
}

# Make 10 concurrent requests
for i in {1..10}; do
    make_request $i &
done

# Wait for all background processes to complete
wait
echo "All requests completed"
