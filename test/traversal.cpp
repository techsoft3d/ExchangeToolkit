
#include <string>
#include <fstream>
#include <iostream>

#include <A3DSDKIncludes.h>

#ifdef _MSC_VER
#pragma warning(disable : 4503)
#endif

#include <ExchangeToolkit.h>
#include "catch.hpp"

#define xstr(s) __str(s)
#define __str(s) #s
static std::string const exchange_path = xstr(HOOPS_EXCHANGE_PATH);

A3DAsmModelFile *getModelFile( std::string const &fn );
void freeModelFile( void );

TEST_CASE( "High level product traversal tests", "[Traversal]" ) {
    auto const input_file = GENERATE( exchange_path + "/samples/data/catiaV5/CV5_Aquo_Bottle/_Aquo Bottle.CATProduct",
                                     exchange_path + "/samples/data/catiaV5/CV5_Micro_Engine/_micro engine.CATProduct",
                                     exchange_path + "/samples/data/catiaV5/CV5_Landing Gear Model/_LandingGear.CATProduct",
                                     exchange_path + "/samples/data/drawing/Carter.CATDrawing",
                                     exchange_path + "/samples/data/inventor/collision.iam",
                                     exchange_path + "/samples/data/jt/Flange287.jt",
                                     exchange_path + "/samples/data/pmi/PMI_Sample/asm/CV5_Assy_Sample_.CATProduct",
                                     exchange_path + "/samples/data/pmi/PMI_Sample/CV5_Sample.CATPart",
                                     exchange_path + "/samples/data/prc/__drill.prc",
                                     exchange_path + "/samples/data/solidworks/SLW_Diskbrakeassembly/_DiskBrakeAssembly-01FINAL.SLDASM",
                                     exchange_path + "/samples/data/step/Flange287.stp" );
    
    auto const model_file = getModelFile( input_file );
    REQUIRE( model_file != nullptr );
    
    auto const root_pos = ts3d::getChildren( model_file, kA3DTypeAsmProductOccurrence );
    UNSCOPED_INFO( "one root product occurrence" );
    
    REQUIRE( 1 == root_pos.size() );
    
    auto const leaf_entity_type = GENERATE( kA3DTypeAsmProductOccurrence, kA3DTypeAsmPartDefinition, kA3DTypeRiBrepModel, kA3DTypeRiSet );
    SECTION( "leaf instance tests" ) {
        auto const leaf_pos = ts3d::getLeafInstances( model_file, leaf_entity_type );
        
        for( auto const path : leaf_pos ) {
            UNSCOPED_INFO( "instance path has appropriate size" );
            REQUIRE( path.size() >= 1 );
        }
        
        
        for( auto const path : leaf_pos ) {
            UNSCOPED_INFO( "the first entity type is a model file" );
            REQUIRE( path.front() == model_file );
        }
        
        for( auto const path : leaf_pos ) {
            UNSCOPED_INFO( "the leaf product occurrence path is not empty" );
            REQUIRE( path.size() >= 1 );
            
            UNSCOPED_INFO( "the first entity type is a model file" );
            REQUIRE( ts3d::getEntityType( path.front() ) == kA3DTypeAsmModelFile );
            
            UNSCOPED_INFO( "the last entity type is correct " );
            REQUIRE( ts3d::getEntityType( path.back() ) == leaf_entity_type );
        }
    }
    
    SECTION( "unique children tests" ) {
        ts3d::InstancePathMap instance_path_map;
        auto const child_pos = ts3d::getUniqueLeafEntities( model_file, leaf_entity_type, instance_path_map );

        for( auto const child_po : child_pos ) {
            UNSCOPED_INFO( "unique child is correct type" );
            REQUIRE( ts3d::getEntityType( child_po ) == leaf_entity_type );
        }
        
        
        for( auto const child_po : child_pos ) {
            auto const it = instance_path_map.find( child_po );
            UNSCOPED_INFO( "child is present in the instance path map" );
            REQUIRE( std::end( instance_path_map ) != it );
            
            auto const &paths = it->second;
            UNSCOPED_INFO( "there is at least one instanc epath for the unique child" );
            REQUIRE( paths.size() >= 1 );
            
            for( auto const path : paths ) {
                UNSCOPED_INFO( "the first entity type is a model file" );
                REQUIRE( ts3d::getEntityType( path.front() ) == kA3DTypeAsmModelFile );
                
                UNSCOPED_INFO( "the last entity is the unique leaf entity" );
                REQUIRE( path.back() == child_po );
            }
        }
    }
}
