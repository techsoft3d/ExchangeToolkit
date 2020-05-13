#pragma once

#include <clang-c/Index.h>
#include <vector>
#include <string>
#include <ostream>

namespace ts3d { namespace exchange { namespace parser {
    namespace raii {
        struct Entry {
            std::string void_type_spelling;
            bool has_get;
            bool has_edit;
            bool has_create;
            std::ostream &operator<<( std::ostream &os ) const;
        };
        std::vector<Entry> get( CXCursor c );
    }
} } }
