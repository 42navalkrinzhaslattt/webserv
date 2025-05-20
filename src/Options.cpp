#include "Options.hpp"
#include "Constants.hpp"

#include <cstring>
#include <iostream>

Options::Options(int ac, char **av)
    : printHelp(false), printVersion(false), onlyCheckConfig(false), onlyDumpConfig(false),
      configPath("conf/default.conf"), logLevel("INFO") {
    for (int i = 1; i < ac; ++i) {
        if (strcmp(av[i], "-h") == 0 || strcmp(av[i], "--help") == 0) {
            printHelp = true;
        } else if (strcmp(av[i], "-v") == 0 || strcmp(av[i], "--version") == 0) {
            printVersion = true;
        } else if (strcmp(av[i], "-t") == 0) {
            onlyCheckConfig = true;
        } else if (strcmp(av[i], "-T") == 0) {
            onlyCheckConfig = true;
            onlyDumpConfig = true;
        } else if (strcmp(av[i], "-c") == 0 && i + 1 < ac) {
            configPath = av[++i];
        } else if (strcmp(av[i], "-l") == 0 && i + 1 < ac) {
            logLevel = av[++i];
        } else if (av[i][0] != '-') {
            // Если аргумент не начинается с '-', считаем его путем к конфигурационному файлу
            configPath = av[i];
        } else {
            std::cerr << "Unknown option: " << av[i] << std::endl;
            printHelp = true;
        }
    }
}

Options::~Options() {}
