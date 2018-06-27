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
        friend class ::DataWriterNetwork;
        friend class ::NetworkReceive;
    private:
        char* data;
        T* next;
        void check_length() const
        {
            if ((char*)(next+1) > data + Data::maxBufferSize)
            {
                throw std::length_error("Out of storage space.");
            }
        }
        void setElements(uint16_t n)
        {
            next = (T*)(data + sizeof(Data::Header));
            next += n;
        }
    public:
        buffer()
        {
            data = new char[Data::maxBufferSize];
            next = (T*)(data + sizeof(Data::Header));
            if ((char*)(next+1) > data + Data::maxBufferSize)
            {
                throw std::runtime_error("buffer creation error: buffer too small");
            }
        }
        ~buffer()
        {
            delete[] data;
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
        void clear()
        { next = (T*)(data + sizeof(Data::Header)); }
        T* begin()
        { return (T*)(data + sizeof(Data::Header)); }
        const T* begin() const
        { return (const T*)(data + sizeof(Data::Header)); }
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

        bool empty() const noexcept
        {
            return (char*)next == data + sizeof(Data::Header);
        }
    };
}
#endif //JADAQ_CONTAINER_HPP
