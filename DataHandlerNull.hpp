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

#ifndef JADAQ_DATAHANDLERNULL_HPP
#define JADAQ_DATAHANDLERNULL_HPP

#include "DataHandler.hpp"

class DataHandlerNull: public DataHandler
{
public:
    void addEvent(Data::ListElement422 event) override {}
    void tick(uint64_t timeStamp) override {}
};


#endif //JADAQ_DATAHANDLERNULL_HPP
