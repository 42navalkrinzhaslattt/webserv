#include "Server.hpp"

inline std::string &Utils::ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
									std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

inline std::string &Utils::rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(),
						 std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

void	Utils::ft_trim(string &s) {
	ltrim(s);
	rtrim(s);
}

vector<string>	Utils::ft_split(const string &str) {
	vector<string>	res;
	string	copy(str);

	rtrim(copy);
	while(!copy.empty()) {
		ltrim(copy);
		string::iterator it;
		if ((it = std::find_if(copy.begin(), copy.end(), std::ptr_fun<int, int>(std::isspace))) != copy.end()) {
			res.push_back(copy.substr(0, copy.find(*it)));
			copy.erase(0, copy.find(*it));
		}
		else if (!copy.empty()) {
			res.push_back(copy);
			return res;
		}
	}
	return res;
}
