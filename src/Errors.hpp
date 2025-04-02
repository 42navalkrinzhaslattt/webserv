#pragma once /* Errors.hpp */

#include <string>

#include "Config.hpp"

using std::string;

namespace Errors {
	namespace Config {
		const string OpeningError(const string &path);
		const string MissingArguments(const string &directiveName);
//		const string MissingSemicolon(const string &directiveName);
		const string InvalidDirectiveName(const string &directiveName);
		const string InvalidArgument(const string &directiveName, const string &argument);
		const string InvalidCtxName(const string &ctxName, const string &level);
//		const string MissingClosingBrace(const string &ctxName);
		const string ExpectedToken(const string &level, const string &token);
	}
}
