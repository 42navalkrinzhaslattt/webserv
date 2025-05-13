#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <string>

class Options {
public:
    Options(int ac, char **av);
    ~Options();

    bool printHelp;
    bool printVersion;
    bool onlyCheckConfig;
    bool onlyDumpConfig;
    std::string configPath;
    std::string logLevel;
};

#endif // OPTIONS_HPP
