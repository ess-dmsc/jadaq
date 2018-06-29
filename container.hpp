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
class NetworkReceive;

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

        template<typename... Args>
        typename std::vector<T>::iterator emplace(Args&&... args)
        {
            std::vector<T>::emplace_back(args...);
            return std::vector<T>::end()-1;
        }

        template< class InputIt >
        void insert(InputIt first, InputIt last)
        { std::vector<T>::insert(this->end(),first,last); }
    };

    template<typename T>
    class set : public std::set<T>
    {
    public:
        template <typename... Args>
        typename std::set<T>::iterator emplace(Args&&... args)
        { return std::set<T>::emplace(args...).first; }
    };

    template<typename T>
    class buffer
    {
    private:
        char* rawData;
        T* next;
        void check_length() const
        {
            if ((char*)(next+1) > rawData + Data::maxBufferSize)
            {
                throw std::length_error("Out of storage space.");
            }
        }
    public:
        buffer()
        {
            rawData = new char[Data::maxBufferSize];
            next = (T*)(rawData + sizeof(Data::Header));
            if ((char*)(next+1) > rawData + Data::maxBufferSize)
            {
                throw std::runtime_error("buffer creation error: buffer too small");
            }
        }
        ~buffer()
        {
            delete[] rawData;
        }

        char* data() { return rawData; }
        const char* data() const { return rawData; }

        //set number of elements from information in header
        void setElements()
        {
            next = (T*)(rawData + sizeof(Data::Header));
            next += ((Data::Header*)rawData)->numElements;
            if ((char*)(next) > rawData + Data::maxBufferSize)
            {
                throw std::runtime_error("Error in header or too much data in buffer.");
            }
        }

        void insert(const T&& v)
        {
            check_length();
            *next++ = std::move(v);
        }
        void insert(const T& v)
        {
            check_length();
            *next++ = v;
        }
        template <typename... Args>
        T* emplace(Args&&... args)
        {
            check_length();
            T* res = new(next++) T(args...);
            return res;
        }

        void clear()
        { next = (T*)(rawData + sizeof(Data::Header)); }
        T* begin()
        { return (T*)(rawData + sizeof(Data::Header)); }
        const T* begin() const
        { return (const T*)(rawData + sizeof(Data::Header)); }
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
            return ((char*)next-rawData);
        }

        bool empty() const noexcept
        {
            return (char*)next == rawData + sizeof(Data::Header);
        }
    };
}
#endif //JADAQ_CONTAINER_HPP
