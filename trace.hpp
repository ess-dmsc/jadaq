//
// Created by troels on 2/9/18.
//
//#define DEBUGING
#define STATS
//#define TIMING


#ifndef JADAQ_TRACE_HPP
#define JADAQ_TRACE_HPP

#ifdef TIMING
#define TIME(x) x
#else
#define TIME(x)
#endif

#ifdef DEBUGING
#define DEBUG(x) x
#else
#define DEBUG(x)
#endif

#ifdef STATS
#define STAT(x) x
#else
#define STAT(x)
#endif



#endif //JADAQ_TRACE_HPP
