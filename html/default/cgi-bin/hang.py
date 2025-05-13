#!/usr/bin/env python3
import os
import sys
import time

# Print the CGI header
print("Content-Type: text/html")
print()  # Empty line to separate headers from body

# Print the HTML body
print("<html>")
print("<head><title>CGI Hang Test</title></head>")
print("<body>")
print("<h1>CGI Hang Test</h1>")
print("<p>This script will hang indefinitely.</p>")
sys.stdout.flush()  # Make sure the output is sent

# Hang indefinitely
while True:
    time.sleep(60)
