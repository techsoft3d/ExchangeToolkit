#ifdef _MSC_VER
#define NOMINMAX
#endif

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

void testLinkedItemConsistency( ts3d::InstancePathArray const &all_markups ) {
    for( auto const this_markup : all_markups ) {
        auto const linked_items = ts3d::getLeafInstances( this_markup.back(), kA3DTypeMiscMarkupLinkedItem );
        for( auto const linked_item : linked_items ) {
            ts3d::A3DMiscMarkupLinkedItemWrapper d( linked_item.back() );
            if( kA3DTypeMiscReferenceOnTopology != ts3d::getEntityType( d->m_pReference ) ) {
                continue;
            }
            
            ts3d::A3DMiscReferenceOnTopologyWrapper t( d->m_pReference );
            auto const topo_brep_data_ptr = t->m_pBrepData;
            REQUIRE( nullptr != topo_brep_data_ptr );
            
            switch( t->m_eTopoItemType ) {
                case kA3DTypeTopoConnex:
                    REQUIRE( t->m_uiSize == 1 );
                    REQUIRE( ts3d::getLeafInstances( topo_brep_data_ptr, kA3DTypeTopoConnex ).size() > t->m_puiAdditionalIndexes[0] );
                    break;
                case kA3DTypeTopoShell:
                    REQUIRE( t->m_uiSize == 1 );
                    REQUIRE( ts3d::getLeafInstances( topo_brep_data_ptr, kA3DTypeTopoShell ).size() > t->m_puiAdditionalIndexes[0] );
                    break;
                case kA3DTypeTopoFace:
                    REQUIRE( t->m_uiSize == 1 );
                    REQUIRE( ts3d::getLeafInstances( topo_brep_data_ptr, kA3DTypeTopoFace ).size() > t->m_puiAdditionalIndexes[0] );
                    break;
                case kA3DTypeTopoEdge:
                case kA3DTypeTopoCoEdge:
                    REQUIRE( t->m_uiSize == 3 );
                {
                    auto const faces = ts3d::getLeafInstances( topo_brep_data_ptr, kA3DTypeTopoFace );
                    REQUIRE( faces.size() > t->m_puiAdditionalIndexes[0] );
                    auto const face = faces[t->m_puiAdditionalIndexes[0]].back();
                    
                    auto const loops = ts3d::getLeafInstances( face, kA3DTypeTopoLoop );
                    REQUIRE( loops.size() > t->m_puiAdditionalIndexes[1] );
                    auto const loop = loops[t->m_puiAdditionalIndexes[1]].back();
                    
                    auto const coedges = ts3d::getLeafInstances( loop, kA3DTypeTopoCoEdge );
                    REQUIRE( coedges.size() > t->m_puiAdditionalIndexes[2] );
                }
                    break;
                case kA3DTypeTopoUniqueVertex:
                case kA3DTypeTopoMultipleVertex:
                        REQUIRE( t->m_uiSize == 4 );
                    {
                        auto const faces = ts3d::getLeafInstances( topo_brep_data_ptr, kA3DTypeTopoFace );
                        REQUIRE( faces.size() > t->m_puiAdditionalIndexes[0] );
                        auto const face = faces[t->m_puiAdditionalIndexes[0]].back();
                        
                        auto const loops = ts3d::getLeafInstances( face, kA3DTypeTopoLoop );
                        REQUIRE( loops.size() > t->m_puiAdditionalIndexes[1] );
                        auto const loop = loops[t->m_puiAdditionalIndexes[1]].back();
                        
                        auto const edges = ts3d::getLeafInstances( loop, kA3DTypeTopoEdge );
                        REQUIRE( edges.size() > t->m_puiAdditionalIndexes[2] );
                        auto const edge = edges[t->m_puiAdditionalIndexes[2]].back();
                        
                        auto const vertices = ts3d::getLeafInstances( edge, kA3DTypeTopoVertex );
                        REQUIRE( vertices.size() > t->m_puiAdditionalIndexes[3] );
                    }
                    break;
                default:
                    break;
            }
        }
    }

}

