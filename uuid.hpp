
#ifndef JADAQ_UUID_HPP
#define JADAQ_UUID_HPP

#include <random>
#include <iostream>
#include <cstdint>
#include <cstddef>
#include  <iomanip>

class uuid
{
private:
    static std::random_device dev;
    static std::mt19937_64 gen;
    static std::uniform_int_distribution<uint64_t> dis;
    uint64_t value;
public:
    uuid() : value(dis(gen)) {}
    uuid(uint64_t v) : value(v) {}
    std::string toString() const;
};

static inline std::string to_string(const uuid &id) { return id.toString(); }
static inline std::ostream& operator<< (std::ostream& os, const uuid &id)
{ os << id.toString(); return os; }

#endif //JADAQ_UUID_HPP
