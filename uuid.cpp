
#include <sstream>
#include "uuid.hpp"
std::uniform_int_distribution<uint64_t> uuid::dis;
std::random_device uuid::dev;
std::mt19937_64 uuid::gen(dev());

std::string uuid::toString() const
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << val;
    return ss.str();
}