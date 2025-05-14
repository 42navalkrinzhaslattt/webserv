#!/usr/bin/env python3
import os
import sys

# Print the HTTP headers
print("Content-Type: text/html")
print()  # Empty line to separate headers from body

# Print the beginning of the HTML body
print("<html>")
print("<head><title>Error Test</title></head>")
print("<body>")
print("<h1>Error Test</h1>")
print("<p>This script will generate an error.</p>")

# Generate a division by zero error
print("<h2>Division by Zero Error:</h2>")
try:
    result = 1 / 0
    print(f"<p>Result: {result}</p>")
except Exception as e:
    print(f"<p>Caught error: {e}</p>")
    # Uncomment to exit with an error code
    # sys.exit(1)

# Generate a file not found error
print("<h2>File Not Found Error:</h2>")
try:
    with open("non_existent_file.txt", "r") as f:
        content = f.read()
        print(f"<p>File content: {content}</p>")
except Exception as e:
    print(f"<p>Caught error: {e}</p>")

# Generate a syntax error
print("<h2>Syntax Error:</h2>")
try:
    exec("print('Hello, World!'")
except Exception as e:
    print(f"<p>Caught error: {e}</p>")

print("<p>End of script.</p>")
print("</body>")
print("</html>")
