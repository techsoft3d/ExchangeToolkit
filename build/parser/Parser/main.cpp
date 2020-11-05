//
//  main.cpp
//  Parser
//
//  Created by Brad Flubacher on 3/5/20.
//  Copyright © 2020 Brad Flubacher. All rights reserved.
//

#include <iostream>
#include <vector>
#include <unordered_map>
#include <set>
#include <regex>
#include <fstream>
#include <sstream>
#include <clang-c/Index.h>
#include "util.h"
#include "config.h"
#include "traversal.h"
#include "wrapper.h"
#include "raii.h"
#include "csharp.h"
#include "MeaningfulCursors.h"

#ifdef __MSVC
static auto const DIR_SEP = '\\';
#else
static auto const DIR_SEP = '/';
#endif

int main(int argc, const char * argv[]) {
    
//    if( argc < 3 ) {
//        std::cerr << "Usage: parser <exchange dir> <wrapper output file> <getter output file>" << std::endl;
//        std::cerr << "  <exchange dir>  - specifies a the root folder of HOOPS Exchange" << std::endl;
//        std::cerr << "  <wrapper output file> - specifies the name of the wrapper output file to be written" << std::endl;
//        std::cerr << "  <getter output file> - specifies the name of the getter output file to be written" << std::endl;
//        std::cerr << std::endl;
//        std::cerr << "This application parses the Exchange headers and generates an output file" << std::endl;
//        std::cerr << "that can be used to populate the ExchangeToolkit functionality." << std::endl;
//        return -1;
//    }

    std::string raii_filename, exchange_folder, config_filename;
    std::string csharp_cpp_filename, csharp_h_filename, csharp_folder;
    for( auto idx = 1u; idx < argc; idx++ ) {
        if( idx + 1 < argc ) {
            std::string const arg = argv[idx];
            if( arg == "--raii" ) {
                raii_filename = argv[++idx];
            } else if( arg == "--exchange" ) {
                exchange_folder = argv[++idx];
            } else if( arg == "--config" ) {
                config_filename = argv[++idx];
            } else if( arg == "--csharp_cpp" ) {
                csharp_cpp_filename = argv[++idx];
            } else if( arg == "--csharp_h" ) {
                csharp_h_filename = argv[++idx];
            } else if( arg == "--csharp_folder" ) {
                csharp_folder = argv[++idx];
            }
        }
    }
        
//    std::string const wrapper_output_file = argv[2];
    std::ofstream raii_stream( raii_filename );
    if( !raii_filename.empty() ) {
        if( !raii_stream.is_open() ) {
            std::cerr << "Unable to open the raii wrapper output file stream." << std::endl;
            return -1;
        }
    }
    
//    std::string const getter_output_file = argv[3];
//    std::ofstream getter_stream( getter_output_file );
//    if( !getter_stream.is_open()) {
//        std::cerr << "Unable to open the getter output file stream." << std::endl;
//        return -1;
//    }
    
    if( !config_filename.empty()) {
        ts3d::exchange::parser::Config::instance().readConfigurationFile( config_filename );
    }
    
    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit unit = clang_parseTranslationUnit(
                                                        index,
                                                        (exchange_folder + "/include/A3DSDKIncludes.h").c_str(), nullptr, 0,
                                                        nullptr, 0,
                                                        CXTranslationUnit_None);
    if (unit == nullptr)
    {
        std::cerr << "Parse translation unit failed." << std::endl;
        return -1;
    }

    ts3d::MeaningfulCursors::instance().populate( clang_getTranslationUnitCursor( unit ) );

    // C#
    {
        using namespace ts3d::exchange::parser::csharp;
        if( !csharp_folder.empty() ) {
            using namespace direct;
            if(! writeEnums( unit, csharp_folder + DIR_SEP + "Enums.cs" ) ) {
                std::cerr << "Failed to write Enums.cs" << std::endl;
                return -1;
            }
            
            if( !writeStructs( unit, csharp_folder + DIR_SEP + "Structs.cs" ) ) {
                std::cerr << "Failed to write Structs.cs" << std::endl;
                return -1;
            }
            
            if( !writeAPI( unit, csharp_folder + DIR_SEP + "API.cs" ) ) {
                std::cerr << "Failed to write API.cs" << std::endl;
                return -1;
            }
            
//            if( !writeStructSizeTests( unit, csharp_folder + DIR_SEP + "StructTests.cs" ) ) {
//                std::cerr << "Failed to write StructTests.cs" << std::endl;
//                return -1;
//            }

            if( !writeClasses( unit, csharp_folder + DIR_SEP + "Classes.cs" ) ) {
                std::cerr << "Failed to write StructTests.cs" << std::endl;
                return -1;
            }
        }
        
//        if( !csharp_h_filename.empty() && ! csharp_cpp_filename.empty()) {
//            std::cout << "Writing C# native layer source files." << std::endl;
//            cpp_layer::write( unit, csharp_h_filename, csharp_cpp_filename);
//            std::cout << "Success." << std::endl;
//        }
    }
    
    

    if( raii_stream.is_open() ) {
        raii_stream << ts3d::exchange::parser::raii::instance().codeSpelling();

    }

    for( auto const &spelling : ts3d::MeaningfulCursors::instance()._stringToCursorMap ) {
        
        if( ts3d::exchange::parser::Category::Data != ts3d::exchange::parser::getCategory( spelling.first ) ) {
            continue;
        }
        
        auto const code_spelling = ts3d::exchange::parser::wrapper::instance().codeSpelling( spelling.second );
        std::cout << code_spelling;
    }
   
    
//
//    std::cout << ts3d::exchange::parser::raii::instance().codeSpelling();
//
//    auto const c = meaningfulCursors.getCursor("A3DAsmProductOccurrenceData");
//    dump( c );
//    ts3d::exchange::parser::wrapper::instance().codeSpelling( c );
//
//    for( auto const &spelling : meaningfulCursors._stringToCursorMap ) {
//
//        if( ts3d::exchange::parser::Category::Data != ts3d::exchange::parser::getCategory( spelling.first ) ) {
//            continue;
//        }
//
//        auto const type_spelling = ts3d::exchange::parser::getExchangeToken( spelling.first, ts3d::exchange::parser::Category::Type );
//        if( clang_equalCursors(meaningfulCursors.getCursor( type_spelling ), clang_getNullCursor()) ) {
//            continue;
//        }
//
//        auto fields = ts3d::exchange::parser::traversal::getArrayFieldMetaData( spelling.first );
//        if( fields.empty() ) {
//            continue;
//        }
//
//        std::unordered_map<std::string, std::vector<size_t>> type_to_index_list;
//        for( auto idx = 0u; idx < fields.size(); ++idx ) {
//            type_to_index_list[fields[idx]._type_spelling].push_back( idx );
//        }
//
//        getter_stream << "{ " << type_spelling << "," << std::endl;
//        getter_stream << "    {" << std::endl;
//        for( auto type_index_list : type_to_index_list ) {
//
//            getter_stream << "        {" << std::endl;
//            getter_stream << "            " << type_index_list.first << "," << std::endl;
//            getter_stream << "            [](A3DEntity *ntt) {" << std::endl;
//
//            auto it = ts3d::exchange::parser::customGetters.find( type_spelling );
//            if( std::end( ts3d::exchange::parser::customGetters ) != it ) {
//                auto getter_it = it->second.find( type_index_list.first );
//                if( std::end( it->second ) != getter_it ) {
//                    for( auto const &custom_getter_loc : getter_it->second ) {
//                        getter_stream << "                " << custom_getter_loc << std::endl;
//                    }
//                    getter_stream << "            }" << std::endl;
//                    getter_stream << "        }," << std::endl;
//                    continue;
//                }
//            }
//
//            getter_stream << "                ts3d::A3D" << ts3d::exchange::parser::getBaseExchangeToken(spelling.first) + "Wrapper d( ntt );" << std::endl;
//            if( type_index_list.second.size() > 1 ) {
//                getter_stream << "                ts3d::EntityArray result;" << std::endl;
//            }
//            for( auto const &idx : type_index_list.second ) {
//                auto const &field = fields[idx];
//                if( type_index_list.second.size() > 1 ) {
//                    if( field._array_size_spelling.empty() ) {
//                        getter_stream << "                result.push_back( d->" << field._pointer_spelling << " );" << std::endl;
//                    } else {
//                        getter_stream << "                {" << std::endl;
//                        getter_stream << "                    auto const sub_vector = ts3d::toVector( d->" << field._pointer_spelling << ", d->" << field._array_size_spelling << " );" << std::endl;
//                        getter_stream << "                    result.insert( std::end( result ), std::begin( sub_vector ), std::end( sub_vector ) );" << std::endl;
//                        getter_stream << "                }" << std::endl;
//                    }
//                } else {
//                    if( field._array_size_spelling.empty() ) {
//                        getter_stream << "                return ts3d::toVector( &(d->" << field._pointer_spelling << "), 1 );" << std::endl;
//                    } else {
//                        getter_stream << "                return ts3d::toVector( d->" << field._pointer_spelling << ", d->" << field._array_size_spelling << ");" << std::endl;
//                    }
//                }
//            }
//            if( type_index_list.second.size() > 1 ) {
//                getter_stream << "                return result;" << std::endl;
//            }
//            getter_stream << "            }" << std::endl;
//            getter_stream << "        }," << std::endl;
//        }
//        getter_stream << "    }" << std::endl;
//        getter_stream << "}," << std::endl;
//    }

//    for( auto missing_type_enum : meaningfulCursors._missingTypeEnums ) {
//        std::cout << "[WARNING] Missing type enum: " << missing_type_enum << std::endl;
//    }

    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);

    return 0;
}
