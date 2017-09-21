//
// Created by troels on 9/21/17.
//

#ifndef JADAQ_TRANSLATOR_HPP
#define JADAQ_TRANSLATOR_HPP
#include "ini_parser.hpp"
struct int_translator {
    typedef std::string internal_type;
    typedef int external_type;
    boost::optional<external_type> get_value(const internal_type& s)
    {
        try { return std::stoi(s,nullptr,0); }
        catch (...) { return boost::none; }
    }
    boost::optional<internal_type> put_value(const external_type& i)
    {
        return std::to_string(i);
    }
};
namespace boost {
    namespace property_tree {
        template<typename Ch, typename Traits, typename Alloc>
        struct translator_between<std::basic_string< Ch, Traits, Alloc >, int>
        {
            typedef int_translator type;
        };
    }
}

#endif //JADAQ_TRANSLATOR_HPP
