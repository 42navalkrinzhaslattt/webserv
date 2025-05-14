#!/usr/bin/env python3
import os
import sys

# This script will output invalid HTTP headers
print("Invalid-Header: This is not a valid HTTP header")
print("Content-Type: text/html")
print("Invalid-Header-2: Another invalid header")
print()  # Empty line to separate headers from body

print("<html>")
print("<head><title>Invalid Headers CGI</title></head>")
print("<body>")
print("<h1>Invalid Headers Test</h1>")
print("<p>This script outputs invalid HTTP headers.</p>")
print("</body>")
print("</html>")
