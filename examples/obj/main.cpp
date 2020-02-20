#define INITIALIZE_A3D_API
#include "A3DSDKIncludes.h"

#include <locale>
#include <codecvt>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef _MSC_VER
#pragma warning(disable:4503)
#endif

#include "ExchangeToolkit.h"
#include "ExchangeEigenBridge.h"

#define xstr(s) __str(s)
#define __str(s) #s

struct MTL_RGB {
    double r, g, b;
    MTL_RGB( double _r = 0., double _g = 0., double _b = 0. ) : r( _r ), g( _g ), b( _b ) {}
};

struct MTL {
    MTL_RGB Ka;
    MTL_RGB Kd;
    MTL_RGB Ks;
    double Tr;
};

bool operator==( MTL_RGB const &lhs, MTL_RGB const &rhs ) {
    return fabs( lhs.r - rhs.r ) < 1e-3 && fabs( lhs.g - rhs.g ) < 1e-3 && fabs( lhs.b - rhs.b ) < 1e-3;
}

std::ostream &operator<<( std::ostream &os, MTL_RGB const &rhs ) {
    os << rhs.r << " " << rhs.g << " " << rhs.b;
    return os;
}

bool operator==( MTL const &lhs, MTL const &rhs ) {
    return lhs.Ka == rhs.Ka && lhs.Kd == rhs.Kd && lhs.Ks == rhs.Ks && fabs( lhs.Tr - rhs.Tr ) < 1e-3;
}

std::string getMaterial( A3DGraphStyleData const &style_data );

static std::vector<MTL> materials;
static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;


