#define INITIALIZE_A3D_API
#include "A3DSDKIncludes.h"

#include <locale>
#include <codecvt>
#include <string>
#include <fstream>
#include <regex>
#include <iostream>
#include <cassert>

#ifdef _MSC_VER
#pragma warning(disable : 4503)
#endif

#include "ExchangeToolkit.h"
#include "ExchangeEigenBridge.h"

#define xstr(s) __str(s)
#define __str(s) #s


static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

static auto THRESHOLD = 1e5;

A3DVector3dData getCenter( A3DBoundingBoxData const &bb ) {
    return bb.m_sMin + (bb.m_sMax - bb.m_sMin) * 0.5;
}

A3DVector3dData computeModelFileTranslation( A3DAsmModelFile *mf ) {
    ts3d::InstancePathMap ri_instance_paths;
    auto const ris = ts3d::getUniqueChildren(mf, kA3DTypeRiRepresentationItem, ri_instance_paths );
    auto cnt = 0u;
    A3DVector3dData avg_o;
    A3D_INITIALIZE_DATA( A3DVector3dData, avg_o );
    for( auto ri : ris ) {
        A3DBoundingBoxData tess_bounds;
        A3D_INITIALIZE_DATA(A3DBoundingBoxData, tess_bounds);
        ts3d::A3DRiRepresentationItemWrapper ri_d( ri );
        ts3d::A3DTessBaseWrapper tess_d( ri_d->m_pTessBase );
        auto const n_coords = tess_d->m_uiCoordSize / 3;
        for( auto idx = 0u; idx < n_coords; ++idx ) {
            auto const coord_idx = idx * 3;
            A3DVector3dData pt;
            A3D_INITIALIZE_DATA(A3DVector3dData, pt);
            pt.m_dX = tess_d->m_pdCoords[coord_idx];
            pt.m_dY = tess_d->m_pdCoords[coord_idx + 1];
            pt.m_dZ = tess_d->m_pdCoords[coord_idx + 2];

            if( idx != 0u ) {
                ts3d::include( tess_bounds, pt );
            } else {
                tess_bounds.m_sMin = tess_bounds.m_sMax = pt;
            }
        }
        auto const tess_pt = ts3d::getPosition( tess_bounds.m_sMin );

        for( auto instance_path : ri_instance_paths[ri] ) {
            auto const m = ts3d::getNetMatrix( instance_path );
            auto const global_tess_pt = m * tess_pt;
            avg_o += ts3d::getExchangeVector( global_tess_pt );
            cnt++;
        }

    }
    avg_o /= cnt;
    return avg_o;
}

bool getOrigin( A3DMiscTransformation *t, A3DVector3dData &o ) {
    switch( ts3d::getEntityType( t ) ) {
        case kA3DTypeMiscGeneralTransformation:
        {
            ts3d::A3DMiscGeneralTransformationWrapper d( t );
            o.m_dX = d->m_adCoeff[3];
            o.m_dX = d->m_adCoeff[7];
            o.m_dX = d->m_adCoeff[11];
        }
            break;
        case kA3DTypeMiscCartesianTransformation:
        {
            ts3d::A3DMiscCartesianTransformationWrapper d( t );
            o = d->m_sOrigin;
        }
            break;
        default:
            return false;
            break;
    }
    return true;
}

std::ostream &operator<<(std::ostream &o, A3DVector3dData const &v ) {
    return o << "(" << v.m_dX << ", " << v.m_dY << ", " << v.m_dZ << ")";
}
    

bool shouldTranslate( A3DVector3dData const &pt, A3DVector3dData const &t ) {
    auto const d = ts3d::length( pt );
    return d > THRESHOLD;
}

bool shouldTranslateCoordinateSystem( A3DEntity *ntt, A3DVector3dData const &t ) {
    A3DVector3dData ntt_location;
    A3D_INITIALIZE_DATA( A3DVector3dData, ntt_location );
    auto const base_type = getBaseType( ts3d::getEntityType( ntt ) );
    if( kA3DTypeAsmProductOccurrence == base_type ) {
        if(!getOrigin( getLocation( ntt ), ntt_location ) ) {
            return false;
        }
    } else if( kA3DTypeRiRepresentationItem == base_type ) {
        ts3d::A3DRiRepresentationItemWrapper d( ntt );
        ts3d::A3DRiCoordinateSystemWrapper cs_d( d->m_pCoordinateSystem );
        if( !getOrigin(cs_d->m_pTransformation, ntt_location ) ) {
            return false;
        }
    } else {
        return false;
    }
    return shouldTranslate( ntt_location, t );
}

