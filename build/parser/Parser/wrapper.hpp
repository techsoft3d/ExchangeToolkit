#pragma once

#include "MeaningfulCursors.hpp"
#include "config.h"
#include "util.h"

namespace ts3d { namespace exchange { namespace parser {
    namespace wrapper {
        struct FieldMetaData {
            FieldMetaData( CXCursor field_cursor ) :
            field_spelling( toString( clang_getCursorSpelling( field_cursor ) ) ),
            field_type_spelling( toString( clang_getTypeSpelling( clang_getCursorType( field_cursor ) ) ) ),
            is_array( false ) {
            }
            FieldMetaData( void ) : is_array( false ) {};
            std::string field_spelling;
            std::string field_type_spelling;
            bool is_array;
            std::string array_size_field_spelling;
        };
        
        static std::vector<FieldMetaData> foo( CXCursor struct_decl_cursor ) {
            std::unordered_map<CXCursor, FieldMetaData> meta_data;
            auto all_ui_field_cursors = ts3d::getAllFieldCursorsMatching( struct_decl_cursor, ts3d::RegExps::UnsignedIntField );
            auto field_cursors = ts3d::getAllFieldCursors( struct_decl_cursor );
            
            
            for( auto field_cursor : field_cursors ) {
                FieldMetaData field_metadata( field_cursor );
                
                std::smatch m;
                if( !std::regex_match( field_metadata.field_type_spelling, m, std::regex("A3D([0-9A-Za-z]+)[ \\*]+") ) || m.size() != 2 ) {
                    meta_data[field_cursor] = field_metadata;
                    continue;
                }
                
                auto const is_double_pointer = std::regex_match( field_metadata.field_spelling, ts3d::RegExps::DoublePointerField );
                if( !is_double_pointer ) {
                    meta_data[field_cursor] = field_metadata;
                    continue;
                }
                
                
                // must determine size field name
                std::regex_match( field_metadata.field_spelling, m, ts3d::RegExps::DoublePointerField );
                auto const field_name_without_m_ = m[1].str();
                auto best_similarity_metric = 0.;
                std::string best_similarity_field_name;
                CXCursor best_similarity_field_cursor;
                for( auto const ui_field_cursor : all_ui_field_cursors ) {
                    auto const potential_size_field = toString( clang_getCursorSpelling( ui_field_cursor ) );
                    auto const metric = getSimilarityMetric(potential_size_field, field_metadata.field_spelling );
                    if( metric > best_similarity_metric ) {
                        best_similarity_metric = metric;
                        best_similarity_field_name = potential_size_field;
                        best_similarity_field_cursor = ui_field_cursor;
                    }
                }
                
                if( best_similarity_metric >= .5 ) {
                    field_metadata.is_array = true;
                    field_metadata.array_size_field_spelling = best_similarity_field_name;
                    all_ui_field_cursors.erase( best_similarity_field_cursor );
                    meta_data.erase( best_similarity_field_cursor );
                    field_cursors.erase( best_similarity_field_cursor );
                } else {
                    // problem! couldn't figure out array size
                    //throw std::runtime_error("Couldn't determine size field for array." );
                }
                
                meta_data[field_cursor] = field_metadata;
            }
            std::vector<FieldMetaData> field_meta_data;
            for( auto const &p : meta_data ) {
                field_meta_data.push_back( p.second );
            }
            return field_meta_data;
        }
    }
}}}

