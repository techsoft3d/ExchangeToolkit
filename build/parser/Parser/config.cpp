#include <fstream>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

#include "util.h"
#include "config.h"

namespace {
    ts3d::StringSet getArrayEntries( json &j, std::string const key) {
        ts3d::StringSet result;
        auto const type_enums_to_ignore = j[key];
        if( type_enums_to_ignore.is_array() ) {
            for( auto const &entry : type_enums_to_ignore.items() ) {
                std::string const v = entry.value();
                result.insert( v );
            }
        }
        return result;
    };
}

ts3d::exchange::parser::Config &ts3d::exchange::parser::Config::instance() {
    static Config _instance;
    return _instance;
}

bool ts3d::exchange::parser::Config::shouldSkipWrapper(const std::string &exchange_data_struct_spelling) const {
    return std::end(wrappersToSkip) != wrappersToSkip.find(exchange_data_struct_spelling);
}

bool ts3d::exchange::parser::Config::shouldSkipField(const std::string &qualified_field_spelling) const {
    return std::end(fieldsToSkip) != fieldsToSkip.find(qualified_field_spelling);
}

bool ts3d::exchange::parser::Config::shouldIgnoreTypeEnum(const std::string &type_enum_spelling) const {
    return std::end(typeEnumsToIgnore) != typeEnumsToIgnore.find(type_enum_spelling);
}

ts3d::StringSet const &ts3d::exchange::parser::Config::getTypeEnumsToAssumeExist( void ) const {
    return typeEnumsToAssumeExist;
}


void ts3d::exchange::parser::Config::readConfigurationFile( std::string const &config_file ) {
    std::ifstream config_stream( config_file );
    if( ! config_stream.is_open()) {
        return;
    }
    
    json j;
    config_stream >> j;
    typeEnumsToIgnore = getArrayEntries(j, "type_enums_to_ignore");
    wrappersToSkip = getArrayEntries(j, "wrappers_to_skip");
    fieldsToSkip = getArrayEntries(j, "fields_to_skip");
    typeEnumsToAssumeExist =  getArrayEntries(j, "type_enums_to_assume_exist" );
    auto custom_getters = j["custom_getters"];
    for( auto const &entry : custom_getters ) {
        for( auto const &owning_type_entry : entry.items() ) {
            auto const owner_type_spelling = owning_type_entry.key();
            for( auto const &e : owning_type_entry.value() ) {
                for( auto const &child_type_entry : e.items() ){
                    auto const child_type_spelling = child_type_entry.key();
                    std::vector<std::string> custom_getter_code;
                    for( auto const &l : child_type_entry.value().items() ) {
                        custom_getter_code.push_back( l.value() );
                    }
                    customGetters[owner_type_spelling][child_type_spelling] = custom_getter_code;
                }
            }
        }
    }
}
