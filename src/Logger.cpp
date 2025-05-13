#include "Logger.hpp"
#include "Ansi.hpp"

#include <iostream>
#include <vector>

Logger *Logger::_lastInstance = NULL;

Logger::Logger(std::ostream &out, const std::string &level) : _out(out), _level(level) {
    _lastInstance = this;
}

Logger::~Logger() {
    if (_lastInstance == this) {
        _lastInstance = NULL;
    }
}

bool Logger::_shouldLog(const std::string &level) const {
    static const std::string levels[] = {
        "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE", "TRACE2", "TRACE3", "TRACE4", "TRACE5", "TRACE6", "TRACE7", "TRACE8", "TRACE9"
    };
    static const size_t levelsSize = sizeof(levels) / sizeof(levels[0]);

    size_t levelIndex = 0;
    size_t configLevelIndex = 0;

    for (size_t i = 0; i < levelsSize; ++i) {
        if (levels[i] == level) {
            levelIndex = i;
        }
        if (levels[i] == _level) {
            configLevelIndex = i;
        }
    }

    return levelIndex <= configLevelIndex;
}

std::ostream &Logger::fatal() {
    if (_shouldLog("FATAL")) {
        return _out << ansi::red("[FATAL] ");
    }
    return std::cout;
}

std::ostream &Logger::error() {
    if (_shouldLog("ERROR")) {
        return _out << ansi::red("[ERROR] ");
    }
    return std::cout;
}

std::ostream &Logger::warn() {
    if (_shouldLog("WARN")) {
        return _out << ansi::yellow("[WARN] ");
    }
    return std::cout;
}

std::ostream &Logger::info() {
    if (_shouldLog("INFO")) {
        return _out << ansi::green("[INFO] ");
    }
    return std::cout;
}

std::ostream &Logger::debug() {
    if (_shouldLog("DEBUG")) {
        return _out << ansi::blue("[DEBUG] ");
    }
    return std::cout;
}

std::ostream &Logger::trace() {
    if (_shouldLog("TRACE")) {
        return _out << ansi::magenta("[TRACE] ");
    }
    return std::cout;
}

std::ostream &Logger::trace2() {
    if (_shouldLog("TRACE2")) {
        return _out << ansi::magenta("[TRACE2] ");
    }
    return std::cout;
}

std::ostream &Logger::trace3() {
    if (_shouldLog("TRACE3")) {
        return _out << ansi::magenta("[TRACE3] ");
    }
    return std::cout;
}

std::ostream &Logger::trace4() {
    if (_shouldLog("TRACE4")) {
        return _out << ansi::magenta("[TRACE4] ");
    }
    return std::cout;
}

std::ostream &Logger::trace5() {
    if (_shouldLog("TRACE5")) {
        return _out << ansi::magenta("[TRACE5] ");
    }
    return std::cout;
}

std::ostream &Logger::trace6() {
    if (_shouldLog("TRACE6")) {
        return _out << ansi::magenta("[TRACE6] ");
    }
    return std::cout;
}

std::ostream &Logger::trace7() {
    if (_shouldLog("TRACE7")) {
        return _out << ansi::magenta("[TRACE7] ");
    }
    return std::cout;
}

std::ostream &Logger::trace8() {
    if (_shouldLog("TRACE8")) {
        return _out << ansi::magenta("[TRACE8] ");
    }
    return std::cout;
}

std::ostream &Logger::trace9() {
    if (_shouldLog("TRACE9")) {
        return _out << ansi::magenta("[TRACE9] ");
    }
    return std::cout;
}

Logger &Logger::lastInstance() {
    if (_lastInstance == NULL) {
        static Logger defaultLogger(std::cerr, "INFO");
        return defaultLogger;
    }
    return *_lastInstance;
}
