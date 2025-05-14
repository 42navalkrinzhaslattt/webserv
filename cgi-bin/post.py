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
print("<head><title>POST CGI</title></head>")
print("<body>")
print("<h1>POST Data Received!</h1>")

# Print the POST data
print("<h2>POST Data:</h2>")
print("<pre>")
print(post_data)
print("</pre>")

# Print environment variables
print("<h2>Environment Variables:</h2>")
print("<ul>")
for key, value in sorted(os.environ.items()):
    print(f"<li><strong>{key}</strong>: {value}</li>")
print("</ul>")

print("</body>")
print("</html>")
