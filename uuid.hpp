/**
 * jadaq (Just Another DAQ)
 * Copyright (C) 2018  Troels Blum <troels@blum.dk>
 *
 * @author Troels Blum <troels@blum.dk>
 * @section LICENSE
 * This program is free software: you can redistribute it and/or modify
 *        it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 *         but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @section DESCRIPTION
 * 64-bit UUID for use in the jadaq project
 *
 */

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
    std::string toString() const
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(16) << val;
        return ss.str();
    }
};

static inline std::string to_string(const uuid &id) { return id.toString(); }
static inline std::ostream& operator<< (std::ostream& os, const uuid &id)
{ os << id.toString(); return os; }

#endif //JADAQ_UUID_HPP
