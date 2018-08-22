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
 *
 */

#ifndef JADAQ_TIMER_HPP
#define JADAQ_TIMER_HPP

#include <iostream>
#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <thread>

namespace asio = boost::asio;
class Timer
{
private:
    asio::io_service timerservice;
    asio::steady_timer timer;
    std::thread* thread = nullptr;
public:
    template<typename F>
    Timer(float seconds,  F f)
            : timer{timerservice, std::chrono::milliseconds{(long)(seconds*1000.0f)}}
    {
        timer.async_wait([&f](const boost::system::error_code &ec)
        {
            if (ec)
                std::cerr << "Timer error: " << ec << std::endl;
            f();
        });
        thread = new std::thread{[this](){ timerservice.run(); }};
    }
    ~Timer()
    {
        thread->join();
        delete thread;
    }
};

#endif //JADAQ_TIMER_HPP
