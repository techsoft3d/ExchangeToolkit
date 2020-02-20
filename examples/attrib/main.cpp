#define INITIALIZE_A3D_API
#include "A3DSDKIncludes.h"

#include <locale>
#include <codecvt>
#include <string>
#include <fstream>
#include <iostream>

#ifdef _MSC_VER
#pragma warning(disable : 4503)
#endif

#include "ExchangeToolkit.h"
#include "PointsOfInterest.h"

#define xstr(s) __str(s)
#define __str(s) #s

static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;




int main( int argc, char *argv[] ) {
    if( argc < 2 ) {
        std::cerr << "Usage: attrib <input file> <output file>" << std::endl;
        std::cerr << "  <input file>  - specifies a the files to be read using HOOPS Exchange" << std::endl;
        std::cerr << "  <output file> - specifies the name of the output file to be written" << std::endl;
        std::cerr << std::endl;
        std::cerr << "This application reads the specified file and iterates over all B-Rep" << std::endl;
        std::cerr << "faces and edges. For each, it computes \"points/directions of interest\"" << std::endl;
        std::cerr << "which are encoded in a JSON string and attached as attributes." << std::endl;
        return -1;
    }
    
    std::string const input_file = argv[1];
    std::string const output_file = argv[2];

    std::string const exchange_path = xstr(HOOPS_EXCHANGE_PATH);
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
    
    A3DImport i( input_file.c_str() );
    i.m_sLoadData.m_sGeneral.m_bReadSolids = true;
    i.m_sLoadData.m_sGeneral.m_bReadWireframes = true;
    i.m_sLoadData.m_sGeneral.m_eReadGeomTessMode = kA3DReadGeomAndTess;
    loader.Import( i );
    if( nullptr == loader.m_psModelFile ) {
        std::cout << "The specified file was not loaded: " << input_file << std::endl;
        return -1;
    }

    //! [Attaching attributes to B-Rep faces and edges]
    // Get all unique parts from the model file
    auto const parts = ts3d::getUniqueLeafEntities( loader.m_psModelFile, kA3DTypeAsmPartDefinition );
    for( auto const part : parts ) {
        // Attach an arbitrary attribute to the part
        A3DRootBaseAttributeAdd( part, const_cast<char*>("part"), const_cast<char*>("foobar") );
        // Get the B-Rep model(s) from the part definition
        auto const brep_models = ts3d::getLeafInstances( part, kA3DTypeRiBrepModel );
        for( auto const brep_model : brep_models ) {
            // Obtain the scale associated with this model
            ts3d::A3DRiBrepModelWrapper d( brep_model.back() );
            ts3d::A3DTopoBodyWrapper topo_body( d->m_pBrepData );
            ts3d::A3DTopoContextWrapper topo_context( topo_body->m_pContext );
            A3DVector3dData scale;
            A3D_INITIALIZE_DATA( A3DVector3dData, scale );
            scale.m_dX = scale.m_dY = scale.m_dZ = ( topo_context->m_bHaveScale ? topo_context->m_dScale : 1. );

            //! [Getting all faces from an A3DRiBrepModel]
            auto const faces = ts3d::getUniqueLeafEntities( brep_model.back(), kA3DTypeTopoFace );
            for( auto const face : faces ) {
                // add face attributes
                ts3d::attachFaceAttributes( face, scale );
            }
            //! [Getting all faces from an A3DRiBrepModel]
            
            //! [Getting all edges from an A3DRiBrepModel]
            ts3d::InstancePathMap edge_instance_path_map;
            auto const edges = ts3d::getUniqueLeafEntities( brep_model.back(), kA3DTypeTopoEdge, edge_instance_path_map );
            for( auto const edge : edges ) {
                auto const &edge_instances = edge_instance_path_map[edge];
                ts3d::EntitySet owning_faces;
                for( auto const &edge_instance : edge_instances ) {
                    // get the face referencing this edge instance
                    auto const face_it = std::find_if( std::begin( edge_instance ), std::end( edge_instance ), [](A3DEntity *ntt ) {
                        return ts3d::getEntityType( ntt ) == kA3DTypeTopoFace;
                    });
                    if( std::end( edge_instance ) != face_it ) {
                        owning_faces.insert( *face_it );
                    }
                }
                ts3d::attachEdgeAttributes( edge, owning_faces, scale );
            }
            //! [Getting all edges from an A3DRiBrepModel]
        }
    }
    //! [Attaching attributes to B-Rep faces and edges]

    A3DExport e( output_file.c_str() );
    loader.Export( e );
    std::cout << "Success." << std::endl;
    return 0;
}
