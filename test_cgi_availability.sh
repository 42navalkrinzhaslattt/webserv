#!/bin/bash

# Number of requests to make
NUM_REQUESTS=100

# URL to test
URL="http://localhost:8090/cgi-bin/test.py"

# Counter for successful requests
SUCCESS=0

# Make the requests
for ((i=1; i<=$NUM_REQUESTS; i++)); do
    # Make the request and check the status code
    STATUS=$(curl -s -o /dev/null -w "%{http_code}" $URL)
    
    # If the status code is 200, increment the success counter
    if [ "$STATUS" -eq 200 ]; then
        SUCCESS=$((SUCCESS + 1))
    fi
    
    # Print progress
    if [ $((i % 10)) -eq 0 ]; then
        echo "Completed $i requests..."
    fi
done

# Calculate availability percentage
AVAILABILITY=$(echo "scale=2; $SUCCESS * 100 / $NUM_REQUESTS" | bc)

echo "Total requests: $NUM_REQUESTS"
echo "Successful requests: $SUCCESS"
echo "Availability: $AVAILABILITY%"

# Check if availability is at least 99.5%
if (( $(echo "$AVAILABILITY >= 99.5" | bc -l) )); then
    echo "Availability is at least 99.5%"
else
    echo "Availability is less than 99.5%"
fi
