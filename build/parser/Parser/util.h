#pragma once

#include <set>
#include <unordered_map>
#include <regex>
#include <iomanip>
#include <iostream>
#include <clang-c/Index.h>

namespace ts3d {
    using StringSet = std::set<std::string>;
    using StringToCursor = std::unordered_map<std::string, CXCursor>;

    namespace RegExps {
        static std::regex const A3D_Data("^A3D([A-Za-z0-9]+)Data$");
        static std::regex const A3D_Get("^A3D([A-Za-z0-9]+)Get$");
        static std::regex const A3D_Edit("^A3D([A-Za-z0-9]+)Edit$");
        static std::regex const A3D_Create("^A3D([A-Za-z0-9]+)Create$");
        static std::regex const A3DTypeEnum("^kA3DType([A-Za-z0-9]+)$");
        static std::regex const UnsignedIntField("^m_ui([A-Z0-9][A-Za-z0-9]+)$");
        static std::regex const SinglePointerField("^m_p([A-Z0-9][A-Za-z0-9]+)$");
        static std::regex const DoublePointerField("^m_pp([A-Z0-9][A-Za-z0-9]+)$");
    }

    static std::string toString( CXString const &str ) {
        std::string result;
        result = clang_getCString( str );
        clang_disposeString( str );
        return result;
    }
}

namespace std {
    template<> struct hash<CXCursor> {
        std::size_t operator()(CXCursor const &c ) const noexcept {
            return clang_hashCursor(c);
        }
    };
}

inline bool operator==(CXCursor const &lhs, CXCursor const &rhs ) {
    return clang_equalCursors( lhs, rhs );
}

inline bool operator<(CXCursor const &lhs, CXCursor const &rhs ) {
    return clang_hashCursor(lhs) < clang_hashCursor(rhs);
}

inline std::ostream& operator<<(std::ostream& stream, const CXString& str) {
    return stream << ts3d::toString(str);
}


namespace ts3d {
    using CursorSet = std::set<CXCursor>;
    static CursorSet getAllFieldCursors( CXCursor c ) {
        struct Data {
            CursorSet cursors;
        };
        Data d;
        clang_visitChildren( c, [](CXCursor c, CXCursor parent, CXClientData client_data) {
            auto d = reinterpret_cast<Data*>(client_data);
            if( clang_getCursorKind( c ) == CXCursor_FieldDecl ) {
                d->cursors.insert( c );
            }
            return CXChildVisit_Continue;
        }, &d );
        return d.cursors;
    }

    static CursorSet getAllFieldCursorsMatching( CXCursor cursor, std::regex const &r ) {
        CursorSet matches;
        for( auto field_cursor : getAllFieldCursors( cursor ) ) {
            auto const field_spelling = toString( clang_getCursorSpelling( field_cursor ) );
            if( std::regex_match( field_spelling, r ) ) {
                matches.insert( field_cursor );
            }
        }
        return matches;
    }
    
    static StringSet getAllFieldsMatching( CXCursor cursor, std::regex const &r ) {
        StringSet matches;
        for( auto field_cursor : getAllFieldCursorsMatching( cursor, r ) ) {
            matches.insert( toString( clang_getCursorSpelling( field_cursor ) ) );
        }
        return matches;
    }
    
    static bool hasField( CXCursor cursor, std::string const &field_spelling ) {
        return !getAllFieldsMatching( cursor, std::regex( field_spelling ) ).empty();
    }
    
    static StringSet getWords( std::string const s ) {
        StringSet result;
        std::smatch m;
        if( std::regex_match( s, m, std::regex("^m_(ui|pp)([0-9A-Z][0-9a-zA-Z]+)") ) && m.size() == 3 ) {
            const_cast<std::string&>(s) = m[2];
        }
        auto const with_spaces = std::regex_replace( s, std::regex("[0-9A-Z]"), " $&" );
        std::string::size_type n = 0;
        do {
            auto last_n = n;
            n = with_spaces.find( " ", last_n );
            if( std::string::npos != n ) {
                auto const word = with_spaces.substr( last_n, n - last_n );
                if( !word.empty() ) {
                    result.insert( word );
                }
                n++;
            } else {
                auto const word = with_spaces.substr( last_n );
                if( !word.empty() ) {
                    result.insert( word );
                }
            }
        } while( std::string::npos != n );
        return result;
    }
    
