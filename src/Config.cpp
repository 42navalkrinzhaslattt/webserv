#include "Config.hpp"

static string unquoteString(const string& str) {
    if (str.length() >= 2 && str[0] == '"' && str[str.length() - 1] == '"') {
        return str.substr(1, str.length() - 2);
    }
    return str;
}

static Tokens lexConfig(std::ifstream &configFile) {
    Tokens tokens;
    string line;
    string rawConfig;
    bool start = true;

    while (std::getline(configFile, line)) {
        // Remove comments
        size_t commentPos = line.find('#');
        if (commentPos != string::npos) {
            line = line.substr(0, commentPos);
        }

        // Skip empty lines after removing comments
        if (line.empty() || line.find_first_not_of(" \t\r\n") == string::npos)
            continue;

        if (!start)
            rawConfig += "\n";
        else
            start = false;
        rawConfig += line;
    }

    std::vector<string> vec = Utils::ft_split(rawConfig);
    for (std::vector<string>::iterator it = vec.begin(); it != vec.end(); it++) {
        string str = *it;

        // Process special characters and quoted strings
        if (str == "{")
            tokens.push_back(Token(TOK_OPENING_BRACE, str));
        else if (str == "}")
            tokens.push_back(Token(TOK_CLOSING_BRACE, str));
        else if (str == ";")
            tokens.push_back(Token(TOK_SEMICOLON, str));
        else if (str.length() >= 2 && str[0] == '"' && str[str.length() - 1] == '"')
            tokens.push_back(Token(TOK_WORD, unquoteString(str)));
        else if (str.find('"') != string::npos)
            throw std::runtime_error("Invalid quote in string: " + str);
        else
            tokens.push_back(Token(TOK_WORD, str));
    }

    return tokens;
}

static bool isValidPath(const string& path, bool) {
    // For quoted paths, we allow spaces
    if (path.length() >= 2 && path[0] == '"' && path[path.length() - 1] == '"') {
        return true;
    }
    // For unquoted paths, no spaces allowed
    return path.find(' ') == string::npos;
}

static bool isValidMethod(const string& method) {
    return method == "GET" || method == "POST" || method == "DELETE";
}

static bool isValidDirective(const string& directive, const string& value, bool) {
    if (directive == "root" || directive == "location") {
        return isValidPath(value, directive == "location");
    }
    else if (directive == "server_name") {
        return true; // Allow any server name
    }
    else if (directive == "listen") {
        int port = std::atoi(value.c_str());
        return port > 0 && port < 65536;
    }
    else if (directive == "allowed_methods") {
        return isValidMethod(value);
    }
    else if (directive == "index") {
        return true; // Allow any index file name
    }
    return false;
}

Directive parseDirective(Tokens &tokens) {
    if (tokens.empty())
        throw std::runtime_error(Errors::Config::MissingArguments(string("")));

    Directive directive;
    directive.first = tokens[0].second;
    tokens.erase(tokens.begin());

    // Check for semicolon
    if (tokens.empty() || tokens[0].first == TOK_SEMICOLON)
        throw std::runtime_error(Errors::Config::MissingArguments(directive.first));

    // Parse arguments until semicolon
    while (!tokens.empty() && tokens[0].first != TOK_SEMICOLON) {
        const string& value = tokens[0].second;

        // Validate directive values
        if (!isValidDirective(directive.first, value, directive.first == "location")) {
            throw std::runtime_error("Invalid value for directive '" + directive.first + "': " + value);
        }

        // Store unquoted value in the directive
        directive.second.push_back(unquoteString(value));
        tokens.erase(tokens.begin());
    }

    // Remove semicolon
    if (!tokens.empty() && tokens[0].first == TOK_SEMICOLON)
        tokens.erase(tokens.begin());
    else
        throw std::runtime_error(Errors::Config::MissingSemicolon(directive.first));

    return directive;
}

static LocationContext parseLocation(Tokens &tokens) {
    if (tokens.size() < 5)
        throw std::runtime_error(Errors::Config::MissingArguments("location"));

    if (tokens[0].second != "location")
        throw std::runtime_error(Errors::Config::InvalidCtxName(tokens[0].second, "location"));
    tokens.pop_front();

    if (tokens[0].first != TOK_WORD)
        throw std::runtime_error(Errors::Config::InvalidArgument("location", tokens[0].second));

    LocationContext location;
    location.first = tokens[0].second;
    tokens.pop_front();

    if (tokens[0].first != TOK_OPENING_BRACE)
        throw std::runtime_error(Errors::Config::MissingBrace("location", "{"));
    tokens.pop_front();

    while (!tokens.empty() && tokens[0].first != TOK_CLOSING_BRACE) {
        location.second.push_back(parseDirective(tokens));
    }

    if (tokens.empty() || tokens[0].first != TOK_CLOSING_BRACE)
        throw std::runtime_error(Errors::Config::MissingBrace("location", "}"));

    tokens.pop_front();
    return location;
}

static ServerContext parseServer(Tokens &tokens) {
    if (tokens.empty())
        throw std::runtime_error(Errors::Config::MissingArguments(string("")));

    if (tokens[0].second != "server")
        throw std::runtime_error(Errors::Config::InvalidCtxName(tokens[0].second, "server"));

    tokens.pop_front();
    tokens.pop_front();

    if (tokens.size() < 2)
        throw std::runtime_error(Errors::Config::MissingArguments("server"));

    ServerContext serverCtx;

    while (!tokens.empty()) {
        if (tokens[0].first == TOK_CLOSING_BRACE) {
            tokens.pop_front();
            break;
        }
        if (tokens.size() == 1)
            throw std::runtime_error(Errors::Config::ExpectedToken("location", "}"));

        if (tokens.size() == 2)
            throw std::runtime_error(Errors::Config::MissingArguments(tokens[1].second));

        switch (tokens[2].first)
        {
            case TOK_WORD:
                serverCtx.first.push_back(parseDirective(tokens));
                break;
            case TOK_SEMICOLON:
                serverCtx.first.push_back(parseDirective(tokens));
                break;
            case TOK_OPENING_BRACE:
                serverCtx.second.push_back(parseLocation(tokens));
                break;
            default:
                throw std::runtime_error(Errors::Config::MissingArguments(tokens[1].second));
        }
    }

    return serverCtx;
}

Config parseConfig(const char *pathConf) {

    std::ifstream configFile(pathConf);

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
                config.second.push_back(parseServer(tokens));
                break;
            default:
                throw std::runtime_error(Errors::Config::MissingArguments(tokens[1].second));
        }
    }

    return config;
}
