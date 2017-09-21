#include <iostream>
#include <fstream>
#include "Configuration.hpp"

int main(int argc, char **argv) {
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <config_file> " << std::endl;
        return -1;
    }
    std::string configFileName(argv[1]);
    std::ifstream configFile(configFileName);
    if (!configFile.good())
    {
        std::cerr << "Could not open configuration file: " << configFileName << std::endl;
    } else
    {
        Configuration configuration(configFile);
        std::ofstream configFile(configFileName+".out");
        if (!configFile.good())
        {
            std::cerr << "Unable to open output file: " << configFileName << std::endl;
        } else {
            configuration.write(configFile);
            configFile.close();
        }
    }
    configFile.close();
    return 0;
}