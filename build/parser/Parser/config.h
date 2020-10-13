#pragma once

#include "util.h"

namespace ts3d { namespace exchange { namespace parser {
    class Config {
    public:
        static Config &instance();
        
        void readConfigurationFile( std::string const &config_file );

        bool shouldSkipWrapper( std::string const &exchange_data_struct_spelling ) const;
        bool shouldIgnoreTypeEnum( std::string const &type_enum_spelling ) const;
        bool shouldSkipField( std::string const &qualified_field_spelling ) const;
        
        StringSet const &getTypeEnumsToAssumeExist( void ) const;
        
        using ArrayFieldSpelling = std::string;
        using ArraySizeFieldSpelling = std::string;
        std::unordered_map<ArrayFieldSpelling, ArraySizeFieldSpelling> getArrayAndSizeFieldOverrides( std::string const &data_struct_spelling ) const;
        
    private:
        StringSet typeEnumsToIgnore;
        StringSet wrappersToSkip;
        StringSet fieldsToSkip;
        StringSet typeEnumsToAssumeExist;
        
        using OwnerTypeSpelling = std::string;
        using ChildTypeSpelling = std::string;
        using GetterCode = std::vector<std::string>;
        using GettersByChildType = std::unordered_map<ChildTypeSpelling, GetterCode>;
        using CustomGetters = std::unordered_map<OwnerTypeSpelling, GettersByChildType>;
        CustomGetters customGetters;
        
        std::unordered_map<std::string, std::unordered_map<ArrayFieldSpelling, ArraySizeFieldSpelling>>_arrayAndSizeFields;
    };
} } }
