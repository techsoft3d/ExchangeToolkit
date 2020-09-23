//
//  MeaningfulCursors.cpp
//  Parser
//
//  Created by Brad Flubacher on 5/8/20.
//  Copyright © 2020 Brad Flubacher. All rights reserved.
//

#include "config.h"
#include "util.h"
#include "MeaningfulCursors.hpp"

using namespace ts3d::exchange;

ts3d::MeaningfulCursors &ts3d::MeaningfulCursors::instance( void ) {
    static MeaningfulCursors _instance;
    return _instance;
}

ts3d::MeaningfulCursors::MeaningfulCursors( void ) {
}

void ts3d::MeaningfulCursors::populate(CXCursor cursor) {
    for( auto const &type_enum_spelling : exchange::parser::Config::instance().getTypeEnumsToAssumeExist() ) {
        _stringToCursorMap[type_enum_spelling] = clang_getNullCursor();
    }
    
    clang_visitChildren( cursor, [](CXCursor c, CXCursor parent, CXClientData client_data) {
        auto pMeaningfulCursors = reinterpret_cast<MeaningfulCursors*>( client_data );
        if( nullptr == pMeaningfulCursors ) {
            return CXChildVisit_Break;
        }
        
        auto const cursor_spelling = toString( clang_getCursorSpelling(c) );
        auto const cursor_type = toString( clang_getTypeSpelling( clang_getCursorType( c ) ) );
        switch( clang_getCursorKind( c ) ) {
            case CXCursor_EnumDecl:
                if( cursor_type == "A3DEEntityType") {
                    return CXChildVisit_Recurse;
                }
                break;
            case CXCursor_EnumConstantDecl:
                if( std::regex_match( cursor_spelling, parser::getRegex( parser::Category::Type ) ) ) {
                    if( !exchange::parser::Config::instance().shouldIgnoreTypeEnum(cursor_spelling) ) {
                        assert( std::end( pMeaningfulCursors->_stringToCursorMap ) == pMeaningfulCursors->_stringToCursorMap.find( cursor_spelling ) );
                        pMeaningfulCursors->_stringToCursorMap[cursor_spelling] = c;
                    }
                    return CXChildVisit_Continue;
                }
                break;
            case CXCursor_StructDecl:
                if( std::regex_match( cursor_type, parser::getRegex( parser::Category::Data ) ) ) {
                    assert( std::end( pMeaningfulCursors->_stringToCursorMap ) == pMeaningfulCursors->_stringToCursorMap.find( cursor_spelling ) );
                    pMeaningfulCursors->_stringToCursorMap[cursor_type] = c;
                    return CXChildVisit_Continue;
                }
                break;
            case CXCursor_VarDecl:
                if( std::regex_match( cursor_spelling, parser::getRegex( parser::Category::Get ) ) ||
                   std::regex_match( cursor_spelling, parser::getRegex( parser::Category::Edit ) ) ||
                   std::regex_match( cursor_spelling, parser::getRegex(parser::Category::Create ) ) ) {
                    assert( std::end( pMeaningfulCursors->_stringToCursorMap ) == pMeaningfulCursors->_stringToCursorMap.find( cursor_spelling ) );
                    pMeaningfulCursors->_stringToCursorMap[cursor_spelling] = c;
                    return CXChildVisit_Continue;
                }
                break;
            default:
                break;
        }
        return CXChildVisit_Continue;
    }, this);
}

///*! \brief Returns the spelling of an input cursor for the specified category. */
//std::string ts3d::MeaningfulCursors::getExchangeToken( std::string const &cursor_spelling, Category const &category )


ts3d::StringSet ts3d::MeaningfulCursors::getAllCursorSpellings( parser::Category const &category ) const {
    auto const &regex = parser::getRegex( category );
    StringSet result;
    for( auto const &stringCursorPair : _stringToCursorMap ) {
        if( std::regex_match( stringCursorPair.first, regex ) ) {
            result.insert( stringCursorPair.first );
        }
    }
    return result;
}

CXCursor ts3d::MeaningfulCursors::getCursor( std::string const &cursor_spelling ) {
    auto it = _stringToCursorMap.find( cursor_spelling );
    if( std::end( _stringToCursorMap ) == it ) {
        return clang_getNullCursor();
    }
    return it->second;
}




bool ts3d::MeaningfulCursors::hasCursor( std::string const &spelling ) {
    return std::end( _stringToCursorMap ) != _stringToCursorMap.find( spelling );
}

