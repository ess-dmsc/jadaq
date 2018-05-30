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
    class set : public std::set<T> {};

    template<typename T>
    class buffer
    {
    private:
        size_t size;
        size_t header;
        char* data;
        T* next;
        void check_length()
        {
            if (next+1 > data+size)
            {
                throw std::length_error("Out of storage space.");
            }
        }
    public:
        buffer(size_t total_size, size_t header_size)
                : size(total_size)
                , header(header_size)
        {
            data = new char[size];
            next = (T*)(data + header);
            if (next+1 > data+size)
            {
                throw std::runtime_error("buffer creation error: buffer too small");
            }
        }
        ~buffer()
        { delete[] data; }
        
        void insert(const T&& v)
        {
            check_length();
            *next = std::move(v);
        }
        void insert(const T& v)
        {
            check_length();
            *next = v;
        }
        void clear()
        { next = (T*)(data + header); }
        T* begin()
        { return (T*)(data+header); }
        const T* begin() const
        { return (const T*)(data+header); }
        T* end()
        { return next; }
        const T* end() const
        { return next; }

    };
}
#endif //JADAQ_CONTAINER_HPP
