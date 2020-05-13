#pragma once

namespace ts3d { namespace exchange { namespace parser {
    namespace cpp {
        struct Field {};
        using FieldPtr = std::shared_ptr<Field>;
        struct SimpleField : Field {
            std::string spelling;
            std::string type_spelling;
        };
        struct ArrayField : Field {
            std::string _pointer_spelling;
            std::string _array_size_spelling;
        };
        
        struct WrapperClassInfo {
            std::string class_name;
            std::string parent_class_name;
            std::string raii_wrapper_typename;
            std::vector<FieldPtr> fields;
        };
    }
} } }
