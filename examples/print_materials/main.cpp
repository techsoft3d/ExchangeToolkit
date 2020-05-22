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
#include "nlohmann/json.hpp"

using json = nlohmann::json;

#define xstr(s) __str(s)
#define __str(s) #s

bool operator==(A3DMiscMaterialPropertiesData const &lhs, A3DMiscMaterialPropertiesData const &rhs ) {
    return 0 == memcmp(&lhs, &rhs, sizeof(A3DMiscMaterialPropertiesData) );
}

bool operator!=(A3DMiscMaterialPropertiesData const &lhs, A3DMiscMaterialPropertiesData const &rhs ) {
    return !(lhs == rhs);
}

bool operator==(A3DGraphStyleData const &lhs, A3DGraphStyleData const &rhs ) {
    return 0 == memcmp(&lhs, &rhs, sizeof(A3DGraphStyleData) );
}

bool operator!=(A3DGraphStyleData const &lhs, A3DGraphStyleData const &rhs ) {
    return !(lhs == rhs);
}

A3DGraphRgbColorData getGlobalColorByIndex( A3DUns32 const &idx ) {
    A3DGraphRgbColorData rgb_data;
    A3D_INITIALIZE_DATA( A3DGraphRgbColorData, rgb_data );
    CheckResult( A3DGlobalGetGraphRgbColorData( idx, &rgb_data ) );
    return rgb_data;
}

void to_json( json &j, A3DVector3dData const &v ) {
    j = json{
        { "x", v.m_dX },
        { "y", v.m_dY },
        { "z", v.m_dZ }
    };
}

void to_json( json &j, A3DMiscCartesianTransformationData const &cart_data ) {
    j = json{
        { "origin", cart_data.m_sOrigin },
        { "u", cart_data.m_sXVector },
        { "v", cart_data.m_sYVector },
        { "scale", cart_data.m_sScale }
    };
}

void to_json( json &j, A3DGraphTextureTransformationData const &tex_transf_data ) {
    j = json {
        { "is_2d", static_cast<bool>(tex_transf_data.m_bIs2D) },
        { "flip_s", static_cast<bool>(tex_transf_data.m_bTextureFlipS) },
        { "flip_t", static_cast<bool>(tex_transf_data.m_bTextureFlipT) },
        { "matrix", ts3d::toVector( tex_transf_data.m_dMatrix, 16 ) }
    };
}

void to_json( json &j, A3DGraphRgbColorData const &rgb_data ) {
    j = json{ { "r", rgb_data.m_dRed }, { "g", rgb_data.m_dGreen }, { "b", rgb_data.m_dBlue } };
}

void to_json( json &j, A3DGraphMaterialData const &material_data ) {
    j = json{ { "ambient", getGlobalColorByIndex( material_data.m_uiAmbient ) },
        { "ambient_alpha", material_data.m_dAmbientAlpha },
        { "diffuse", getGlobalColorByIndex( material_data.m_uiDiffuse ) },
        { "diffuse_alpha", material_data.m_dDiffuseAlpha },
        { "emissive", getGlobalColorByIndex( material_data.m_uiEmissive ) },
        { "emissive_alpha", material_data.m_dEmissiveAlpha },
        { "specular", getGlobalColorByIndex( material_data.m_uiSpecular ) },
        { "specular_alpha", material_data.m_dSpecularAlpha },
        { "shininess", material_data.m_dShininess } };
}

void to_json( json &j, A3DEPictureDataFormat const &f ) {
    switch( f ) {
        case kA3DPicturePng:
            j = "png";
            break;
        case kA3DPictureBmp:
            j = "bmp";
            break;
        case kA3DPictureJpg:
            j = "jpg";
            break;
        case kA3DPictureBitmapRgbByte:
            j = "rgb_byte";
            break;
        case kA3DPictureBitmapGreyByte:
            j = "grey_byte";
            break;
        case kA3DPictureBitmapRgbaByte:
            j = "rgba_byte";
            break;
        case kA3DPictureBitmapGreyaByte:
            j = "greya_byte";
            break;
        default:
            j = "unknown";
            break;
    }
}

