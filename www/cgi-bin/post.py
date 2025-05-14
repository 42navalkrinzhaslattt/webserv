#!/usr/bin/env python3
import os
import sys

# Get the content length
content_length = int(os.environ.get("CONTENT_LENGTH", 0))

# Read the POST data
post_data = sys.stdin.read(content_length) if content_length > 0 else ""

# Print the HTTP headers
print("Content-Type: text/html")
print()  # Empty line to separate headers from body

# Print the HTML body
print("<html>")
print("<head><title>POST Data</title></head>")
print("<body>")
print("<h1>POST Data Received</h1>")

# Print the POST data
print("<h2>POST Data:</h2>")
print("<pre>")
print(post_data)
print("</pre>")

# Parse and display the POST data in a table
if post_data:
    print("<h2>Parsed POST Data:</h2>")
    print("<table border='1'>")
    print("<tr><th>Parameter</th><th>Value</th></tr>")
    for param in post_data.split("&"):
        if "=" in param:
            key, value = param.split("=", 1)
            print(f"<tr><td>{key}</td><td>{value}</td></tr>")
        else:
            print(f"<tr><td colspan='2'>{param}</td></tr>")
    print("</table>")

# Print environment variables
print("<h2>Environment Variables:</h2>")
print("<table border='1'>")
print("<tr><th>Variable</th><th>Value</th></tr>")
for key, value in sorted(os.environ.items()):
    print(f"<tr><td>{key}</td><td>{value}</td></tr>")
print("</table>")

print("</body>")
print("</html>")
