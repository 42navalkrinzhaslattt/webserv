#!/usr/bin/env python3
import os
import sys

# Print the CGI header
print("Content-Type: text/html")
print()  # Empty line to separate headers from body

# Print the HTML body
print("<html>")
print("<head><title>CGI Crash Test</title></head>")
print("<body>")
print("<h1>CGI Crash Test</h1>")
print("<p>This script will crash intentionally.</p>")

# Intentionally crash the script
sys.exit(1)
