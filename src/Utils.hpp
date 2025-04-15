#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <iterator>

using std::iterator;
using std::string;
using std::vector;

namespace Utils {
	inline std::string &ltrim(std::string &s);
	inline std::string &rtrim(std::string &s);
	void	ft_trim(string &s);
	vector<string>	ft_split(const string &str);
	string ft_join(const vector<string>& strings, const string& delimiter);

	// Path sanitization methods
	bool isPathSafe(const string& path, const string& rootDir);
	string normalizePath(const string& path);
	string sanitizePath(const string& path, const string& rootDir);
	bool containsSuspiciousPatterns(const string& path);
	string urlDecode(const string& encoded);
}