void to_json( json &j, A3DETextureBlendParameter const &texture_blend_param ) {
    switch (texture_blend_param) {
        case kA3DTextureBlendParameterUnknown:
            j = "unknown";
            break;
        case kA3DTextureBlendParameterZero:
            j = "zero";
            break;
        case kA3DTextureBlendParameterOne:
            j = "one";
            break;
        case kA3DTextureBlendParameterSrcColor:
            j = "src_color";
            break;
        case kA3DTextureBlendParameterOneMinusSrcColor:
            j = "one_minus_src_color";
            break;
        case  kA3DTextureBlendParameterDstColor:
            j = "dst_color";
            break;
        case kA3DTextureBlendParameterOneMinusDstColor:
            j = "one_minus_dst_color";
            break;
        case  kA3DTextureBlendParameterSrcAlpha:
            j = "src_alpha";
            break;
        case kA3DTextureBlendParameterOneMinusSrcAlpha:
            j = "one_minus_src_alpha";
            break;
        case kA3DTextureBlendParameterDstAlpha:
            j = "dst_alpha";
            break;
        case kA3DTextureBlendParameterOneMinusDstAlpha:
            j = "one_minus_dst_alpha";
            break;
        case kA3DTextureBlendParameterSrcAlphaSaturate:
            j = "src_alpha_saturate";
            break;
    }
}

void to_json( json &j, A3DETextureMappingOperator const &texture_mapping_op ) {
    switch( texture_mapping_op ) {
        case kA3DTextureMappingOperatorUnknown:
            j = "unknown";
            break;
        case kA3DTextureMappingOperatorPlanar:
            j = "planar";
            break;
        case kA3DTextureMappingOperatorCylindrical:
            j = "cylindrical";
            break;
        case kA3DTextureMappingOperatorSpherical:
            j = "spherical";
            break;
        case kA3DTextureMappingOperatorCubical:
            j = "cubical";
            break;
    }
}

void to_json( json &j, A3DETextureMappingType const &texture_mapping_type ) {
    switch( texture_mapping_type ) {
        case kA3DTextureMappingTypeUnknown:
            j = "unknown";
            break;
        case kA3DTextureMappingTypeStored:
            j = "stored";
            break;
        case kA3DTextureMappingTypeParametric:
            j = "parametric";
            break;
        case kA3DTextureMappingTypeOperator:
            j = "operator";
            break;
    }
}

void to_json( json &j, A3DETextureAlphaTest const &texture_alpha_test ) {
    switch( texture_alpha_test ) {
        case kA3DTextureAlphaTestUnknown:
            j = "unknown";
            break;
        case kA3DTextureAlphaTestNever:
            j = "never";
            break;
        case kA3DTextureAlphaTestLess:
            j = "less_than";
            break;
        case kA3DTextureAlphaTestEqual:
            j = "equal";
            break;
        case kA3DTextureAlphaTestLequal:
            j = "less_than_equal";
            break;
        case kA3DTextureAlphaTestGreater:
            j = "greater_than";
            break;
        case kA3DTextureAlphaTestNotequal:
            j = "not_equal";
            break;
        case kA3DTextureAlphaTestGequal:
            j = "greater_than_equal";
            break;
        case kA3DTextureAlphaTestAlways:
            j = "always";
            break;
    }
}

void to_json( json &j, A3DETextureFunction const &texture_function ) {
    switch( texture_function ) {
        case kA3DTextureFunctionUnknown:
            j = "unknown";
            break;
        case kA3DTextureFunctionModulate:
            j = "modulate";
            break;
        case kA3DTextureFunctionReplace:
            j = "replace";
            break;
        case kA3DTextureFunctionBlend:
            j = "blend";
            break;
        case kA3DTextureFunctionDecal:
            j = "decal";
            break;
    }
}

void to_json( json &j, A3DETextureWrappingMode const texture_wrapping_mode ) {
    switch( texture_wrapping_mode ) {
        case kA3DTextureWrappingModeUnknown:
            j = "unknown";
            break;
        case kA3DTextureWrappingModeRepeat:
            j = "repeat";
            break;
        case kA3DTextureWrappingModeClampToBorder:
            j = "clamp_to_border";
            break;
        case kA3DTextureWrappingModeClamp:
            j = "clamp";
            break;
        case kA3DTextureWrappingModeClampToEdge:
            j = "clamp_to_edge";
            break;
        case kA3DTextureWrappingModeMirroredRepeat:
            j = "mirrored_repeat";
            break;
    }
}

void to_json( json &j, A3DGraphPictureData const &picture_data ) {
    j = json{
        { "width", picture_data.m_uiPixelWidth },
        { "height", picture_data.m_uiPixelHeight },
        { "format", picture_data.m_eFormat },
        { "size", picture_data.m_uiSize }
    };
}

