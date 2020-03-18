//
//  main.cpp
//  Parser
//
//  Created by Brad Flubacher on 3/5/20.
//  Copyright © 2020 Brad Flubacher. All rights reserved.
//

#include <iostream>
#include <vector>
#include <set>
#include <regex>
#include <fstream>
#include <clang-c/Index.h>

std::string toString( CXString const &str ) {
    std::string result;
    result = clang_getCString( str );
    clang_disposeString( str );
    return result;
}

inline std::ostream& operator<<(std::ostream& stream, const CXString& str) {
    return stream << toString(str);
}
    
void printCursorInfo( CXCursor c ) {
    std::cout << "Cursor '" << clang_getCursorSpelling(c)
        << "' of kind '" << clang_getCursorKindSpelling(clang_getCursorKind(c))
        << "' of type '" << clang_getTypeSpelling( clang_getCursorType(c ) )
        << "'"
        << std::endl;
}

void dump( CXCursor cursor ) {
    printCursorInfo( cursor );
        clang_visitChildren(
          cursor,
          [](CXCursor c, CXCursor parent, CXClientData client_data)
          {
            printCursorInfo( c );
            return CXChildVisit_Recurse;
          },
          nullptr);

}

int main(int argc, const char * argv[]) {
    if( argc < 2 ) {
        std::cerr << "Usage: parser <exchange dir> <output file>" << std::endl;
        std::cerr << "  <exchange dir>  - specifies a the root folder of HOOPS Exchange" << std::endl;
        std::cerr << "  <output file> - specifies the name of the output file to be written" << std::endl;
        std::cerr << std::endl;
        std::cerr << "This application parses the Exchange headers and generates an output file" << std::endl;
        std::cerr << "that can be used to populate the ExchangeToolkit functionality." << std::endl;
        return -1;
    }
    
    std::string const exchange_folder = argv[1];
    std::string const output_file = argv[2];
    std::vector<char const*> args = { "-DINITIALIZE_A3D_API" };
    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit unit = clang_parseTranslationUnit(
      index,
      (exchange_folder + "/include/A3DSDKIncludes.h").c_str(), args.data(), 1,
      nullptr, 0,
      CXTranslationUnit_None);
    if (unit == nullptr)
    {
        std::cerr << "Parse translation unit failed." << std::endl;
        return -1;
    }

    using StringSet = std::set<std::string>;
    using RegexStringSetPair = std::pair<std::regex, StringSet>;
    static std::vector<RegexStringSetPair> classifiedTypeDecls = {
        { std::regex("A3D[A-Za-z0-9]+Data"), StringSet() },
        { std::regex("A3D[A-Za-z0-9]+Get"), StringSet() },
        { std::regex("A3D[A-Za-z0-9]+Edit"), StringSet() },
        { std::regex("A3D[A-Za-z0-9]+Create"), StringSet() }
    };
    enum {
        Data = 0,
        Get = 1,
        Edit = 2,
        Create = 3
    };
    
    CXCursor cursor = clang_getTranslationUnitCursor(unit);
    clang_visitChildren(
                        cursor,
                        [](CXCursor c, CXCursor parent, CXClientData client_data) {
        auto const cursor_spelling = toString( clang_getCursorSpelling(c) );
        for( auto &p : classifiedTypeDecls ) {
            if( std::regex_match( cursor_spelling, p.first ) ) {
                p.second.insert( cursor_spelling );
            }
        }
        return CXChildVisit_Recurse;
      },
      nullptr);

    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);
    
    StringSet identifier_bases;
    auto const &dataTypedefDecls = classifiedTypeDecls[Data].second;
    for( auto const &data_typedef_decl : dataTypedefDecls ) {
        static std::regex base_regex( "(A3D[A-Za-z0-9]+)Data" );
        std::smatch base_match;
        if( std::regex_match( data_typedef_decl, base_match, base_regex ) ) {
            if( base_match.size() == 2) {
                auto base_sub_match = base_match[1];
                identifier_bases.insert( base_sub_match.str() );
            }
        }
    }
        
    std::ofstream output_stream( output_file.c_str() );
    if(!output_stream.is_open()) {
        std::cerr << "Unable to open output stream." << std::endl;
        return -2;
    }
    
    for( auto const &base : identifier_bases ) {
        auto has_get = false;
        auto has_edit = false;
        auto has_create = false;
        
        if( std::end( classifiedTypeDecls[Get].second ) != classifiedTypeDecls[Get].second.find( base + "Get" ) ) {
            output_stream << "A3D_HELPERS(" << base << ")" << std::endl;
            has_get = true;
        }
        
        if( std::end( classifiedTypeDecls[Edit].second ) != classifiedTypeDecls[Edit].second.find( base + "Edit" ) ) {
            has_edit = true;
        }
        
        if( std::end( classifiedTypeDecls[Create].second ) != classifiedTypeDecls[Create].second.find( base + "Create" ) ) {
            has_create = true;
        }
        
        std::cout << base << "[ " << (has_get ? "GET " : "") << (has_edit ? "EDIT " : "" ) << (has_create ? "CREATE " : "" ) << "]" << std::endl;
    }
    
    return 0;
}