int main( int argc, char *argv[] ) {
    if( argc < 3 ) {
        std::cerr << "Usage: obj <input file> <output file>" << std::endl;
        std::cerr << "  <input file>  - specifies a the files to be read using HOOPS Exchange" << std::endl;
        std::cerr << "  <output file> - specifies the name of the .obj file to be written" << std::endl;
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

    std::ofstream obj_file( output_file.c_str() );
    
    auto const mtl_filename = output_file + ".mtl";
    std::ofstream mtl_file( mtl_filename );
    obj_file << "mtllib ./" << mtl_filename.substr( mtl_filename.rfind( DIR_SEP ) + 1 ) << std::endl;
    
    //! [Generate an OBJ file]
    auto const ri_instances = ts3d::getLeafInstances( loader.m_psModelFile, kA3DTypeRiRepresentationItem );
    for( auto ri_instance : ri_instances ) {
        ts3d::RepresentationItemInstance const ri( ri_instance );
        if( !ri.getNetShow() ) {
            continue;
        }

        auto const tess3d = std::dynamic_pointer_cast<ts3d::Tess3DInstance>( ri.getTessellation() );
        if( nullptr == tess3d ) {
            continue;
        }

        auto const name = ri.getName();
        obj_file << "o " << name << std::endl;
        
        auto const net_style = ri.getNetStyle();
        auto const mtl = getMaterial( net_style );
        if( !mtl.empty() ) {
            obj_file << "usemtl " << mtl << std::endl;
        }
        
        auto const net_matrix = ts3d::getNetMatrix( ri );
        auto const exchange_coords = tess3d->coords();
        auto const n_coords = tess3d->coordsSize();
        for( auto idx = 0u; idx < n_coords; idx += 3 ) {
            auto v = net_matrix * ts3d::VectorType( exchange_coords[idx], exchange_coords[idx+1], exchange_coords[idx+2], 1. );
            obj_file << "v " << v(0) << " " << v(1) << " " << v(2) << std::endl;
        }
        
        auto const exchange_normals = tess3d->normals();
        auto const n_normals = tess3d->normalsSize();
        for( auto idx = 0u; idx < n_normals; idx += 3 ) {
            auto n = net_matrix * ts3d::VectorType( exchange_normals[idx], exchange_normals[idx+1], exchange_normals[idx+2], 0. );
            obj_file << "vn " << n(0) << " " << n(1) << " " << n(2) << std::endl;
        }
        
        for( auto idx = 0u; idx < tess3d->faceSize(); ++idx ) {
            auto const face_mesh = tess3d->getIndexMeshForFace( idx );
            auto const n_vertices = face_mesh.vertices().size();
            for(auto idx = 0u; idx < n_vertices; ++idx ) {
                if( 0 == idx % 3 ) {
                    obj_file << std::endl << "f";
                }
                
                obj_file << " -" << (n_coords - face_mesh.vertices()[idx])/3 << "//-" << (n_normals - face_mesh.normals()[idx])/3;
            }
        }
        obj_file << std::endl;
    }
    //! [Generate an OBJ file]
    
    if( ! materials.empty() ) {
        // write the mtl file.
        auto idx = 0u;
        for( auto const mtl : materials ) {
            mtl_file << "newmtl material" << idx++ << std::endl;
            mtl_file << "Ka " << mtl.Ka << std::endl;
            mtl_file << "Kd " << mtl.Kd << std::endl;
            mtl_file << "Tr " << mtl.Tr << std::endl;
            if( mtl.Ks.r > 0. || mtl.Ks.g > 0. || mtl.Ks.b > 0. ) {
                mtl_file << "Ks " << mtl.Ks << std::endl;
                mtl_file << "illum 2" << std::endl;
            } else {
                mtl_file << "illum 1" << std::endl;
            }
        }
    }
}

MTL_RGB getColor( A3DUns32 const &color_idx ) {
    if( A3D_DEFAULT_COLOR_INDEX == color_idx ) {
        return MTL_RGB( 1., 0., 0. );
    }
    A3DGraphRgbColorData MTL_RGB_color_data;
    A3D_INITIALIZE_DATA( A3DGraphRgbColorData, MTL_RGB_color_data );
    A3DGlobalGetGraphRgbColorData( color_idx, &MTL_RGB_color_data );
    auto const &r = MTL_RGB_color_data.m_dRed;
    auto const &g = MTL_RGB_color_data.m_dGreen;
    auto const &b = MTL_RGB_color_data.m_dBlue;
    return MTL_RGB( r, g, b );
}

std::string getMaterial( A3DGraphStyleData const &style_data ) {
    MTL mtl;
    if( style_data.m_bMaterial ) {
        A3DBool is_texuture = false;
        A3DGlobalIsMaterialTexture( style_data.m_uiRgbColorIndex, &is_texuture );
        if( is_texuture ) {
            A3DGraphTextureDefinitionData texture_def;
            A3D_INITIALIZE_DATA( A3DGraphTextureDefinitionData, texture_def );
            A3DGlobalGetGraphTextureDefinitionData( style_data.m_uiRgbColorIndex, &texture_def );
            
            A3DGraphPictureData picture;
            A3D_INITIALIZE_DATA( A3DGraphPictureData, picture );
            A3DGlobalGetGraphPictureData( texture_def.m_uiPictureIndex, &picture );
            
            // TODO deal with textures in a very slick way
            
        } else {
            A3DGraphMaterialData material_data;
            A3D_INITIALIZE_DATA( A3DGraphMaterialData, material_data );
            A3DGlobalGetGraphMaterialData( style_data.m_uiRgbColorIndex, &material_data );
            mtl.Ka = getColor( material_data.m_uiAmbient );
            mtl.Kd = getColor( material_data.m_uiDiffuse );
            mtl.Ks = getColor( material_data.m_uiSpecular );
//            auto const emissive_color = getColor( material_data.m_uiEmissive );
        }
    } else {
        auto const a = static_cast<double>(style_data.m_bIsTransparencyDefined ? style_data.m_ucTransparency : 255u) / 255.;
        mtl.Ka = mtl.Kd = getColor( style_data.m_uiRgbColorIndex );
        mtl.Tr = a;
    }
    
    for( auto idx = 0u; idx < materials.size(); ++idx ) {
        if( materials[idx] == mtl ) {
            std::stringstream str;
            str << "material" << idx;
            return str.str();
        }
    }
    
    auto const sz = materials.size();
    materials.push_back( mtl );
    std::stringstream str;
    str << "material" << sz;
    return str.str();
}
