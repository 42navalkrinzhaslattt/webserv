#include "Server.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
    std::string configPath = "config/default.conf";  // Default config path
    
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
        return 1;
    }
    
    if (argc == 2) {
        configPath = argv[1];
    }
    
    try {
        Server server(configPath);
        if (!server.initialize(8080)) {
            std::cerr << "Failed to initialize server" << std::endl;
            return 1;
        }

        std::cout << "Server started on port 8080. Press Ctrl+C to stop." << std::endl;
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
