#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <exception>
#include <string>

class OnlyCheckConfigException : public std::exception {
public:
    OnlyCheckConfigException() {}
    virtual ~OnlyCheckConfigException() throw() {}
    virtual const char *what() const throw() { return "Configuration check successful"; }
};

class ConfigException : public std::exception {
private:
    std::string _message;

public:
    ConfigException(const std::string &message) : _message(message) {}
    virtual ~ConfigException() throw() {}
    virtual const char *what() const throw() { return _message.c_str(); }
};

#endif // EXCEPTIONS_HPP
