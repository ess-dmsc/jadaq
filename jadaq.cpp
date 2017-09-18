#include <iostream>
#include <fstream>
#include "Configuration.hpp"

int main(int argc, char **argv) {
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <config_file> " << std::endl;
        return -1;
    }
    std::ifstream configFile(argv[1]);
    if (!configFile.good())
    {
        std::cout << "Could not open file " << argv[1] << std::endl;
        return  -1;
    }
    Configuration configuration(configFile);


    return 0;
}