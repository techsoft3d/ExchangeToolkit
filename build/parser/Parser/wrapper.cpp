//
//  wrapper.cpp
//  Parser
//
//  Created by Brad Flubacher on 5/8/20.
//  Copyright © 2020 Brad Flubacher. All rights reserved.
//
#include "MeaningfulCursors.hpp"
#include "config.h"
#include "util.h"
#include "wrapper.hpp"

using namespace ts3d::exchange::parser;

wrapper &wrapper::instance( void ) {
    static wrapper _instance;
    
    return _instance;
}

namespace {
    struct FieldMetaData {
        FieldMetaData( CXCursor field_cursor ) :
            spelling( ts3d::toString( clang_getCursorSpelling( field_cursor ) ) ),
            type_spelling( ts3d::toString( clang_getTypeSpelling( clang_getCursorType( field_cursor ) ) ) ),
            is_array( false ) {
        }
        
        FieldMetaData( void ) : is_array( false ) {};
        
        std::string spelling;
        std::string type_spelling;
        bool is_array;
        std::string array_size_field_spelling;
    };
    
    CXCursor determineSizeField( ts3d::CursorSet all_ui_field_cursors, CXCursor array_cursor ) {
        auto const field_spelling = ts3d::toString( clang_getCursorSpelling( array_cursor ) );
        std::smatch base_match;
        if( !std::regex_match( field_spelling, base_match, std::regex("^m_[a-z]+([A-Z0-9][A-Za-z0-9]+)$") ) || base_match.size() != 2) {
            return clang_getNullCursor();
        }
        
        auto const field_name_without_m_ = base_match[1].str();
        auto best_similarity_metric = 0.;
        CXCursor best_similarity_field_cursor;
        for( auto const ui_field_cursor : all_ui_field_cursors ) {
            auto const potential_size_field = ts3d::toString( clang_getCursorSpelling( ui_field_cursor ) );
            auto const metric = ts3d::getSimilarityMetric(potential_size_field, field_spelling );
            if( metric > best_similarity_metric ) {
                best_similarity_metric = metric;
                best_similarity_field_cursor = ui_field_cursor;
            }
        }

        if( best_similarity_metric < .5 ) {
            return clang_getNullCursor();
        }

        return best_similarity_field_cursor;
    }
    
    static void foo( CXCursor struct_decl_cursor ) {
        std::vector<FieldMetaData> fields;
        auto all_ui_field_cursors = ts3d::getAllFieldCursorsMatching( struct_decl_cursor, ts3d::RegExps::UnsignedIntField );
        auto field_cursors = ts3d::getAllFieldCursors( struct_decl_cursor );
        
        for( auto field_cursor : field_cursors ) {
            auto const field_spelling = ts3d::toString( clang_getCursorSpelling( field_cursor ) );
            
            auto const cursor_type = clang_getCursorType( field_cursor );
            if( cursor_type.kind != CXType_Pointer ) {
                continue;
            }
            
            FieldMetaData field_meta_data( field_cursor );
            
            auto const pointee_type = clang_getPointeeType( cursor_type );
            if( pointee_type.kind == CXType_Pointer ) {
                // double pointer
                auto const pointee_pointee_type = clang_getPointeeType( pointee_type );
                if( pointee_pointee_type.kind == CXType_Typedef && clang_getTypedefDeclUnderlyingType(clang_getTypeDeclaration( pointee_pointee_type ) ).kind == CXType_Void ) {
                    // double pointer to void type
                    auto const pointee_pointee_spelling = ts3d::toString( clang_getTypeSpelling( pointee_pointee_type ) );
                    auto const data_type_spelling = pointee_pointee_spelling + "Data";
                    if( ts3d::MeaningfulCursors::instance().hasCursor( data_type_spelling ) ) {
                        // In this block, we know it is a double pointer to a void type
                        // that has a corresponding "Data" struct.
                        
                        // must determine size field
                        auto const size_field_cursor = determineSizeField( all_ui_field_cursors, field_cursor );
                        if( clang_equalCursors( size_field_cursor, clang_getNullCursor() ) ) {
                            // couldn't figure out a size field!
                            throw std::runtime_error("Unable to determine size field for array pointer: " + field_spelling );
                        }
                        all_ui_field_cursors.erase( size_field_cursor );
                        field_cursors.erase( size_field_cursor );

                        // wrapper array accessor
                    }
                }
            } else {
                // single pointer
                if( pointee_type.kind == CXType_Typedef && clang_getTypedefDeclUnderlyingType(clang_getTypeDeclaration( pointee_type ) ).kind == CXType_Void ) {
                    // single pointer to void type
                    auto const pointee_spelling = ts3d::toString( clang_getTypeSpelling( pointee_type ) );
                    auto const data_type_spelling = pointee_spelling + "Data";
                    if( ts3d::MeaningfulCursors::instance().hasCursor( data_type_spelling ) ) {
                        // In this block, we know it is a single pointer to a void type
                        // that has a corresponding "Data" struct.
                        
                        // no need for size field
                        
                        // wrapper accessor
                    }
                } else {
                    auto const size_field_cursor = determineSizeField( all_ui_field_cursors, field_cursor );
                    if( clang_equalCursors( size_field_cursor, clang_getNullCursor() ) ) {
                        // couldn't figure out a size field!
                        throw std::runtime_error("Unable to determine size field for array pointer: " + field_spelling );
                    }
                    all_ui_field_cursors.erase( size_field_cursor );
                    field_cursors.erase( size_field_cursor );

                    // Plain array accessor
                    field_meta_data.is_array = true;
                    field_meta_data.array_size_field_spelling = ts3d::toString( clang_getCursorSpelling( size_field_cursor ) );
                    fields.push_back( field_meta_data );
                }
            }
            
        }
    }
}

std::string wrapper::codeSpelling( CXCursor c ) const {
    foo( c );
    return std::string();
}

