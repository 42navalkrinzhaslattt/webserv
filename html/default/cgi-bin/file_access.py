#!/usr/bin/env python3
import os
import sys

# Print the HTTP headers
print("Content-Type: text/html")
print()  # Empty line to separate headers from body

# Print the HTML body
print("<html>")
print("<head><title>File Access CGI</title></head>")
print("<body>")
print("<h1>File Access Test</h1>")

# Print current working directory
print("<h2>Current Working Directory:</h2>")
print("<pre>")
print(os.getcwd())
print("</pre>")

# List files in the current directory
print("<h2>Files in Current Directory:</h2>")
print("<ul>")
try:
    for file in sorted(os.listdir(".")):
        print(f"<li>{file}</li>")
except Exception as e:
    print(f"<li>Error listing files: {e}</li>")
print("</ul>")

# Try to read a test file in the same directory
print("<h2>Reading test.txt:</h2>")
print("<pre>")
try:
    with open("test.txt", "r") as f:
        print(f.read())
except Exception as e:
    print(f"Error reading test.txt: {e}")
print("</pre>")

print("</body>")
print("</html>")
