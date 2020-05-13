#pragma once

#include "util.h"

namespace ts3d { namespace exchange { namespace parser {
    static StringSet typeEnumsToIgnore;
    static StringSet wrappersToSkip;
    static StringSet fieldsToSkip;
    static StringSet typeEnumsToAssumeExist;

    using OwnerTypeSpelling = std::string;
    using ChildTypeSpelling = std::string;
    using GetterCode = std::vector<std::string>;
    using GettersByChildType = std::unordered_map<ChildTypeSpelling, GetterCode>;
    using CustomGetters = std::unordered_map<OwnerTypeSpelling, GettersByChildType>;
    static CustomGetters customGetters;

    void readConfigurationFile( std::string const &config_file );

} } }
