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

#ifndef JADAQ_CONTAINER_HPP
#define JADAQ_CONTAINER_HPP

#include <cstddef>
#include <cstring>

namespace jadaq
{
    template<typename T>
    class buffer
    {
    private:
        char* const data_raw;    // pointer to the raw allocated data
        char* const data_begin;  // pointer to where we begin inserting elements
        char* const data_end;    // pointer to end of data
        size_t const element_size;
        char* next;              // past end pointer
        void check_length() const
        {
            if (next+element_size > data_end)
            {
                throw std::length_error{"Out of storage space."};
            }
        }
    public:
        buffer(size_t raw_size, size_t object_size, size_t header_size)
                : data_raw(new char[raw_size])
                , data_begin(data_raw+header_size)
                , data_end(data_raw+raw_size)
                , element_size(object_size)
                , next(data_begin) {}

        buffer(size_t raw_size, size_t object_size)
                : buffer(raw_size,object_size,0) {}

        buffer(size_t raw_size)
                : buffer(raw_size, sizeof(T),0) {}

        buffer(size_t raw_size, buffer<T>& other)
                : buffer(raw_size, other.element_size, other.header_size())
        {
            copy(other);
        }

        static buffer* empty_like(buffer<T>& other)
        {
            return new buffer<T>(other.data_capacity(), other.element_size, other.header_size());
        }

        ~buffer()
        { delete[] data_raw; }

        void push_back(const T& v)
        {
            check_length();
            *next = v;
            memcpy(next, &v, element_size);
            next+=element_size;
        }
        template <typename... Args>
        void emplace_back(Args&&... args)
        {
            check_length();
            T* res = new (reinterpret_cast<T*>(next)) T(args...);
            next+=element_size;
        }

        void clear()
        { next = data_begin; }

        T* begin()
        { return reinterpret_cast<T*>(data_begin); }

        const T* begin() const
        { return reinterpret_cast<const T*>(data_begin); }

        T* end()
        { return reinterpret_cast<T*>(next); }

        const T* end() const
        { return reinterpret_cast<const T*>(next); }

        char* data()
        { return data_raw; }

        const char* data() const
        { return data_raw; }

        size_t data_size() const noexcept
        { return next-data_raw; }

        size_t data_capacity() const noexcept
        { return data_end-data_raw; }

        size_t header_size() const noexcept
        { return data_begin-data_raw; }

        size_t size() const
        { return end() - begin(); }

        size_t capacity() const
        { return (data_end-data_begin)/element_size; }

        size_t max_size() const noexcept
        { return capacity(); }

        bool empty() const noexcept
        { return next == data_begin; }

        void setElements(size_t n)
        { next = (data_begin + element_size*n); }

        void copy(const buffer<T>& other)
        {
            memcpy(data_raw,other.data_raw,other.data_size());
        }

        buffer<T>& operator=(const buffer<T>& other)
        {
            copy(other);
            return *this;
        }
    };
}
#endif //JADAQ_CONTAINER_HPP
