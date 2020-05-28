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
#include <cassert>
#include <iostream>

#ifdef _MSC_VER
#pragma warning(disable : 4503)
#endif

#include "ExchangeToolkit.h"

#define xstr(s) __str(s)
#define __str(s) #s

static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

auto const SHOULD_FORCE_RETESSELLATION = false;
auto const TOLERANCE = 1e-2;

int main( int argc, char *argv[] ) {
    auto usage = []{
        std::cerr << "Usage: compare_brep_tess [--tolerance=<t>] <input file>" << std::endl;
        std::cerr << "  <t>           - Threshold value for reporting tessellation/B-Rep comparison failure" << std::endl;
        std::cerr << "  <input file>  - specifies a the files to be read using HOOPS Exchange" << std::endl;
        std::cerr << std::endl;
		std::cerr << "This application reads the specified file and asserts that the tessellation" << std::endl;
		std::cerr << "is consistent with the B-Rep definition. This is done by comparing face counts," << std::endl;
		std::cerr << "loop counts, and edge counts. In addition, the points obtained from the edge" << std::endl;
        std::cerr << "tessellation are projected to their corresponding edges and faces, and the" << std::endl;
        std::cerr << "minimum distances are determined and tested against the provided tolerance." << std::endl;
	};
    
    if( argc != 2 && argc != 3 ) {
        usage();
        return -1;
    }

    if( argc == 3 ) {
        auto const tolerance_switch = argv[1];
        std::cmatch matcher;
        std::regex const expr( "--tolerance=(.*)" );
        if( std::regex_match( tolerance_switch, matcher, expr ) && 2u == matcher.size() ) {
            auto const tolerance = atof( matcher[1].str().c_str() );
            if( tolerance > 0. ) {
                const_cast<double&>( TOLERANCE ) = tolerance;
            } else {
                std::cerr << "Unable to parse tolerance specification." << std::endl;
                usage();
                return -1;
            }
        } else {
            std::cerr << "Unable to parse tolerance specification." << std::endl;
            usage();
            return -1;
        }
    }
    std::cout << "Tolerance: " << TOLERANCE << std::endl;

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
    i.m_sLoadData.m_sGeneral.m_eReadGeomTessMode = SHOULD_FORCE_RETESSELLATION ? kA3DReadGeomOnly : kA3DReadGeomAndTess;


    loader.Import( i );
    if( nullptr == loader.m_psModelFile ) {
        std::cerr << "The specified file could not be loaded." << std::endl;
        return -1;
    }

    auto failure_encountered = false;
    //! [Compare tessellation to b-rep]
    auto const ri_instances = ts3d::getLeafInstances( loader.m_psModelFile, kA3DTypeRiBrepModel );
    for( auto ri_instance : ri_instances ) {
        
        if( SHOULD_FORCE_RETESSELLATION ) {
            A3DRWParamsTessellationData tess_params;
            A3D_INITIALIZE_DATA( A3DRWParamsTessellationData, tess_params );
            tess_params.m_eTessellationLevelOfDetail = kA3DTessLODMedium;
            tess_params.m_bAccurateTessellation = false;
            CheckResult( A3DRiRepresentationItemComputeTessellation( ri_instance.back(), &tess_params ) );
        }
        
        ts3d::A3DRiBrepModelWrapper brep_model_d( ri_instance.back() );
        ts3d::A3DTopoBodyWrapper topo_body_d( brep_model_d->m_pBrepData );
        ts3d::A3DTopoContextWrapper context_d( topo_body_d->m_pContext );
        auto const brep_scale = context_d->m_bHaveScale ? context_d->m_dScale : 1.;
                
        ts3d::RepresentationItemInstance const ri( ri_instance );
        auto const tess3d = std::dynamic_pointer_cast<ts3d::Tess3DInstance>( ri.getTessellation() );
        if( nullptr == tess3d ) {
            continue;
        }

        // ensure the number of tess faces is equal to the number of b-rep faces
        auto const n_tess_faces = tess3d->faceSize();
        auto const brep_faces = ts3d::getLeafInstances( ri_instance.back(), kA3DTypeTopoFace );
        if( n_tess_faces != brep_faces.size() ) {
            std::cerr << "FAILURE: n_tess_faces (" << n_tess_faces << ") != brep_faces.size() (" << brep_faces.size() << ")" << std::endl;
            failure_encountered = true;
        }
        
        // for each face, examine the loops
		auto const max_face_idx = std::min( static_cast<std::size_t>( n_tess_faces ), brep_faces.size() );
        for( auto face_idx = 0u; face_idx < max_face_idx; ++face_idx ) {
            auto const tess_face = tess3d->getIndexMeshForFace( face_idx );
            auto const brep_face = brep_faces[face_idx].back();
            ts3d::A3DTopoFaceWrapper face_d( brep_face );
            auto const surface = face_d->m_pSurface;
        
            // ensure the number of tess loops is equal to the number of b-rep loops
            auto const tess_loops = tess_face.loops();
            auto const n_tess_loops = tess_loops.size();
            auto const brep_loops = ts3d::getLeafInstances( brep_faces[face_idx].back(), kA3DTypeTopoLoop );
            if( n_tess_loops != brep_loops.size() ) {
                std::cerr << "FAILURE: n_tess_loops (" << n_tess_loops << ") != brep_loops.size() (" << brep_loops.size() << ")" << std::endl;
                failure_encountered = true;
            }

            // for each loop, examine the edges
			auto const max_loop_idx = std::min( n_tess_loops, brep_loops.size() );
            for( auto loop_idx = 0u; loop_idx < max_loop_idx; ++loop_idx ) {
                auto const &tess_loop = tess_loops[loop_idx];
                
                // ensure the number of tess edges is equal to the number of b-rep coedges
                auto const n_tess_edges = tess_loop._edges.size();
                auto const brep_coedges = ts3d::getLeafInstances( brep_loops[loop_idx].back(), kA3DTypeTopoCoEdge );
                if( n_tess_edges != brep_coedges.size() ) {
                    std::cerr << "FAILURE: n_tess_edges ( " << n_tess_edges << ") != brep_coedges.size() (" << brep_coedges.size() << ")" << std::endl;
                    failure_encountered = true;
                }
                
				auto const max_edge_idx = std::min(n_tess_edges, brep_coedges.size());
                for( auto edge_idx = 0u; edge_idx < max_edge_idx; ++edge_idx ) {
                    auto const &tess_edge = tess_loop._edges[edge_idx];
                    auto const brep_coedge = brep_coedges[edge_idx];

                    ts3d::A3DTopoCoEdgeWrapper coedge_d( brep_coedge.back() );
                    ts3d::A3DTopoEdgeWrapper edge_d( coedge_d->m_pEdge );
                    A3DCrvBase const *curve = nullptr;
                    CheckResult( A3DTopoEdgeGetOrCompute3DCurve( brep_model_d->m_pBrepData, coedge_d->m_pEdge, &curve ) );

                    A3DIntervalData interval;
                    A3D_INITIALIZE_DATA( A3DIntervalData, interval );
                    if( edge_d->m_bHasTrimDomain ) {
                        interval = edge_d->m_sInterval;
                    } else {
                        CheckResult( A3DCrvGetInterval( curve, &interval ) );
                    }
                    
                    for( auto const vertex_index : tess_edge._vertices ) {
                        A3DVector3dData pt;
                        A3D_INITIALIZE_DATA( A3DVector3dData, pt );
                        pt.m_dX = tess3d->coords()[vertex_index] / brep_scale;
                        pt.m_dY = tess3d->coords()[vertex_index + 1] / brep_scale;
                        pt.m_dZ = tess3d->coords()[vertex_index + 2] / brep_scale;
                    
                        auto min_curve_distance = std::numeric_limits<double>::max();
                        // project the point to the curve
                        A3DUns32 n_solutions = 0u;
                        A3DDouble *parameters = nullptr, *distances = nullptr;
                        if( CheckResult( A3DCrvProjectPoint( curve, &pt, &n_solutions, &parameters, &distances ) ) ) {
                            for( auto n_solution = 0u; n_solution < n_solutions; ++n_solution ) {
                                if( distances[n_solution] < min_curve_distance ) {
                                    min_curve_distance = distances[n_solution];
                                }
                            }
                            CheckResult( A3DCrvProjectPoint( nullptr, &pt, &n_solutions, &parameters, &distances ) );
                        }
                        
                        // check resulting error against tolerance
                        auto const curve_error = min_curve_distance;// * brep_scale;
                        if( curve_error > TOLERANCE ) {
                            std::cerr << "FAILURE: [Distance to curve] " << curve_error << " > " << TOLERANCE << std::endl;
                            failure_encountered = true;
                        }
                        
                        auto min_surf_distance = std::numeric_limits<double>::max();
                        // Project the point to the surface
                        A3DVector2dData *face_solutions = nullptr;
                        if( CheckResult( A3DSurfProjectPoint( surface, &pt, &n_solutions, &face_solutions, &distances ) ) ) {
                            for( auto n_solution = 0u; n_solution < n_solutions; ++n_solution ) {
                                if( distances[n_solution] < min_surf_distance ) {
                                    min_surf_distance = distances[n_solution];
                                }
                            }
                            CheckResult( A3DSurfProjectPoint( nullptr, &pt, &n_solutions, &face_solutions, &distances ) );
                        }
                        
                        // check resulting error against tolerance
                        auto const surface_error = min_surf_distance;// * brep_scale;
                        if( surface_error > TOLERANCE ) {
                            std::cerr << "FAILURE: [Distance to surface] " << surface_error << " > " << TOLERANCE << std::endl;
                            failure_encountered = true;
                        }
                    }
                }
            }
        }
    }
    //! [Compare tessellation to b-rep]
    
    std::cerr << "Tests completed " << (failure_encountered ? "un" : "") << "successfully." << std::endl;
    
    return 0;
}