    static double getSimilarityMetric( std::string const &a, std::string const &b ) {
        auto const a_words = getWords( a );
        auto const b_words = getWords( b );
        std::set<std::string> matching_words;
        for( auto const &a : a_words ) {
            for( auto const &b : b_words ) {
                if( a == b ) {
                    matching_words.insert( a );
                    continue;
                }

                auto common_prefix_character_count = 0u;
                for( auto idx = 0u; idx < std::min( a.size(), b.size() ); ++idx ) {
                    if( a.at(idx) == b.at(idx) ) {
                        common_prefix_character_count++;
                    } else {
                        break;
                    }
                }
                auto const percent_match = static_cast<double>( common_prefix_character_count ) / static_cast<double>(std::min(a.size(), b.size()));
                if( percent_match >= .5 ) {
                    matching_words.insert( a );
                }
            }
        }
        auto const denom = std::min( a_words.size(), b_words.size() );
        return static_cast<double>( matching_words.size() ) / static_cast<double>(denom);
    }
}

//void foo( CXCursor cursor ) {
//    struct Data {
//        std::vector<std::string> code;
//    };
//    Data d;
//    clang_visitChildren( cursor, [](CXCursor c, CXCursor parent, CXClientData client_data) {
//        Data &d = *static_cast<Data*>(client_data);
//        if( CXCursor_FieldDecl == clang_getCursorKind( c ) ) {
//            auto const field_spelling = toString( clang_getCursorSpelling( c ) );
//            auto const field_type_spelling = toString( clang_getTypeSpelling( clang_getCursorType( c ) ) );
//        }
//        return CXChildVisit_Recurse;
//    }, &d );
//}

namespace {
    void printCursorInfo( CXCursor c, int const indent_level ) {
        if( indent_level ) {
            std::cout <<  std::setw( 4 * indent_level ) << " ";
        }
        std::cout << "Cursor '" << clang_getCursorSpelling(c)
        << "' of kind '" << clang_getCursorKindSpelling(clang_getCursorKind(c))
        << "' of type '" << clang_getTypeSpelling( clang_getCursorType(c ) )
        << "'"
        << std::endl;
    }
    
    void dump( CXCursor cursor ) {
        std::unordered_map<CXCursor, int> indent_levels;
        indent_levels[cursor] = 0;
        printCursorInfo( cursor, 0 );
        clang_visitChildren(
                            cursor,
                            [](CXCursor c, CXCursor parent, CXClientData client_data)
                            {
            auto &indent_levels = *static_cast<std::unordered_map<CXCursor, int>*>( client_data );
            auto const indent_level = indent_levels[parent] + 1;
            indent_levels[c] = indent_level;
            printCursorInfo( c, indent_level );
            return CXChildVisit_Recurse;
        },
                            &indent_levels );
        
    }
}

namespace ts3d { namespace exchange { namespace parser {
    enum class Category {
        Data = 0,
        Get = 1,
        Edit = 2,
        Create = 3,
        Type = 4,
        MAX_VALUE = 5
    };

    
    static std::regex const &getRegex( Category const &c ) {
        switch( c ) {
            case Category::Data:
                return ts3d::RegExps::A3D_Data;
            case Category::Get:
                return ts3d::RegExps::A3D_Get;
            case Category::Edit:
                return ts3d::RegExps::A3D_Edit;
            case Category::Create:
                return ts3d::RegExps::A3D_Create;
            case Category::Type:
                return ts3d::RegExps::A3DTypeEnum;
            default:
                break;
        }
        static std::regex const _empty;
        return _empty;
    }

    /*! \brief Returns the base token that is common to all category type.
     * kA3DTypeAsmModelFile -> AsmModelFile
     *  A3DAsmModelFileData -> AsmModelFile
     */
    static std::string getBaseExchangeToken( std::string const &cursor_spelling ) {
        std::smatch base_match;
        for( auto idx = 0u; idx < static_cast<unsigned int>(Category::MAX_VALUE); ++idx ) {
            auto const &category = static_cast<Category>(idx);
            auto const &regex = getRegex( category );
            if( std::regex_match( cursor_spelling, base_match, regex ) && base_match.size() == 2) {
                return base_match[1];
            }
        }
        return std::string();
    }

    /*! \brief Returns the spelling of an input cursor for the specified category. */
    static std::string getExchangeToken( std::string const &cursor_spelling, Category const &category ) {
        auto const baseToken = getBaseExchangeToken( cursor_spelling );
        if( !baseToken.empty() ) {
            switch( category ) {
                case Category::Data:
                    return "A3D" + baseToken + "Data";
                case Category::Get:
                    return "A3D" + baseToken + "Get";
                case Category::Edit:
                    return "A3D" + baseToken + "Edit";
                case Category::Create:
                    return "A3D" + baseToken + "Create";
                case Category::Type:
                    return "kA3DType" + baseToken;
                default:
                    break;
            }
        }
        return std::string();
    }
    
    static Category getCategory( std::string const &cursor_spelling ) {
        for( auto idx = 0u; idx < static_cast<unsigned int>(Category::MAX_VALUE); ++idx ) {
            auto const &category = static_cast<Category>(idx);
            if( std::regex_match( cursor_spelling, getRegex( category ) ) ) {
                return category;
            }
        }
        return Category::MAX_VALUE;
    }
}}}
