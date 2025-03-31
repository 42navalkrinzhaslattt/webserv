#pragma once /* Errors.hpp */

#include <string>

#include "Config.hpp"

using std::string;

namespace Errors {
	namespace Config {
		const string OpeningError(const string &path);
		const string MissingArguments(const string &directiveName);
		const string MissingSemicolon(const string &directiveName);
		const string InvalidDirectiveName(const string &directiveName);
		const string InvalidArgument(const string &directiveName, const string &argument);
	}
}
