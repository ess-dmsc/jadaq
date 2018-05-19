/**
 * jadaq (Just Another DAQ)
 * Copyright (C) 2017  Troels Blum <troels@blum.dk>
 *
 * @file
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
 * Special "augmented" instances of of containners for the purpose of delivring
 * a consistant interface for templated methods in jadaq
 *
 */

#include <vector>
#include <set>
#include <cassert>

#ifndef JADAQ_CONTAINER_HPP
#define JADAQ_CONTAINER_HPP

namespace jadaq
{
    template<typename T>
    class vector : public std::vector<T>
    {
    public:
        void insert(const T &v)
        { this->push_back(v); }

        void insert(const T &&v)
        { this->push_back(v); }
    };

    template<typename T>
    class set : public std::set<T>
    {
    private:
        std::vector<T> v;
    public:
        const T* data()
        {
            assert(v.size() == 0);
            v.insert(v.begin(),this->begin(),this->end());
            return v.data();
        }
        void clear() noexcept
        {
            v.clear();
            std::set<T>::clear();
        }
    };
}
#endif //JADAQ_CONTAINER_HPP
