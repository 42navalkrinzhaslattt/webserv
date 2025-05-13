#!/usr/bin/env python3

import os
import sys
import datetime

# Print HTTP headers
print("Content-Type: text/html")
print()  # Empty line to separate headers from body

# Print HTML content
print("<!DOCTYPE html>")
print("<html>")
print("<head>")
print("    <title>CGI Test</title>")
print("    <style>")
print("        body { font-family: Arial, sans-serif; margin: 20px; }")
print("        h1 { color: #2c3e50; }")
print("        table { border-collapse: collapse; width: 100%; }")
print("        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }")
print("        th { background-color: #f2f2f2; }")
print("        tr:nth-child(even) { background-color: #f9f9f9; }")
print("    </style>")
print("</head>")
print("<body>")
print("    <h1>CGI Test Script</h1>")
print("    <p>This is a test CGI script running on the Webserv HTTP server.</p>")
print("    <p>Current time: {}</p>".format(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")))
print("    <h2>Environment Variables</h2>")
print("    <table>")
print("        <tr><th>Variable</th><th>Value</th></tr>")

# Print environment variables
for key, value in sorted(os.environ.items()):
    print("        <tr><td>{}</td><td>{}</td></tr>".format(key, value))

print("    </table>")
print("</body>")
print("</html>")
