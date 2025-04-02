#include <string>

#include "Config.hpp"
#include "Errors.hpp"

using std::string;

const string Errors::Config::OpeningError(const string &path) { return "Failed to open configuration file: " + path; }

const string Errors::Config::MissingArguments(const string &directiveName) { return "Missing arguments of the \"" + directiveName + "\""; }

//const string Errors::Config::MissingSemicolon(const string &directiveName) { return "Missing semicolon in the end of the \"" + directiveName + "\" directive"; }

const string Errors::Config::InvalidDirectiveName(const string &directiveName) { return "Invalid directive name \"" + directiveName + "\""; }

const string Errors::Config::InvalidArgument(const string &directiveName, const string &argument) { return "Invalid argument \"" + argument + "\"" + " in directive \"" + directiveName + "\""; }

const string Errors::Config::InvalidCtxName(const string &ctxName, const string &level) { return "Invalid context name \"" + ctxName + "\"; must be \"" + "\"" + level; }

//const string Errors::Config::MissingClosingBrace(const string &ctxName) { return "Missing closing brace in the end of the \"" + ctxName + "\""; }

const string Errors::Config::ExpectedToken(const string &level, const string &token) { return "Expected token \'" + token + "\' in " + level; }
