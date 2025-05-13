#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <ostream>
#include <string>

class Logger {
private:
    static Logger *_lastInstance;
    std::ostream &_out;
    std::string _level;
    bool _shouldLog(const std::string &level) const;

public:
    Logger(std::ostream &out, const std::string &level);
    ~Logger();

    std::ostream &fatal();
    std::ostream &error();
    std::ostream &warn();
    std::ostream &info();
    std::ostream &debug();
    std::ostream &trace();
    std::ostream &trace2();
    std::ostream &trace3();
    std::ostream &trace4();
    std::ostream &trace5();
    std::ostream &trace6();
    std::ostream &trace7();
    std::ostream &trace8();
    std::ostream &trace9();

    static Logger &lastInstance();
};

#endif // LOGGER_HPP
