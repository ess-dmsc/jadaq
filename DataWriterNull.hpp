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
 * Data handler that simply throws data away - for performance testing purposes
 *
 */

#ifndef JADAQ_DATAWRITERNULL_HPP
#define JADAQ_DATAWRITERNULL_HPP


#include <cstdint>

class DataWriterNull
{
public:
    void addDigitizer(uint32_t digitizerID) {}
    template <typename E, template<typename...> typename C>
    void write(const C<E>* buffer, uint32_t digitizerID, uint64_t globalTimeStamp) {}
};


#endif //JADAQ_DATAWRITERNULL_HPP
