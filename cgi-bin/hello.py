#!/usr/bin/env python3
import os
import sys

# Print the HTTP headers
print("Content-Type: text/html")
print()  # Empty line to separate headers from body

# Print the HTML body
print("<html>")
print("<head><title>Hello CGI</title></head>")
print("<body>")
print("<h1>Hello from CGI!</h1>")
print("<h2>Environment Variables:</h2>")
print("<ul>")
for key, value in sorted(os.environ.items()):
    print(f"<li><strong>{key}</strong>: {value}</li>")
print("</ul>")

# Print query parameters if any
query_string = os.environ.get("QUERY_STRING", "")
if query_string:
    print("<h2>Query Parameters:</h2>")
    print("<ul>")
    for param in query_string.split("&"):
        if "=" in param:
            key, value = param.split("=", 1)
            print(f"<li><strong>{key}</strong>: {value}</li>")
        else:
            print(f"<li>{param}</li>")
    print("</ul>")

print("</body>")
print("</html>")
