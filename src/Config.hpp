#pragma once

#include <deque>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "Utils.hpp"
#include "Errors.hpp"

using std::string;

typedef std::vector<string> Arguments;
typedef std::pair<string, Arguments> Directive;
typedef std::vector<Directive> Directives;
typedef std::pair<string, Directives> LocationContext;
typedef std::vector<LocationContext> LocationContexts;
typedef std::pair<Directives, LocationContexts> ServerContext;
typedef std::vector<ServerContext> ServerContexts;
typedef std::pair<Directives, ServerContexts> Config;

typedef std::vector<Arguments> ArgResults;

enum TokenType {
	TOK_SEMICOLON,
	TOK_OPENING_BRACE,
	TOK_CLOSING_BRACE,
	TOK_WORD,
	TOK_EOF,
	TOK_UNKNOWN
};
typedef enum TokenType TokenType;

typedef std::pair<TokenType, string> Token;
typedef std::deque<Token> Tokens;

Config	parseConfig(const char *pathConf);
