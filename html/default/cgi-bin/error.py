#!/usr/bin/env python3
import os
import sys

# This script will generate an error
print("Content-Type: text/html")
print()  # Empty line to separate headers from body

print("<html>")
print("<head><title>Error CGI</title></head>")
print("<body>")
print("<h1>Error Test</h1>")

# Generate a division by zero error
try:
    result = 1 / 0
    print(f"Result: {result}")
except Exception as e:
    print(f"<p>Error: {e}</p>")
    # Exit with an error code
    sys.exit(1)

print("</body>")
print("</html>")
