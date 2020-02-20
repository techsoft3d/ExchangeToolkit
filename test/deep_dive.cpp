#include <string>
#include <fstream>
#include <iostream>

#include <A3DSDKIncludes.h>

#ifdef _MSC_VER
#pragma warning(disable : 4503)
#endif

#include <ExchangeToolkit.h>
#include <ExchangeEigenBridge.h>

#include "catch.hpp"

#define xstr(s) __str(s)
#define __str(s) #s
static std::string const exchange_path = xstr(HOOPS_EXCHANGE_PATH);

A3DAsmModelFile *getModelFile( std::string const &fn );
void freeModelFile( void );

TEST_CASE( "helloworld.stp deep dive tests", "[ObjectCounts], [Instance], [Traversal], [B-Rep]" ) {
    auto const model_file = getModelFile( exchange_path + "/samples/data/step/helloworld.stp" );
    REQUIRE( model_file != nullptr );
    
    auto const pos = ts3d::getLeafInstances( model_file, kA3DTypeAsmProductOccurrence );
    REQUIRE( pos.size() == 1 );
    
    {
        ts3d::Instance i( pos[0] );
        REQUIRE( i.path() == pos[0] );
        REQUIRE( ts3d::getNetMatrix( i ) == ts3d::MatrixType::Identity() );
        REQUIRE( i.getName() == "Part1" );
    }
    
    auto const ri_brep_models = ts3d::getLeafInstances( model_file, kA3DTypeRiBrepModel );
    REQUIRE( ri_brep_models.size() == 10 );
    for( auto const ri_brep_model_path : ri_brep_models ) {
        auto const brep_datas = ts3d::getUniqueLeafEntities( ri_brep_model_path.back(), kA3DTypeTopoBrepData );
        REQUIRE( brep_datas.size() == 1 );
        auto const brep_data = *brep_datas.begin();
        REQUIRE( nullptr != brep_data );

        auto const faces = ts3d::getLeafInstances( ri_brep_model_path.back(), kA3DTypeTopoFace );
        A3DUns32 n_faces = 0;
        A3DTopoFace **pfaces = nullptr;
        REQUIRE( A3D_SUCCESS == A3DTopoBrepDataGetFaces( brep_data, &n_faces, &pfaces ) );
        UNSCOPED_INFO( "number of faces are correct" );
        REQUIRE( faces.size() == n_faces );
        REQUIRE( A3D_SUCCESS == A3DTopoBrepDataGetFaces( nullptr, &n_faces, &pfaces ) );

        for( auto const face_path : faces ) {
            auto const shell_it = std::find_if( std::begin( face_path ), std::end( face_path ) , [](A3DEntity *ntt) {
                return ts3d::getEntityType( ntt ) == kA3DTypeTopoShell;
            });
            REQUIRE( std::end( face_path ) != shell_it );
            auto const shell = *shell_it;
            
            A3DTopoShell const *pshell = nullptr;
            REQUIRE( A3D_SUCCESS == A3DTopoFaceGetShell( brep_data, face_path.back(), &pshell ) );
            UNSCOPED_INFO( "shell in path is correct" );
            REQUIRE( pshell == shell );
            
            auto const loops = ts3d::getLeafInstances( face_path.back(), kA3DTypeTopoLoop );
            for( auto const loop_path : loops ) {
                A3DTopoFace const *pface = nullptr;
                REQUIRE( A3D_SUCCESS == A3DTopoLoopGetFace( brep_data, loop_path.back(), &pface ) );
                UNSCOPED_INFO( "loop consistency" );
                REQUIRE( pface == face_path.back() );
            }
        }

        auto const edges = ts3d::getUniqueLeafEntities( ri_brep_model_path.back(), kA3DTypeTopoEdge );
        A3DUns32 n_edges = 0;
        A3DTopoEdge **pedges = nullptr;
        REQUIRE( A3D_SUCCESS == A3DTopoBrepDataGetEdges( brep_data, &n_edges, &pedges ) );
        UNSCOPED_INFO( "number of edges are correct" );
        REQUIRE( n_edges == edges.size() );
        REQUIRE( A3D_SUCCESS == A3DTopoBrepDataGetEdges( nullptr, &n_edges, &pedges ) );
    }
    
    auto getRiBrepModelByName = [&ri_brep_models]( std::string const &name ) {
        auto const it = std::find_if( std::begin( ri_brep_models ), std::end( ri_brep_models ), [name](ts3d::EntityArray const &path) {
            ts3d::Instance i( path );
            return i.getName() == name;
        });
        return std::end( ri_brep_models ) == it ? ts3d::EntityArray() : *it;
    };
    
    // testing a single body will suffice
    ts3d::RepresentationItemInstance ri_instance( getRiBrepModelByName( "PartBody.1" ) );
    REQUIRE( ri_instance.getName() == "PartBody.1" );
    if( auto const t = std::dynamic_pointer_cast<ts3d::Tess3DInstance>(ri_instance.getTessellation())) {
        REQUIRE( t->coordsSize() == 768 );
        REQUIRE( t->normalsSize() == 1530 );
        REQUIRE( t->texCoordsSize() == 0 );
    }
}
