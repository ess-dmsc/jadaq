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