json mappingAttributesToJson( A3DUns32 const &mapping_attributes ) {
    json j;
    if( mapping_attributes & kA3DTextureMappingAmbient ) {
        j.push_back("ambient");
    }
    if( mapping_attributes & kA3DTextureMappingDiffuse ) {
        j.push_back("diffuse");
    }
    if( mapping_attributes & kA3DTextureMappingBump ) {
        j.push_back("bump");
    }
    if( mapping_attributes & kA3DTextureMappingOpacity ) {
        j.push_back("opacity");
    }
    if( mapping_attributes & kA3DTextureMappingSphericalReflection ) {
        j.push_back("spherical_reflection");
    }
    if( mapping_attributes & kA3DTextureMappingCubicalReflection ) {
        j.push_back("cubical_reflection");
    }
    if( mapping_attributes & kA3DTextureMappingRefraction ) {
        j.push_back("refraction");
    }
    if( mapping_attributes & kA3DTextureMappingSpecular ) {
        j.push_back("specular");
    }
    if( mapping_attributes & kA3DTextureMappingEmission ) {
        j.push_back("emission");
    }
    if( mapping_attributes & kA3DTextureMappingNormal ) {
        j.push_back("normal");
    }
    if( mapping_attributes & kA3DTextureMappingMetallness ) {
        j.push_back("metallness");
    }
    if( mapping_attributes & kA3DTextureMappingRoughness ) {
        j.push_back("roughness");
    }
    if( mapping_attributes & kA3DTextureMappingOcclusion ) {
        j.push_back("occlusion");
    }
    return j;
}

json applyingModeToJson( A3DUns32 const &applying_mode ) {
    json j;
    if( applying_mode & kA3DTextureApplyingModeAlphaTest ) {
        j.push_back("alpha_test");
    }
    if( applying_mode & kA3DTextureApplyingModeLighting ) {
        j.push_back("lighting");
    }
    if( applying_mode & kA3DTextureApplyingModeNone ) {
        j.push_back("none");
    }
    if( applying_mode & kA3DTextureApplyingModeVertexColor ) {
        j.push_back("vertex_color");
    }
    return j;
}
void to_json( json &j, A3DGraphTextureDefinitionData const &texture_def_data ) {
    ts3d::A3DMiscCartesianTransformationWrapper cart_d( texture_def_data.m_pOperatorTransfo );
    ts3d::A3DGraphTextureTransformationWrapper tex_transf_d( texture_def_data.m_pTextureTransfo );
    j = json{
        { "alpha", texture_def_data.m_dAlpha },
        { "alpha_test_reference", texture_def_data.m_dAlphaTestReference },
        { "b", texture_def_data.m_dBlue },
        { "g", texture_def_data.m_dGreen },
        { "r", texture_def_data.m_dRed },
        { "blend_dst_alpha", texture_def_data.m_eBlend_dst_Alpha },
        { "blend_dst_rgb", texture_def_data.m_eBlend_dst_RGB },
        { "blend_src_alpha", texture_def_data.m_eBlend_src_Alpha },
        { "blend_src_rgb", texture_def_data.m_eBlend_src_RGB },
        { "mapping_op", texture_def_data.m_eMappingOperator },
        { "mapping_type", texture_def_data.m_eMappingType },
        { "texture_alpha_test", texture_def_data.m_eTextureAlphaTest },
        { "texture_func", texture_def_data.m_eTextureFunction },
        { "texture_wrapping_s", texture_def_data.m_eTextureWrappingModeS },
        { "texture_wrapping_t", texture_def_data.m_eTextureWrappingModeT },
        { "texture_applying_mode", applyingModeToJson( texture_def_data.m_ucTextureApplyingMode ) },
        { "texture_dimension", texture_def_data.m_ucTextureDimension },
        { "mapping_attributes", mappingAttributesToJson( texture_def_data.m_uiMappingAttributes ) }
    };

    if(texture_def_data.m_uiMappingAttributesIntensitySize) {
        j["mapping_attributes_intensity"] = ts3d::toVector( texture_def_data.m_pdMappingAttributesIntensity, texture_def_data.m_uiMappingAttributesIntensitySize );
    }
    
    if(texture_def_data.m_uiMappingAttributesComponentsSize) {
        j["mapping_attributes_components"] = ts3d::toVector( texture_def_data.m_pucMappingAttributesComponents, texture_def_data.m_uiMappingAttributesComponentsSize );
    }
    
    if(texture_def_data.m_pTextureTransfo) {
        j["texture_transfo"] = *tex_transf_d.operator->();
    }
    
    if(texture_def_data.m_pOperatorTransfo) {
        j["operator_transfo"] = *cart_d.operator->();
    }
    
    A3DGraphPictureData picture_data;
    A3D_INITIALIZE_DATA( A3DGraphPictureData,  picture_data );
    if( ! CheckResult( A3DGlobalGetGraphPictureData( texture_def_data.m_uiPictureIndex, &picture_data ) ) ) {
        return;
    }
    j["picture"] = picture_data;
}

