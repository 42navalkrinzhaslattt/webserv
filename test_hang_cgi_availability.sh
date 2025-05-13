#!/bin/bash

# Number of requests to make
NUM_REQUESTS=10

# URL to test
URL="http://localhost:8090/cgi-bin/hang.py"

# Counter for successful requests
SUCCESS=0

# Make the requests
for ((i=1; i<=$NUM_REQUESTS; i++)); do
    # Make the request with a timeout and check the status code
    STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 60 $URL)
    
    # If the status code is 504, increment the success counter
    # (We expect a 504 Gateway Timeout for a hanging CGI script)
    if [ "$STATUS" -eq 504 ]; then
        SUCCESS=$((SUCCESS + 1))
    fi
    
    echo "Request $i: Status code $STATUS"
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
