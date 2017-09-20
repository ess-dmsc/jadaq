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
        // Config file did not exist so we will create it
        std::vector<Digitizer> digitizers;
        int nDigitizers = 0;
        try {
            while(true)
            {
                digitizers.push_back(Digitizer(caen::Digitizer::USB(nDigitizers)));
                ++nDigitizers;
            }
        } catch (caen::Error& e)
        {
            std::cout << "Found " << nDigitizers << " Digitizers." << std::endl;
        }
        if (nDigitizers > 0)
        {
            std::ofstream configFile(configFileName);
            if (!configFile.good())
            {
                std::cout << "Unable to open configuration file: " << configFileName << std::endl;
                return -1;
            }
            Configuration configuration(std::move(digitizers));
            configuration.write(configFile);
        }


    } else
    {
        Configuration configuration(configFile);
    }

    return 0;
}