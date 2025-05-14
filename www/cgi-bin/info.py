#!/usr/bin/env python3
import os
import sys

# Print the HTTP headers
print("Content-Type: text/html")
print()  # Empty line to separate headers from body

# Print the HTML body
print("<html>")
print("<head><title>CGI Info</title></head>")
print("<body>")
print("<h1>CGI Environment Information</h1>")

# Print environment variables
print("<h2>Environment Variables:</h2>")
print("<table border='1'>")
print("<tr><th>Variable</th><th>Value</th></tr>")
for key, value in sorted(os.environ.items()):
    print(f"<tr><td>{key}</td><td>{value}</td></tr>")
print("</table>")

# Print query parameters if any
query_string = os.environ.get("QUERY_STRING", "")
if query_string:
    print("<h2>Query Parameters:</h2>")
    print("<table border='1'>")
    print("<tr><th>Parameter</th><th>Value</th></tr>")
    for param in query_string.split("&"):
        if "=" in param:
            key, value = param.split("=", 1)
            print(f"<tr><td>{key}</td><td>{value}</td></tr>")
        else:
            print(f"<tr><td colspan='2'>{param}</td></tr>")
    print("</table>")

# Print current working directory
print("<h2>Current Working Directory:</h2>")
print(f"<p>{os.getcwd()}</p>")

print("</body>")
print("</html>")
