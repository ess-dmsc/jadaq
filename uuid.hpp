
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
    uint64_t val;
public:
    uuid() : val(dis(gen)) {}
    uuid(uint64_t v) : val(v) {}
    uuid(const uuid& that) : val(that.value()) {}
    uuid& operator=(const uuid& that) { val = that.value(); return *this; }
    bool operator==(const uuid& that) const { return value() == that.value(); }
    bool operator!=(const uuid& that) const { return !(*this == that); }
    bool operator<(const uuid& that) const { return value() < that.value(); }
    bool operator>(const uuid& that) const { return that < *this; }
    bool operator<=(const uuid& that) const { return !(*this > that); }
    bool operator>=(const uuid& that) const { return !(*this < that); }
    uint64_t value() const { return val; }
    std::string toString() const;
};

static inline std::string to_string(const uuid &id) { return id.toString(); }
static inline std::ostream& operator<< (std::ostream& os, const uuid &id)
{ os << id.toString(); return os; }

#endif //JADAQ_UUID_HPP
