#include <locale>
#include <codecvt>
#include <string>
#include <fstream>
#include <iostream>

#define INITIALIZE_A3D_API
#include <A3DSDKIncludes.h>

#ifdef _MSC_VER
#pragma warning(disable : 4503)
#endif

#include <ExchangeToolkit.h>

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#define xstr(s) __str(s)
#define __str(s) #s
static std::string const exchange_path = xstr(HOOPS_EXCHANGE_PATH);
static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;


A3DAsmModelFile *getModelFile( std::string const &fn ) {
    static std::pair< std::string, A3DAsmModelFile * > current_model_file_info;
    if( current_model_file_info.first == fn ) {
        return current_model_file_info.second;
    }
    
    if( current_model_file_info.second ) {
        A3DAsmModelFileDelete( current_model_file_info.second );
        current_model_file_info.second = nullptr;
        current_model_file_info.first.clear();
    }

    if( !fn.empty() ) {
        A3DImport i( fn.c_str() );
        i.m_sLoadData.m_sGeneral.m_bReadSolids = true;
        i.m_sLoadData.m_sGeneral.m_bReadWireframes = true;
        i.m_sLoadData.m_sGeneral.m_bReadPmis = true;
        i.m_sLoadData.m_sGeneral.m_eReadGeomTessMode = kA3DReadGeomAndTess;
        REQUIRE( A3D_SUCCESS == A3DAsmModelFileLoadFromFile( fn.c_str(), &i.m_sLoadData, &current_model_file_info.second ) );
        current_model_file_info.first = fn;
    }

    return current_model_file_info.second;
}

void freeModelFile( void ) {
    getModelFile( std::string() );
}

int main( int argc, char *argv[] ) {
#ifdef __MACH__
    auto const DIR_SEP = '/';
    auto const lib_path = exchange_path + "/bin/osx64";
    A3DSDKHOOPSExchangeLoader loader( lib_path.c_str()  );
#elif __linux__
    auto const DIR_SEP = '/';
    auto const lib_path = exchange_path + "/bin/linux64";
    A3DSDKHOOPSExchangeLoader loader( lib_path.c_str() );
#else
    auto const DIR_SEP = '\\';
    auto const lib_path = exchange_path + "/bin/win64";
    A3DSDKHOOPSExchangeLoader loader( converter.from_bytes( lib_path ).c_str() );
#endif
    
    if(! loader.m_bSDKLoaded ) {
        std::cerr << "Failed to load Exchange." << std::endl;
        std::cerr << "Tried: " << lib_path << std::endl;
        return -1;
    }

    auto const result =  Catch::Session().run( argc, argv );
    
    freeModelFile();

    return result;
}

