#!/usr/bin/env python3
import os
import sys
import time

# Print the HTTP headers
print("Content-Type: text/html")
print()  # Empty line to separate headers from body

# Print the beginning of the HTML body
print("<html>")
print("<head><title>Infinite Loop Test</title></head>")
print("<body>")
print("<h1>Infinite Loop Test</h1>")
print("<p>This script will run in a loop for 10 seconds.</p>")

# Flush the output buffer
sys.stdout.flush()

# Run in a loop for 10 seconds
start_time = time.time()
count = 0
while True:
    count += 1
    # Print a message every 1000 iterations
    if count % 1000 == 0:
        print(f"<p>Iteration {count}, elapsed time: {time.time() - start_time:.2f} seconds</p>")
        sys.stdout.flush()
    
    # Break after 10 seconds to avoid hanging the server indefinitely
    if time.time() - start_time > 10:
        break

print("<p>Loop finished after 10 seconds.</p>")
print("</body>")
print("</html>")
