#pragma once

#include <clang-c/Index.h>
#include <vector>
#include <string>
#include <ostream>

namespace ts3d { namespace exchange { namespace parser {
    class raii {
    public:
        static raii &instance( void );
        
        std::string codeSpelling( void ) const;

        struct Wrapper {
            std::string data_type_spelling;
            std::string void_type_spelling;
            std::string initialize_function_spelling;
            std::string alias_declaration_spelling;
            std::string wrapper_spelling;
        };

    private:
        raii( void );
        std::vector<Wrapper> _wrappers;
        
    };
    inline bool operator<(raii::Wrapper const &lhs, raii::Wrapper const &rhs ) { return lhs.data_type_spelling < rhs.data_type_spelling; }
} } }