void to_json( json &j, A3DGraphLinePatternData const &line_pattern_data ) {
    j = json{
        { "real_length", static_cast<bool>(line_pattern_data.m_bRealLength) },
        { "phase", line_pattern_data.m_dPhase },
        { "lengths", ts3d::toVector(line_pattern_data.m_pdLengths, line_pattern_data.m_uiNumberOfLengths) }
    };
}

void to_json( json &j, A3DGraphTextureApplicationData const &texture_app_data ) {
    A3DGraphTextureDefinitionData texture_def_data;
    A3D_INITIALIZE_DATA( A3DGraphTextureDefinitionData, texture_def_data );
    if( !CheckResult( A3DGlobalGetGraphTextureDefinitionData( texture_app_data.m_uiTextureDefinitionIndex, &texture_def_data ) ) ) {
        return;
    }
    
    A3DGraphMaterialData material_data;
    A3D_INITIALIZE_DATA( A3DGraphMaterialData, material_data );
    if( !CheckResult( A3DGlobalGetGraphMaterialData( texture_app_data.m_uiMaterialIndex, &material_data ) ) ) {
        return;
    }

    j = json{
        { "texture_def", texture_def_data },
        { "material_def", material_data }
    };
    
    A3DEntity *material = nullptr;
    if( !CheckResult( A3DMiscPointerFromIndexGet( texture_app_data.m_uiMaterialIndex, kA3DTypeGraphMaterial, &material ) ) ) {
        return;
    }
    
    if( !material ) {
        return;
    }
    
    ts3d::A3DRootBaseWrapper root_base_d( material );
    auto attributes = ts3d::toVector( root_base_d->m_ppAttributes, root_base_d->m_uiSize );
    for( auto attribute : attributes ) {
        ts3d::A3DMiscAttributeWrapper attrib_d( attribute );
        auto const title = attrib_d->m_pcTitle ? std::string( attrib_d->m_pcTitle ) : std::string();
        auto const single_attribs = ts3d::toVector(attrib_d->m_asSingleAttributesData, attrib_d->m_uiSize );
        for( auto const &single_attrib : single_attribs ) {
            switch( single_attrib.m_eType ) {
                case kA3DModellerAttributeTypeInt:
                    if( title == "AlphaMode" ) {
                        j["alpha_mode"] = *reinterpret_cast<A3DInt32*>(single_attrib.m_pcData);
                    }
                    break;
                case kA3DModellerAttributeTypeReal:
                    if( title == "AlphaCutOff" ) {
                        j["alpha_cut_off"] = *reinterpret_cast<A3DDouble*>(single_attrib.m_pcData);
                    } else if( title == "MetallicFactor" ) {
                        j["metallic_factor"] = *reinterpret_cast<A3DDouble*>(single_attrib.m_pcData);
                    } else if( title == "NormalTextureFactor" ) {
                        j["normal_texture_factor"] = *reinterpret_cast<A3DDouble*>(single_attrib.m_pcData);
                    } else if( title == "OcclusionTextureFactor" ) {
                        j["occlusion_texture_factor"] = *reinterpret_cast<A3DDouble*>(single_attrib.m_pcData);
                    } else if( title == "RoughnessFactor" ) {
                        j["roughness_factor"] = *reinterpret_cast<A3DDouble*>(single_attrib.m_pcData);
                    }
                    break;
                default:
                    break;
            }
        }
    }
    
    if( A3D_DEFAULT_MATERIAL_INDEX != texture_app_data.m_uiNextTextureApplicationIndex ) {
        A3DGraphTextureApplicationData next_texture_app_data;
        A3D_INITIALIZE_DATA(A3DGraphTextureApplicationData, next_texture_app_data );
        if( CheckResult( A3DGlobalGetGraphTextureApplicationData( texture_app_data.m_uiNextTextureApplicationIndex, &next_texture_app_data ) ) ) {
            j["next_texture_app_data"] = json( next_texture_app_data );
        }
    }
}