bool translateCoordinateSystem( A3DEntity *ntt, A3DVector3dData const &t ) {
    A3DMiscTransformation *transf = nullptr, *new_transf = nullptr;
    auto const base_type = getBaseType(ts3d::getEntityType( ntt ) );
    if( kA3DTypeAsmProductOccurrence == base_type ) {
        ts3d::A3DAsmProductOccurrenceWrapper d( ntt );
        if( d->m_pLocation ) {
            transf = d->m_pLocation;
        } else if( d->m_pPrototype ) {
            translateCoordinateSystem( d->m_pPrototype, t );
        } else {
            ts3d::A3DMiscCartesianTransformationWrapper cs;
            cs->m_sScale.m_dX = cs->m_sScale.m_dY = cs->m_sScale.m_dZ = 1.;
            cs->m_sXVector.m_dX = 1.;
            cs->m_sYVector.m_dY = 1.;
            cs->m_ucBehaviour = kA3DTransformationTranslate;
            CheckResult( A3DMiscCartesianTransformationCreate( cs.operator->(), &transf ) );
        }
    } else if( kA3DTypeRiRepresentationItem == base_type ) {
        ts3d::A3DRiRepresentationItemWrapper d( ntt );
        ts3d::A3DRiCoordinateSystemWrapper cs_d( d->m_pCoordinateSystem );
        transf = cs_d->m_pTransformation;
    }
    
    switch( ts3d::getEntityType( transf ) ) {
        case kA3DTypeMiscGeneralTransformation:
        {
            ts3d::A3DMiscGeneralTransformationWrapper d( transf );
            d->m_adCoeff[3] += t.m_dX;
            d->m_adCoeff[7] += t.m_dY;
            d->m_adCoeff[11] += t.m_dZ;
            if( !CheckResult( A3DMiscGeneralTransformationCreate( d.operator->(), &new_transf ) ) ) {
                return false;
            }
        }
            break;
        case kA3DTypeMiscCartesianTransformation:
        {
            ts3d::A3DMiscCartesianTransformationWrapper d( transf );
            d->m_sOrigin += t;
            if( !CheckResult( A3DMiscCartesianTransformationCreate( d.operator->(), &new_transf ) ) ) {
                return false;
            }
        }
            break;
        default:
            break;
    }
 
    if( nullptr == new_transf ) {
        return false;
    }
    
    if( kA3DTypeAsmProductOccurrence == base_type ) {
        ts3d::A3DAsmProductOccurrenceWrapper d( ntt );
        d->m_pLocation = new_transf;
        return CheckResult( A3DAsmProductOccurrenceEdit( d.operator->(), ntt ) );

    } else if( kA3DTypeRiRepresentationItem == base_type ) {
        ts3d::A3DRiRepresentationItemWrapper d( ntt );
        A3DRiCoordinateSystemData cs_d;
        A3D_INITIALIZE_DATA( A3DRiCoordinateSystemData, cs_d );
        cs_d.m_pTransformation = new_transf;
        return CheckResult( A3DRiCoordinateSystemEdit( &cs_d, d->m_pCoordinateSystem ) );
    }
    return false;
}

void translateTessellation( A3DTessBase *tess, A3DVector3dData const &t ) {
    if( nullptr == tess ) {
        return;
    }
    ts3d::A3DTessBaseWrapper tess_d( tess );
    auto const n_coords = tess_d->m_uiCoordSize / 3;
    for( auto idx = 0u; idx < n_coords; ++idx ) {
        auto const coord_idx = idx * 3;
        auto const x = tess_d->m_pdCoords[coord_idx] + t.m_dX;
        auto const y = tess_d->m_pdCoords[coord_idx + 1] + t.m_dY;
        auto const z = tess_d->m_pdCoords[coord_idx + 2] + t.m_dZ;
        
        tess_d->m_pdCoords[coord_idx] = x;
        tess_d->m_pdCoords[coord_idx + 1] = y;
        tess_d->m_pdCoords[coord_idx + 2] = z;
    }
    CheckResult( A3DTessBaseSet( tess, tess_d.operator->() ) );
}
    
