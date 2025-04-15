#!/usr/bin/env python3
import requests
import sys
import time

# Base URL for the server
BASE_URL = "http://localhost:8080"

# Test cases for path traversal attempts
PATH_TRAVERSAL_TESTS = [
    # Basic path traversal attempts
    "/../../etc/passwd",
    "/%2e%2e/%2e%2e/etc/passwd",
    "/%252e%252e/%252e%252e/etc/passwd",
    "/..%2f..%2fetc%2fpasswd",
    "/uploads/../../etc/passwd",
    "/uploads/..%2f..%2fetc%2fpasswd",
    
    # Attempts to access sensitive files
    "/etc/passwd",
    "/etc/shadow",
    "/proc/self/environ",
    "/proc/self/cmdline",
    "/proc/self/exe",
    
    # Attempts with null bytes
    "/uploads/test.txt%00.jpg",
    "/cgi-bin/test.py%00.txt",
    
    # Attempts with multiple dots
    "/....//....//etc/passwd",
    "/uploads/....//....//etc/passwd",
    
    # Attempts with URL encoding variations
    "/%25%32%65%25%32%65/%25%32%65%25%32%65/etc/passwd",
    "/uploads/%25%32%65%25%32%65/%25%32%65%25%32%65/etc/passwd"
]

# Test cases for file upload path traversal
UPLOAD_TESTS = [
    ("../../../etc/passwd", "This is a test"),
    ("..%2f..%2f..%2fetc%2fpasswd", "This is a test"),
    ("test.txt%00.jpg", "This is a test"),
    ("../test.txt", "This is a test"),
    ("../../test.txt", "This is a test")
]

# Test cases for CGI path traversal
CGI_TESTS = [
    "/cgi-bin/../../../etc/passwd",
    "/cgi-bin/..%2f..%2f..%2fetc%2fpasswd",
    "/cgi-bin/test.py%00.txt",
    "/cgi-bin/../test.py",
    "/cgi-bin/../../test.py"
]

def test_path_traversal():
    print("Testing path traversal attempts...")
    for path in PATH_TRAVERSAL_TESTS:
        url = f"{BASE_URL}{path}"
        print(f"Testing: {url}")
        try:
            response = requests.get(url, timeout=5)
            print(f"  Status: {response.status_code}")
            if response.status_code == 200:
                print("  WARNING: Request succeeded, potential security issue!")
            else:
                print("  Good: Request was blocked or file not found")
        except Exception as e:
            print(f"  Error: {e}")
        print()

def test_upload_traversal():
    print("Testing upload path traversal attempts...")
    for filename, content in UPLOAD_TESTS:
        url = f"{BASE_URL}/uploads/{filename}"
        print(f"Testing upload to: {url}")
        try:
            response = requests.post(url, data=content, timeout=5)
            print(f"  Status: {response.status_code}")
            if response.status_code == 201:
                print("  WARNING: Upload succeeded, potential security issue!")
            else:
                print("  Good: Upload was blocked")
        except Exception as e:
            print(f"  Error: {e}")
        print()

def test_cgi_traversal():
    print("Testing CGI path traversal attempts...")
    for path in CGI_TESTS:
        url = f"{BASE_URL}{path}"
        print(f"Testing: {url}")
        try:
            response = requests.get(url, timeout=5)
            print(f"  Status: {response.status_code}")
            if response.status_code == 200 and "root:" in response.text:
                print("  WARNING: CGI request succeeded and returned sensitive data, potential security issue!")
            else:
                print("  Good: CGI request was blocked or file not found")
        except Exception as e:
            print(f"  Error: {e}")
        print()

if __name__ == "__main__":
    print("Path Sanitization Test Script")
    print("============================")
    print()
    
    # Run the tests
    test_path_traversal()
    test_upload_traversal()
    test_cgi_traversal()
    
    print("Tests completed.")