void to_json( json &j, A3DERenderingMode const &rendering_mode ) {
    switch( rendering_mode ) {
        case kA3DSolid:
            j = "solid";
            break;
        case kA3DSolidOutline:
            j = "solid_outline";
            break;
        case kA3DWireframe:
            j = "wireframe";
            break;
        case kA3DHLR:
            j = "hlr";
            break;
        case kA3DRMDefault:
            j = "default";
            break;
    }
}

void to_json( json &j, A3DGraphVPicturePatternData const &vpicture_pattern_data ) {
    ts3d::A3DTessMarkupWrapper markup_d( vpicture_pattern_data.m_pMarkupTess );
    if( markup_d->m_pcLabel ) {
        j["label"] = markup_d->m_pcLabel;
    }
    j["behavior"] = markup_d->m_cBehaviour;
    if( markup_d->m_uiCodesSize ) {
        j["codes"] = ts3d::toVector(markup_d->m_puiCodes, markup_d->m_uiCodesSize);
    }
    if( markup_d->m_uiTextsSize) {
        j["texts"] = ts3d::toVector(markup_d->m_ppcTexts, markup_d->m_uiTextsSize );
    }
    
    // to do, next vpicture pattern
    
}

void to_json( json &j, A3DGraphStyleData const &style_data ) {
    A3DBool is_texture = false;
    if( style_data.m_bMaterial ) {
        CheckResult( A3DGlobalIsMaterialTexture( style_data.m_uiRgbColorIndex, &is_texture ) );
    }

    A3DGraphTextureApplicationData texture_app_data;
    A3D_INITIALIZE_DATA( A3DGraphTextureApplicationData, texture_app_data );
    json texture_json;
    
    A3DGraphMaterialData material_data;
    A3D_INITIALIZE_DATA(A3DGraphMaterialData, material_data);
    json material_json;

    A3DGraphRgbColorData color_data;
    A3D_INITIALIZE_DATA(A3DGraphRgbColorData, color_data);
    json rgb_json;
    
    if( style_data.m_bMaterial ) {
        if( is_texture ) {
            if(! CheckResult( A3DGlobalGetGraphTextureApplicationData( style_data.m_uiRgbColorIndex, &texture_app_data ) ) ) {
                return;
            }
            texture_json = json( texture_app_data );
        } else {
            if( !CheckResult( A3DGlobalGetGraphMaterialData( style_data.m_uiRgbColorIndex, &material_data ) ) ) {
                return;
            }
            material_json = json( material_data );
        }
    } else {
        if( !CheckResult( A3DGlobalGetGraphRgbColorData( style_data.m_uiRgbColorIndex, &color_data ) ) ) {
            return;
        }
        rgb_json = json( color_data );
    }
    
    json vpicture_pattern_json;
    json line_pattern_json;
    if( style_data.m_bVPicture ) {
        A3DGraphVPicturePatternData picture_pattern_data;
        A3D_INITIALIZE_DATA(A3DGraphVPicturePatternData, picture_pattern_data);
        if( CheckResult( A3DGlobalGetGraphVPicturePatternData( style_data.m_uiLinePatternIndex, &picture_pattern_data ) ) ) {
            vpicture_pattern_json = json( picture_pattern_data );
        }
    } else {
        A3DGraphLinePatternData line_pattern_data;
        A3D_INITIALIZE_DATA( A3DGraphLinePatternData, line_pattern_data );
        if( CheckResult( A3DGlobalGetGraphLinePatternData( style_data.m_uiLinePatternIndex, &line_pattern_data ) ) ) {
            line_pattern_json = json( line_pattern_data);
        }
    }

    j = json{
        { "is_texture", static_cast<bool>(is_texture) },
        { "back_culling", static_cast<bool>(style_data.m_bBackCulling) },
        { "front_culling", static_cast<bool>(style_data.m_bFrontCulling) },
        { "transparency_defined", static_cast<bool>(style_data.m_bIsTransparencyDefined) },
        { "is_material", static_cast<bool>(style_data.m_bMaterial) },
        { "no_light", static_cast<bool>(style_data.m_bNoLight) },
        { "special_culling", static_cast<bool>(style_data.m_bSpecialCulling) },
        { "v_picture", static_cast<bool>(style_data.m_bVPicture) },
        { "width", style_data.m_dWidth },
        { "render_mode", style_data.m_eRenderingMode },
        { "transparency", style_data.m_ucTransparency } };

    if( !line_pattern_json.is_null() ) {
        j["line_pattern"] = line_pattern_json;
    }
    
    if( !material_json.is_null() ) {
        j["material"] = material_json;
    }
    
    if( !texture_json.is_null() ) {
        j["texture"] = texture_json;
    }
    
    if( !rgb_json.is_null() ) {
        j["rgb"] = rgb_json;
    }
}

