#!/usr/bin/env python3
import os
import sys

# Print the HTTP headers
print("Content-Type: text/html")
print()  # Empty line to separate headers from body

# Print the HTML body
print("<html>")
print("<head><title>File Access Test</title></head>")
print("<body>")
print("<h1>File Access Test</h1>")

# Print current working directory
print("<h2>Current Working Directory:</h2>")
print(f"<pre>{os.getcwd()}</pre>")

# List files in the current directory
print("<h2>Files in Current Directory:</h2>")
print("<ul>")
try:
    for file in sorted(os.listdir(".")):
        print(f"<li>{file}</li>")
except Exception as e:
    print(f"<li>Error listing files: {e}</li>")
print("</ul>")

# Create a test file
test_file_path = "test_file.txt"
print("<h2>Creating a Test File:</h2>")
try:
    with open(test_file_path, "w") as f:
        f.write("This is a test file created by the CGI script.\n")
        f.write("It demonstrates file access in the CGI script's directory.\n")
    print(f"<p>Successfully created file: {test_file_path}</p>")
except Exception as e:
    print(f"<p>Error creating file: {e}</p>")

# Read the test file
print("<h2>Reading the Test File:</h2>")
print("<pre>")
try:
    with open(test_file_path, "r") as f:
        print(f.read())
except Exception as e:
    print(f"Error reading file: {e}")
print("</pre>")

print("</body>")
print("</html>")
