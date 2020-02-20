#define INITIALIZE_A3D_API
#include "A3DSDKIncludes.h"

#include <locale>
#include <codecvt>
#include <string>
#include <fstream>
#include <regex>
#include <iostream>

#ifdef _MSC_VER
#pragma warning(disable : 4503)
#endif

#include "ExchangeToolkit.h"

#define xstr(s) __str(s)
#define __str(s) #s

static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

int main( int argc, char *argv[] ) {
    auto usage = []{
        std::cerr << "Usage: bom <input file>" << std::endl;
        std::cerr << "  <input file>  - specifies a the files to be read using HOOPS Exchange" << std::endl;
        std::cerr << std::endl;
        std::cerr << "This application reads the specified file and prints a BOM." << std::endl;
    };
    
    if( argc != 2 ) {
        usage();
        return -1;
    }

    std::string const input_file = argv[argc-1];
    std::cout << "Input file: " << input_file << std::endl;
    
    std::string const exchange_path = xstr(HOOPS_EXCHANGE_PATH);
#ifdef __MACH__
    auto const lib_path = exchange_path + "/bin/osx64";
    A3DSDKHOOPSExchangeLoader loader( lib_path.c_str()  );
#elif __linux__
    auto const lib_path = exchange_path + "/bin/linux64";
    A3DSDKHOOPSExchangeLoader loader( lib_path.c_str() );
#else
    auto const lib_path = exchange_path + "/bin/win64";
    A3DSDKHOOPSExchangeLoader loader( converter.from_bytes( lib_path ).c_str() );
#endif
    
    if(! loader.m_bSDKLoaded ) {
        std::cerr << "Failed to load Exchange." << std::endl;
        std::cerr << "Tried: " << lib_path << std::endl;
        return -1;
    }
    std::cout << "Exchange: " << exchange_path << std::endl;

    A3DImport i( input_file.c_str() );
    i.m_sLoadData.m_sGeneral.m_bReadSolids = true;
    i.m_sLoadData.m_sGeneral.m_bReadWireframes = false;
    i.m_sLoadData.m_sGeneral.m_eReadGeomTessMode = kA3DReadGeomOnly;

    loader.Import( i );
    if( nullptr == loader.m_psModelFile ) {
        std::cerr << "The specified file could not be loaded." << std::endl;
        return -1;
    }

    //! [Constructing a BOM]
    
    // An instance path map will be used to count the number of part occurrences
    ts3d::InstancePathMap instance_paths;

    // Obtain a set of unique part definition children
    auto const part_definitions = ts3d::getUniqueLeafEntities( loader.m_psModelFile, kA3DTypeAsmPartDefinition, instance_paths );

    // Iterator over each unique part
    for( auto part_definition : part_definitions ) {
        // Get the part's name or owner's name if empty
        ts3d::Instance const part_instance( { part_definition } );
        auto const part_name = part_instance.getName().empty() ? "<unknown>" : part_instance.getName();

        // Print the part name and number of occurrences
        std::cout << "\"" << part_name << "\": "
                  << "(" << instance_paths[part_definition].size() 
                  << " instance" 
                  << (1 != instance_paths[part_definition].size() ? "s)" : ")") 
                  << std::endl;
    }
    //! [Constructing a BOM]
    return 0;
}