void to_json( json &j, A3DMaterialPhysicType const &physic_type ) {
    switch( physic_type ) {
        case A3DPhysicType_None:
            j = "none";
            break;
        case A3DPhysicType_Fiber:
            j = "fiber";
            break;
        case A3DPhysicType_HoneyComb:
            j = "honey_comb";
            break;
        case A3DPhysicType_Isotropic:
            j = "isotropic";
            break;
        case A3DPhysicType_Orthotropic2D:
            j = "orthotropic_2d";
            break;
        case A3DPhysicType_Orthotropic3D:
            j = "orthotropic_3d";
            break;
        case A3DPhysicType_Anisotropic:
            j = "anisotropic";
            break;
    }
}

void to_json( json &j, A3DMiscMaterialPropertiesData const &material_data ) {
    j["density"] = material_data.m_dDensity;
    if( material_data.m_pcMaterialName ) {
        j["name"] = material_data.m_pcMaterialName;
    }
    j["type"] = json( material_data.m_ePhysicType );
}

int main( int argc, char *argv[] ) {
    auto usage = []{
        std::cerr << "Usage: template <input file>" << std::endl;
        std::cerr << "  <input file>  - specifies a the files to be read using HOOPS Exchange" << std::endl;
        std::cerr << std::endl;
        std::cerr << "This application reads the specified file. Use this to test, but don't commit." << std::endl;
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

    A3DImport i( input_file.c_str() );
    i.m_sLoadData.m_sGeneral.m_bReadSolids = true;
    i.m_sLoadData.m_sGeneral.m_bReadWireframes = false;
    i.m_sLoadData.m_sGeneral.m_eReadGeomTessMode = kA3DReadGeomAndTess;

    loader.Import( i );
    
    A3DMiscMaterialPropertiesData empty_prop_data;
    A3D_INITIALIZE_DATA( A3DMiscMaterialPropertiesData, empty_prop_data );

    ts3d::InstancePathMap instance_path_map;
    auto const rep_items = ts3d::getUniqueLeafEntities( loader.m_psModelFile, kA3DTypeRiRepresentationItem, instance_path_map );
    for( auto ri : rep_items ) {
        A3DMiscMaterialPropertiesData material_prop_data;
        A3D_INITIALIZE_DATA( A3DMiscMaterialPropertiesData, material_prop_data );
        CheckResult( A3DMiscGetMaterialProperties( ri, &material_prop_data ) );
        if( material_prop_data != empty_prop_data ) {
            std::cout << json( material_prop_data ).dump( 4 ) << std::endl;
        }
        
        for( auto instance_path : instance_path_map[ri] ) {
            auto part_it = std::find_if( std::begin( instance_path ), std::end( instance_path ), [](A3DEntity *ntt) {
                return ts3d::getEntityType( ntt ) == kA3DTypeAsmPartDefinition;
            });
            CheckResult( A3DMiscGetMaterialProperties( *part_it, &material_prop_data ) );
            if( material_prop_data != empty_prop_data ) {
                std::cout << json( material_prop_data ).dump( 4 ) << std::endl;
            }

            ts3d::RepresentationItemInstance ri_instance( instance_path );
            auto const instance_net_style = ri_instance.Instance::getNetStyle();
            std::cout << json( instance_net_style ).dump( 4 );
            if( auto tess = std::dynamic_pointer_cast<ts3d::Tess3DInstance>( ri_instance.getTessellation() ) ) {
                for( auto f_idx = 0u; f_idx < tess->faceSize(); f_idx++ ) {
                    auto face_net_style = ri_instance.getNetStyle( f_idx );
                    if( face_net_style != instance_net_style ) {
                        std::cout << "Face level override" << std::endl;
                        std::cout << json( face_net_style ).dump( 4 ) << std::endl;
                    }
                }
            }
        }
    }
	return 0;
}



