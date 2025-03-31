#include "Config.hpp"

static Tokens	lexConfig(std::ifstream	&configFile) {
	Tokens	tokens;
	string	line;
	string	rawConfig;
	bool	start = true;

	while (std::getline(configFile, line)) {
		if (!start)
			rawConfig += "\n";
		else
			start = false;
		rawConfig += line;
	}
	std::vector<string> vec = Utils::ft_split(rawConfig);

	for (std::vector<string>::iterator it = vec.begin(); it != vec.end(); it++) {
		string str = *it;

		if (str == "{")
			tokens.push_back(Token(TOK_OPENING_BRACE, str));
		else if (str == "}")
			tokens.push_back(Token(TOK_CLOSING_BRACE, str));
		else if (str == ";")
			tokens.push_back(Token(TOK_SEMICOLON, str));
		else
			tokens.push_back(Token(TOK_WORD, str));
	}

	for (Tokens::iterator it = tokens.begin(); it != tokens.end(); it++) {
		std::cout << it->second << std::endl;
	}

	return tokens;
}

Directive parseDirective(Tokens &tokens) {
	if (tokens.empty())
		throw std::runtime_error(Errors::Config::MissingArguments(string("")));

	if (tokens[0].first != TOK_WORD)
		throw std::runtime_error(Errors::Config::InvalidDirectiveName(tokens[0].second));

	if (tokens.size() < 3)
		throw std::runtime_error(Errors::Config::MissingArguments(tokens[0].second));

	Directive directive;
	directive.first = tokens[0].second;
	tokens.pop_front();

	while (!tokens.empty()) {
		if (tokens[0].first == TOK_SEMICOLON) {
			tokens.pop_front();
			return directive;
		}

		if (tokens[0].first != TOK_WORD)
			throw std::runtime_error(Errors::Config::InvalidArgument(directive.first, tokens[0].second));

		directive.second.push_back(tokens[0].second);
		tokens.pop_front();
	}

	throw std::runtime_error(Errors::Config::MissingSemicolon(directive.first));
}

Config	parseConfig(const char *pathConf) {

	std::ifstream	configFile(pathConf);

	if (!configFile.is_open())
		throw std::runtime_error(Errors::Config::OpeningError(pathConf));

	Tokens tokens = lexConfig(configFile);

	Config config;

	while (!tokens.empty()) {
		if (tokens.size() == 1)
			throw std::runtime_error(Errors::Config::MissingArguments(string(tokens[0].second)));
		switch (tokens[1].first)
		{
			case TOK_WORD:
				config.first.push_back(parseDirective(tokens));
				break;
			case TOK_OPENING_BRACE:
//				config.second.push_back(parseServer(tokens));
				break;
			default:
				throw std::runtime_error(Errors::Config::MissingArguments(tokens[1].second));
		}
	}

	for (Directives::iterator it = config.first.begin(); it != config.first.end(); it++) {
		Directive curr = *it;
		std::cout << "Directive name: " << curr.first << std::endl;

		for (Arguments::iterator strIt = curr.second.begin(); strIt != curr.second.end(); strIt++) {
			std::cout << "Directive argument: " << *strIt << std::endl;
		}

		std::cout << std::endl;
	}

	return config;
}
