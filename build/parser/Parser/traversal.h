#pragma once

#include <string>

namespace ts3d { namespace exchange { namespace parser {
    namespace traversal {
        struct ArrayFieldMetaData {
            std::string _pointer_spelling;
            std::string _array_size_spelling;
            std::string _type_spelling;
        };
        std::vector<ArrayFieldMetaData> getArrayFieldMetaData( std::string const &data_object_spelling );
    }
} } }
