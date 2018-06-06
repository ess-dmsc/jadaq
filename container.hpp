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
#include "DataFormat.hpp"

#ifndef JADAQ_CONTAINER_HPP
#define JADAQ_CONTAINER_HPP

class DataWriterNetwork;

namespace jadaq
{
    template<typename T>
    class vector : public std::vector<T>
    {
    public:
        void insert(const T &v)
        { std::vector<T>::push_back(v); }

        void insert(const T &&v)
        { std::vector<T>::push_back(v); }

        template< class InputIt >
        void insert(InputIt first, InputIt last)
        { std::vector<T>::insert(this->end(),first,last); }
    };

    template<typename T>
    class set : public std::set<T> {};

    template<typename T>
    class buffer
    {
        friend class ::DataWriterNetwork;
    private:
        size_t max_size;
        const size_t header_size = sizeof(Data::Header);
        char* data;
        T* next;
        bool newData = false;
        void check_length() const
        {
            if (next+1 > data+max_size)
            {
                throw std::length_error("Out of storage space.");
            }
        }
    public:
        buffer(size_t size)
                : max_size(max_size)
        {
            data = new char[size];
            newData = true;
            next = (T*)(data + header_size);
            if (next+1 > data+max_size)
            {
                throw std::runtime_error("buffer creation error: buffer too small");
            }
        }
        buffer(Data::Header* header)
        {
            data = (char*)header;
            next = (T*)(data + header_size) + header->numElements;

        }
        ~buffer()
        {
            if (newData)
            {
                delete[] data;
            }
        }
        
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
        { next = (T*)(data + header_size); }
        T* begin()
        { return (T*)(data+header_size); }
        const T* begin() const
        { return (const T*)(data+header_size); }
        T* end()
        { return next; }
        const T* end() const
        { return next; }
        size_t size() const
        {
            return (end()-begin());
        }
        size_t data_size() const
        {
            return ((char*)next-data);
        }

    };
}
#endif //JADAQ_CONTAINER_HPP