TEST_CASE( "CV5_Sample.CATPart deep dive tests", "[PMI], [Traversal]" ) {
    auto const model_file = getModelFile( exchange_path + "/samples/data/pmi/PMI_Sample/CV5_Sample.CATPart" );
    REQUIRE( model_file != nullptr );

    
    auto const annotation_references = ts3d::getLeafInstances( model_file, kA3DTypeMkpAnnotationReference );
    REQUIRE( annotation_references.size() == 3 );
    
    static size_t num_linked_items[] = { 1, 2, 1 };
    for( auto idx = 0u; idx < annotation_references.size(); ++idx ) {
        auto const &annotation_reference_path = annotation_references[idx];
        auto const linked_items = ts3d::getLeafInstances( annotation_reference_path.back(), kA3DTypeMiscMarkupLinkedItem );
        REQUIRE( linked_items.size() == num_linked_items[idx] );
    }

    auto const all_markups = ts3d::getLeafInstances( model_file, kA3DTypeMkpMarkup );
    REQUIRE( 20 == all_markups.size() );
    
    testLinkedItemConsistency( all_markups );
    
    auto const rich_text_markups = ts3d::getLeafInstances( model_file, kA3DTypeMarkupRichText );
    REQUIRE( 5 == rich_text_markups.size() );
    
    auto const roughness_markups = ts3d::getLeafInstances( model_file, kA3DTypeMarkupRoughness );
    REQUIRE( 1 == roughness_markups.size() );
    
    auto const datum_markups = ts3d::getLeafInstances( model_file, kA3DTypeMarkupDatum );
    REQUIRE( 3 == datum_markups.size() );
    
    auto const gdt_markups = ts3d::getLeafInstances( model_file, kA3DTypeMarkupGDT );
    REQUIRE( 4 == gdt_markups.size() );
    
    auto const dimension_markups = ts3d::getLeafInstances( model_file, kA3DTypeMarkupDimension );
    REQUIRE( 7 == dimension_markups.size() );
    
    auto const markup_views = ts3d::getLeafInstances( model_file, kA3DTypeMkpView );
    REQUIRE( 6 == markup_views.size() );
}

TEST_CASE( "Proe_sample.prt deep dive tests", "[PMI], [Traversal]" ) {
    auto const model_file = getModelFile( exchange_path + "/samples/data/pmi/PMI_Sample/Proe_sample.prt" );
    REQUIRE( model_file != nullptr );
    
    auto const all_markups = ts3d::getLeafInstances( model_file, kA3DTypeMkpMarkup );
    REQUIRE( 23 == all_markups.size() );
    
    testLinkedItemConsistency( all_markups );

    auto const rich_text_markups = ts3d::getLeafInstances( model_file, kA3DTypeMarkupRichText );
    REQUIRE( 4 == rich_text_markups.size() );
    
    auto const roughness_markups = ts3d::getLeafInstances( model_file, kA3DTypeMarkupRoughness );
//    REQUIRE( 2 == roughness_markups.size() );
    
    auto const datum_markups = ts3d::getLeafInstances( model_file, kA3DTypeMarkupDatum );
    REQUIRE( 1 == datum_markups.size() );
    
    auto const gdt_markups = ts3d::getLeafInstances( model_file, kA3DTypeMarkupGDT );
    REQUIRE( 4 == gdt_markups.size() );
    
    auto const dimension_markups = ts3d::getLeafInstances( model_file, kA3DTypeMarkupDimension );
    REQUIRE( 10 == dimension_markups.size() );
    
    auto const markup_views = ts3d::getLeafInstances( model_file, kA3DTypeMkpView );
    REQUIRE( 3 == markup_views.size() );
}

TEST_CASE( "CV5_Assy_Sample_.CATProduct deep dive tests", "[PMI], [Traversal]" ) {
    auto const model_file = getModelFile( exchange_path + "/samples/data/pmi/PMI_Sample/asm/CV5_Assy_Sample_.CATProduct" );
    REQUIRE( model_file != nullptr );

    auto const all_markups = ts3d::getLeafInstances( model_file, kA3DTypeMkpMarkup );
    REQUIRE( 28 == all_markups.size() );
    
    testLinkedItemConsistency( all_markups );

    auto const rich_text_markups = ts3d::getLeafInstances( model_file, kA3DTypeMarkupRichText );
    REQUIRE( 7 == rich_text_markups.size() );
    
    auto const roughness_markups = ts3d::getLeafInstances( model_file, kA3DTypeMarkupRoughness );
    REQUIRE( 1 == roughness_markups.size() );
    
    auto const datum_markups = ts3d::getLeafInstances( model_file, kA3DTypeMarkupDatum );
    REQUIRE( 4 == datum_markups.size() );
    
    auto const gdt_markups = ts3d::getLeafInstances( model_file, kA3DTypeMarkupGDT );
    REQUIRE( 5 == gdt_markups.size() );
    
    auto const dimension_markups = ts3d::getLeafInstances( model_file, kA3DTypeMarkupDimension );
    REQUIRE( 11 == dimension_markups.size() );
    
    auto const markup_views = ts3d::getLeafInstances( model_file, kA3DTypeMkpView );
    REQUIRE( 7 == markup_views.size() );
}