bool alreadyTranslated( ts3d::InstancePath const &instance_path, ts3d::EntitySet const &translated_entities ) {
    for( auto ntt : instance_path ) {
        if( std::end( translated_entities ) != translated_entities.find( ntt ) ) {
            return true;
        }
    }
    return false;
}

int main( int argc, char *argv[] ) {
    
    auto usage = []{
        std::cerr << "Usage: normalize_coords <input file> <output file>" << std::endl;
        std::cerr << "  <input file>   - specifies a the files to be read using HOOPS Exchange" << std::endl;
        std::cerr << "  <output file>  - specifies a the files to be written with normalized coordinates." << std::endl;
        std::cerr << std::endl;
        std::cerr << "This application reads the specified file, computes a global center, and normalizes coordinate systems accordingly." << std::endl;
    };

    if( argc != 3 ) {
        usage();
        return -1;
    }

    std::string const input_file = argv[1];
    std::cout << "Input file: " << input_file << std::endl;

    std::string const output_file = argv[2];
    std::cout << "Output file: " << output_file << std::endl;
    
    if( input_file == output_file ) {
        std::cerr << "The output file is the same as the input file. This is not allowed for data safety." << std::endl;
        return -1;
    }

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

    if( !CheckResult( loader.Import( i ) ) ) {
        std::cerr << "Failed to load input file." << std::endl;
        return -1;
    }
    
    std::cout << "Translation threshold is: " << THRESHOLD << std::endl;

    ts3d::InstancePathMap instance_path_map;
    auto const offset = computeModelFileTranslation( loader.m_psModelFile );
    auto const offset_length = ts3d::length( offset );
    if( offset_length < THRESHOLD ) {
        std::cout << "Distance to center (" << offset_length << ") < THRESHOLD. Nothing to do." << std::endl;
        return -1;
    }
    std::cout << "Adjusting origin to: " << offset << std::endl;

    ts3d::A3DAsmModelFileWrapper mf_d( loader.m_psModelFile );
    auto const all_rep_items = ts3d::getUniqueChildren( loader.m_psModelFile, kA3DTypeRiRepresentationItem, instance_path_map );
    std::cout << "Processing " << all_rep_items.size() << " representation item" << (all_rep_items.size() > 1 ? "s." : ".") << std::endl;
    ts3d::EntitySet translated_entities;
    auto normalizations_made = false;
    for( auto const &rep_item : all_rep_items ) {
        std::cout << "Representation item with " << instance_path_map[rep_item].size() << " instance" << (instance_path_map[rep_item].size() > 1 ? "s." : "." ) << std::endl;
        for( auto const &instance_path : instance_path_map[rep_item]) {
            if( alreadyTranslated( instance_path, translated_entities ) ) {
                std::cout << "- Instance path already translated." << std::endl;
                continue;
            }

            auto translated_cs = false;
            for( auto const ntt : instance_path ) {
                if( shouldTranslateCoordinateSystem( ntt, offset ) && translateCoordinateSystem( ntt, -offset ) ) {
                    std::cout << "- Translated instance path." << std::endl;
                    translated_entities.insert( ntt );
                    translated_cs = true;
                    normalizations_made = true;
                    break;
                }
            }

            if( !translated_cs ) {
                ts3d::A3DRiRepresentationItemWrapper d( rep_item );
                translateTessellation( d->m_pTessBase, -offset );
                translated_entities.insert( rep_item );
                normalizations_made = true;
                std::cout << "- Translated tessellation." << std::endl;
            }
        }
    }

    if(!normalizations_made) {
        std::cout << "No normalizations necessary." << std::endl;
        return 0;
    }
    
    std::stringstream ss;
    ss << "{ ";
    ss <<     "offset : { ";
    ss <<         "x : " << offset.m_dX << ", ";
    ss <<         "y : " << offset.m_dY << ", ";
    ss <<         "z : " << offset.m_dZ << " ";
    ss <<    "} ";
    ss << "}";

    CheckResult( A3DRootBaseAttributeAdd( loader.m_psModelFile, const_cast<char*>("origin_offset"), const_cast<char*>(ss.str().c_str()) ) );

    A3DExport e( output_file.c_str() );
    if(!CheckResult( loader.Export( e ) )) {
        return -1;
    }
    
    std::cout << "Success." << std::endl;
	return 0;
}



