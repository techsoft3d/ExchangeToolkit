#include "MeaningfulCursors.h"
#include "config.h"
#include "traversal.h"

using namespace ts3d::exchange::parser;

std::vector<traversal::ArrayFieldMetaData> traversal::getArrayFieldMetaData( std::string const &data_object_spelling ) {
    auto &meaningfulCursors = MeaningfulCursors::instance();
    auto it = meaningfulCursors._stringToCursorMap.find( data_object_spelling );
    if( std::end( meaningfulCursors._stringToCursorMap ) == it ) {
        return std::vector<ArrayFieldMetaData>();
    }
    
    struct Data {
        Data( StringToCursor const &map, StringSet &missing_type_enums ) : _string_to_cursor_map( map ), _missing_type_enums( missing_type_enums ) {}
        StringSet all_ui_fields;
        StringToCursor const &_string_to_cursor_map;
        StringSet &_missing_type_enums;
        std::vector<ArrayFieldMetaData> result;
        std::vector<ArrayFieldMetaData> incomplete;
    };
    Data d( meaningfulCursors._stringToCursorMap, meaningfulCursors._missingTypeEnums );
    d.all_ui_fields = getAllFieldsMatching( it->second, ts3d::RegExps::UnsignedIntField );
    
    clang_visitChildren( it->second, [](CXCursor c, CXCursor parent, CXClientData client_data) {
        if( CXCursor_FieldDecl != clang_getCursorKind(c) ) {
            return CXChildVisit_Continue;
        }
        auto d = reinterpret_cast<Data*>( client_data );
        
        auto const cursor_spelling = toString( clang_getCursorSpelling(c) );
        auto const is_single_pointer = std::regex_match( cursor_spelling, ts3d::RegExps::SinglePointerField );
        auto const is_double_pointer = std::regex_match( cursor_spelling, ts3d::RegExps::DoublePointerField );
        if( !is_single_pointer && !is_double_pointer ) {
            return CXChildVisit_Continue;
        }
        
        auto const type_spelling = toString( clang_getTypeSpelling( clang_getCursorType(c ) ) );
        std::smatch m;
        if( !std::regex_match( type_spelling, m, std::regex("A3D([0-9A-Za-z]+)[ \\*]+") ) || m.size() != 2 ) {
            return CXChildVisit_Continue;
        }
        
        auto const stripped_identifier = m[1].str();
        auto const type_enum_spelling = std::string( "kA3DType" ) + stripped_identifier;
        if( Config::instance().shouldIgnoreTypeEnum(type_enum_spelling) ) {
            return CXChildVisit_Continue;
        }
        
        if( std::end( d->_string_to_cursor_map ) == d->_string_to_cursor_map.find( type_enum_spelling ) ) {
            // the type enum is invalid
            d->_missing_type_enums.insert( type_enum_spelling );
            return CXChildVisit_Continue;
        }
        
        auto const scoped_field_name = toString(clang_getTypeSpelling(clang_getCursorType(parent))) + "::" + cursor_spelling;
        if( Config::instance().shouldSkipField( scoped_field_name ) ) {
            return CXChildVisit_Continue;
        }
        
        ArrayFieldMetaData md;
        md._pointer_spelling = cursor_spelling;
        md._type_spelling = type_enum_spelling;
        if( is_single_pointer ) {
            d->result.push_back( md );
            return CXChildVisit_Continue;
        }
        
        // must determine size field name
        std::regex_match( cursor_spelling, m, ts3d::RegExps::DoublePointerField );
        auto const field_name_without_m_ = m[1].str();
        auto best_similarity_metric = 0.;
        std::string best_similarity_field_name;
        for( auto const &potential_size_field : d->all_ui_fields ) {
            auto const metric = getSimilarityMetric(potential_size_field,  cursor_spelling);
            if( metric > best_similarity_metric ) {
                best_similarity_metric = metric;
                best_similarity_field_name = potential_size_field;
            }
        }
        
        if( best_similarity_metric >= .5 ) {
            md._array_size_spelling = best_similarity_field_name;
            d->result.push_back( md );
        } else {
            // problem! couldn't figure out array size
            d->incomplete.push_back( md );
        }
        return CXChildVisit_Continue;
    }, &d );
    
    if( d.incomplete.size() > 1 ) {
        std::cout << "Lots of incomplete fields for object \"" << data_object_spelling << "\"" << std::endl;
    }
    if( d.result.empty() && d.incomplete.size() == 1 && d.all_ui_fields.size() == 1 ) {
        auto field = d.incomplete.back();
        field._array_size_spelling = *d.all_ui_fields.begin();
        d.result.push_back( field );
    }
    
    return d.result;
}
