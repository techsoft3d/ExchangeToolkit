#ifdef _MSC_VER
#define NOMINMAX
#endif

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

A3DAsmPartDefinition *getPartDefinition( ts3d::InstancePath const &instance_path );

int main( int argc, char *argv[] ) {
    auto usage = []{
        std::cerr << "Usage: pmi_linked_items <input file>" << std::endl;
        std::cerr << "  <input file>  - specifies a the files to be read using HOOPS Exchange" << std::endl;
        std::cerr << std::endl;
        std::cerr << "This application reads the specified file and examines all PMI for linked items." << std::endl;
        std::cerr << "Each linked item is examined and additional information from the linked B-Rep and" << std::endl;
        std::cerr << "tessellation is printed to stdout." << std::endl;
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

    // First we load the file
    A3DImport i( input_file.c_str() );
    i.m_sLoadData.m_sGeneral.m_bReadSolids = true;
    i.m_sLoadData.m_sGeneral.m_bReadWireframes = false;
    i.m_sLoadData.m_sGeneral.m_bReadPmis = true;
    i.m_sLoadData.m_sGeneral.m_eReadGeomTessMode = kA3DReadGeomAndTess;

    loader.Import( i );
    if( nullptr == loader.m_psModelFile ) {
        std::cerr << "The specified file could not be loaded." << std::endl;
        return -1;
    }

    //! [Dump markup linked items]
    // Use the function to get all markup objects
    auto const all_markups = ts3d::getLeafInstances( loader.m_psModelFile, kA3DTypeMkpMarkup );
    std::cout << "This file contains " << all_markups.size() << " markups." << std::endl;

    // Loop over each markup object and examine if each has any linked items
    for( auto const this_markup : all_markups ) {
        ts3d::Instance markup_instance( this_markup );
        auto const linked_items = ts3d::getLeafInstances( this_markup.back(), kA3DTypeMiscMarkupLinkedItem );
        if( ! linked_items.empty() ) {
            std::cout << "Mark-up: \"" << markup_instance.getName() << "\" [" << markup_instance.getType() << "]" << std::endl;
        }
        
        for( auto const linked_item : linked_items ) {
            ts3d::A3DMiscMarkupLinkedItemWrapper d( linked_item.back() );
            if( kA3DTypeMiscReferenceOnTopology != ts3d::getEntityType( d->m_pReference ) ) {
                // This code is only handling linked items that reference topology
                std::cout << "Unhandled reference type: " << ts3d::Instance( { d->m_pReference } ).getType() << std::endl;
                continue;
            }
            
            // we'll use this data wrapper to obtain info about the topology reference
            ts3d::A3DMiscReferenceOnTopologyWrapper t( d->m_pReference );
            auto const topo_brep_data_ptr = t->m_pBrepData;
            if( nullptr == topo_brep_data_ptr ) {
                continue;
            }

            // Do something unique for each possible type of referenced topology
            switch( t->m_eTopoItemType ) {
                case kA3DTypeTopoConnex:
                    {
                        auto const connex = ts3d::getLeafInstances( topo_brep_data_ptr, t->m_eTopoItemType )[t->m_puiAdditionalIndexes[0]];
                        ts3d::A3DTopoConnexWrapper d( connex.back() );
                        std::cout << "The linked connex contains " << d->m_uiShellSize << " shell(s)." << std::endl;
                    }
                    break;
                case kA3DTypeTopoShell:
                    {
                        auto const shell = ts3d::getLeafInstances( topo_brep_data_ptr, t->m_eTopoItemType )[t->m_puiAdditionalIndexes[0]];
                        ts3d::A3DTopoShellWrapper d( shell.back() );
                        std::cout << "The linked shell contains " << d->m_uiFaceSize << " face(s)." << std::endl;
                    }
                    break;
                case kA3DTypeTopoFace:
                    {
                        // Print some info about the B-Rep
                        auto const face = ts3d::getLeafInstances( topo_brep_data_ptr, t->m_eTopoItemType )[t->m_puiAdditionalIndexes[0]];
                        ts3d::A3DTopoFaceWrapper d( face.back() );
                        std::cout << "The linked face has a surface type: " << ts3d::Instance( { d->m_pSurface } ).getType() << " and contains " << d->m_uiLoopSize << " loop(s)." << std::endl;
                        
                        // Print some info about the associated tessellation
                        auto const part_def = getPartDefinition( this_markup );
                        if( nullptr != part_def ) {
                            auto const ri_brep_models = ts3d::getLeafInstances( part_def, kA3DTypeRiBrepModel );
                            if( ! ri_brep_models.empty() ) {
                                auto const ri_brep_model = ri_brep_models.back();

                                ts3d::RepresentationItemInstance ri_instance( { ri_brep_model } );
                                if( auto const tess = std::dynamic_pointer_cast<ts3d::Tess3DInstance>( ri_instance.getTessellation() ) ) {
                                    auto const index_mesh = tess->getIndexMeshForFace( t->m_puiAdditionalIndexes[0] );
                                    std::cout << "The face's tessellation contains " << index_mesh.vertices().size()/3u << " triangles." << std::endl;
                                }
                            }
                        }
                    }
                    break;
                case kA3DTypeTopoEdge:
                case kA3DTypeTopoCoEdge:
                {
                    // Print some info about the B-Rep
                    auto const faces = ts3d::getLeafInstances( topo_brep_data_ptr, kA3DTypeTopoFace );
                    auto const face = faces[t->m_puiAdditionalIndexes[0]].back();

                    auto const loops = ts3d::getLeafInstances( face, kA3DTypeTopoLoop );
                    auto const loop = loops[t->m_puiAdditionalIndexes[1]].back();

                    auto const coedges = ts3d::getLeafInstances( loop, kA3DTypeTopoCoEdge );
                    auto const coedge = coedges[t->m_puiAdditionalIndexes[2]].back();
                    ts3d::A3DTopoCoEdgeWrapper d( coedge );
                    
                    auto const curve = d->m_pUVCurve ? d->m_pUVCurve : ts3d::A3DTopoEdgeWrapper( d->m_pEdge )->m_p3dCurve;
                    
                    std::cout << "The linked coedge has a curve type: " << ts3d::Instance( { curve } ).getType() << std::endl;
                    
                    // Print some info about the associated tessellation
                    auto const part_def = getPartDefinition( this_markup );
                    if( nullptr != part_def ) {
                        auto const ri_brep_models = ts3d::getLeafInstances( part_def, kA3DTypeRiBrepModel );
                        if( ! ri_brep_models.empty() ) {
                            auto const ri_brep_model = ri_brep_models.back();

                            ts3d::RepresentationItemInstance ri_instance( { ri_brep_model } );
                            if( auto const tess = std::dynamic_pointer_cast<ts3d::Tess3DInstance>( ri_instance.getTessellation() ) ) {
                                auto const index_mesh = tess->getIndexMeshForFace( t->m_puiAdditionalIndexes[0] );
                                auto const tess_loop = index_mesh.loops()[t->m_puiAdditionalIndexes[1]];
                                auto const tess_edge = tess_loop._edges[t->m_puiAdditionalIndexes[2]];
                                std::cout << "The edge's tessellation contains " << tess_edge._vertices.size() << " points." << std::endl;
                            }
                        }
                    }
                }
                    break;
                case kA3DTypeTopoUniqueVertex:
                case kA3DTypeTopoMultipleVertex:
                    {
                        auto const faces = ts3d::getLeafInstances( topo_brep_data_ptr, kA3DTypeTopoFace );
                        auto const face = faces[t->m_puiAdditionalIndexes[0]].back();

                        auto const loops = ts3d::getLeafInstances( face, kA3DTypeTopoLoop );
                        auto const loop = loops[t->m_puiAdditionalIndexes[1]].back();

                        auto const edges = ts3d::getLeafInstances( loop, kA3DTypeTopoEdge );
                        auto const edge = edges[t->m_puiAdditionalIndexes[2]].back();

                        auto const vertices = ts3d::getLeafInstances( edge, kA3DTypeTopoVertex );
                        auto const vertex = vertices[t->m_puiAdditionalIndexes[3]].back();
                        auto const vertex_type = ts3d::getEntityType( vertex );
                        if( kA3DTypeTopoUniqueVertex == vertex_type ) {
                            ts3d::A3DTopoUniqueVertexWrapper d( vertex );
                            std::cout << "The linked vertex is at (" << d->m_sPoint.m_dX << ", " << d->m_sPoint.m_dY << ", " << d->m_sPoint.m_dZ << ")" << std::endl;
                        } else {
                            // kA3DTypeTopoMultipleVertex
                            ts3d::A3DTopoMultipleVertexWrapper d( vertex );
                            std::cout << "The linked vertex has multiple positions at: " << std::endl;
                            for( auto idx = 0u; idx < d->m_uiSize; ++idx ) {
                                std::cout << "(" << d->m_pPts[idx].m_dX << ", " << d->m_pPts[idx].m_dY << ", " << d->m_pPts[idx].m_dZ << ")" << std::endl;
                            }
                        }
                    }
                    break;
                default:
                    break;
            }
        }
        std::cout << std::endl;
    }
    //! [Dump markup linked items]
}

A3DAsmPartDefinition *getPartDefinition( ts3d::InstancePath const &instance_path ) {
    
    // Part level PMI
    auto const part_definition_it = std::find_if( instance_path.rbegin(), instance_path.rend(), [](A3DEntity *ntt) {
        return kA3DTypeAsmPartDefinition == ts3d::getEntityType( ntt );
    });
    
    if( instance_path.rend() != part_definition_it ) {
        return *part_definition_it;
    }
    
    // Assembly level PMI
    auto const po_it = std::find_if( instance_path.rbegin(), instance_path.rend(), [](A3DEntity *ntt) {
        return kA3DTypeAsmProductOccurrence == ts3d::getEntityType( ntt );
    });
    
    if( instance_path.rend() != po_it ) {
        return ts3d::getPartDefinition( *po_it );
    }
    
    return nullptr;
}
