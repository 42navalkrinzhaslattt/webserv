#include <string>

#include "Config.hpp"
#include "Errors.hpp"

using std::string;

const string Errors::Config::OpeningError(const string &path) { return "Failed to open configuration file: " + path; }

const string Errors::Config::MissingArguments(const string &directiveName) { return "Missing arguments of the \"" + directiveName + "\""; }

const string Errors::Config::MissingSemicolon(const string &directiveName) { return "Missing semicolon in the end of the \"" + directiveName + "\""; }

const string Errors::Config::InvalidDirectiveName(const string &directiveName) { return "Invalid directive name \"" + directiveName + "\""; }

const string Errors::Config::InvalidArgument(const string &directiveName, const string &argument) { return "Invalid argument \"" + argument + "\"" + " in directive \"" + directiveName + "\""; }
