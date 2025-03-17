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
}
