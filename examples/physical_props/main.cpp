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
#include "ExchangeEigenBridge.h"

#define xstr(s) __str(s)
#define __str(s) #s

static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

int main( int argc, char *argv[] ) {
    auto usage = []{
        std::cerr << "Usage: physical_props [--density=<d>] <input file>" << std::endl;
        std::cerr << "  <d>           - specifies the default value to use for density (kg/m^3)" << std::endl;
        std::cerr << "  <input file>  - specifies a the files to be read using HOOPS Exchange" << std::endl;
        std::cerr << std::endl;
        std::cerr << "This application reads the specified file and iterates over all A3DRiBrepModel" << std::endl;
        std::cerr << "objects. For each, it computes physical properties using the function" << std::endl;
        std::cerr << "A3DComputePhysicalProperties and prints the results to stdout." << std::endl;
    };
    
    if( argc != 2 && argc != 3 ) {
        usage();
        return -1;
    }
    
    auto const default_density = 7800.;
    if( argc == 3 ) {
        auto const density_switch = argv[1];
        std::cmatch matcher;
        std::regex const expr( "--density=(.*)" );
        if( std::regex_match( density_switch, matcher, expr ) && 2u == matcher.size() ) {
            auto const density = atof( matcher[1].str().c_str() );
            if( density > 0. ) {
                const_cast<double&>( default_density ) = density;
            } else {
                std::cerr << "Unable to parse density specification." << std::endl;
                usage();
                return -1;
            }
        } else {
            std::cerr << "Unable to parse density specification." << std::endl;
            usage();
            return -1;
        }
    }
    std::cout << "Default density (kg/m^3): " << default_density << std::endl;

    std::string const input_file = argv[argc-1];
    std::cout << "Input file: " << input_file << std::endl;
    
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

    double const unit = ts3d::getUnit( loader.m_psModelFile );
    if( 0. == unit ) {
        std::cerr << "Failed to get unit from model file." << std::endl;
        return -1;
    }
    std::cout << "Unit (mm/unit): " << unit  << std::endl;
    auto const to_mm = unit;
    auto const to_mm2 = to_mm * to_mm;
    auto const to_mm3 = to_mm2 * to_mm;
    
    ts3d::A3DAsmModelFileWrapper model_file_data( loader.m_psModelFile );
    auto const invert_diagonals = model_file_data->m_eModellerType == kA3DModellerSlw || model_file_data->m_eModellerType ==  kA3DModellerParasolid;
    if( invert_diagonals ) {
        std::cout << "Inverting off-diagonals due to file type." << std::endl;
    }
    
    ts3d::InstancePathMap instance_paths;
    auto const part_definitions = ts3d::getUniqueLeafEntities( loader.m_psModelFile, kA3DTypeAsmPartDefinition, instance_paths );
    for( auto part_definition : part_definitions ) {
        std::cout << "Processing ";
        auto const part_name = ts3d::Instance( { part_definition } ).getName();
        if( part_name.empty() ) {
            std::cout << "unnammed A3DAsmPartDefinition ";
        } else {
            std::cout << "A3DAsmPartDefinition named \"" << part_name << "\" ";
        }
        std::cout << "(" << instance_paths[part_definition].size() << " instance" << (1 != instance_paths[part_definition].size() ? "s)" : ")") << std::endl;

        // obtain the density for this part
        A3DMiscMaterialPropertiesData mpd;
        A3D_INITIALIZE_DATA(A3DMiscMaterialPropertiesData, mpd);
        A3DMiscGetMaterialProperties( part_definition, &mpd );
        auto const density = mpd.m_dDensity > 0. ? mpd.m_dDensity : default_density;
        std::cout << "Part density (kg/m^3): " << density << (mpd.m_dDensity > 0. ? " (from part)" : " (from default)") << std::endl;
        A3DMiscGetMaterialProperties( nullptr, &mpd );
        
        auto const ri_brep_models = ts3d::getUniqueLeafEntities( part_definition, kA3DTypeRiBrepModel );
        for( auto ri_brep_model : ri_brep_models ) {
            auto const ri_name = ts3d::Instance( { ri_brep_model } ).getName();
            if( ri_name.empty() ) {
                std::cout << "Unnammed A3DRiBrepModel [" << std::endl;
            } else {
                std::cout << "A3DRiBrepModel \"" << ri_name << "\" [" << std::endl;
            }
            
            for( auto instance_path : instance_paths[part_definition] ) {
                std::cout << "\t{" << std::endl;
                ts3d::Instance part_instance( instance_path );
             
                auto const matrix = ts3d::getNetMatrix( part_instance );
                auto const i_scale = (matrix * ts3d::VectorType( 1., 0., 0., 0. )).norm();
                auto const j_scale = (matrix * ts3d::VectorType( 0., 1., 0., 0. )).norm();
                auto const k_scale = (matrix * ts3d::VectorType( 0., 0., 1., 0. )).norm();
                A3DVector3dData scale;
                A3D_INITIALIZE_DATA( A3DVector3dData, scale );
                scale.m_dX = i_scale;
                scale.m_dY = j_scale;
                scale.m_dZ = k_scale;
                A3DPhysicalPropertiesData physical_props;
                A3D_INITIALIZE_DATA( A3DPhysicalPropertiesData, physical_props );
                physical_props.m_bUseGeometryOnRiBRep = true;
                auto const pScale = &scale;
                if( A3D_SUCCESS != A3DComputePhysicalProperties( ri_brep_model, pScale, &physical_props ) ) {
                    std::cerr << "Failure to compute physical properties." << std::endl;
                    continue;
                }
                
                
                if( ! physical_props.m_bVolumeComputed ) {
                    continue;
                }
                
                std::cout << "\t\tVolume (mm^3): " << physical_props.m_dVolume * to_mm3 << std::endl;
                std::cout << "\t\tSurface area (mm^2): " << physical_props.m_dSurface * to_mm2 << std::endl;
                std::cout << "\t\tCOG (mm): " << physical_props.m_sGravityCenter.m_dX * to_mm << ", " << physical_props.m_sGravityCenter.m_dY * to_mm << ", " << physical_props.m_sGravityCenter.m_dZ * to_mm << std::endl;
                
                auto const mass = density * (1e-9) * physical_props.m_dVolume * to_mm3;
                std::cout << "\t\tMass (kg): " << mass  << std::endl;
                
                Eigen::Matrix3d vol_inertia;
                auto const inverter = (invert_diagonals ? -1. : 1.);
                auto const k = density * (1e-9) * to_mm2 * to_mm3;
                vol_inertia << physical_props.m_adVolumeMatrixOfInertia[0] * k, physical_props.m_adVolumeMatrixOfInertia[1] * inverter * k, physical_props.m_adVolumeMatrixOfInertia[2] * inverter * k,
                physical_props.m_adVolumeMatrixOfInertia[3]  * inverter * k, physical_props.m_adVolumeMatrixOfInertia[4] * k, physical_props.m_adVolumeMatrixOfInertia[5] * inverter * k,
                physical_props.m_adVolumeMatrixOfInertia[6] * inverter * k, physical_props.m_adVolumeMatrixOfInertia[7] * inverter *k, physical_props.m_adVolumeMatrixOfInertia[8] * k;
                
                static Eigen::IOFormat tabFormat( Eigen::StreamPrecision, 0, " ", "\n", "\t\t" );
                std::cout << "\t\tLocal moments of inertia (kg mm^2):" << std::endl;
                std::cout << vol_inertia.format( tabFormat ) << std::endl;

                std::cout << "\t}, " << std::endl;
            }
            std::cout << "]" << std::endl;
        }
    }
    return 0;
}
