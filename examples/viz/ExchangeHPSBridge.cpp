//
//  ExchangeHPSBridge.cpp
//  ExchangeToolkit
//
//  Created by Brad Flubacher on 4/18/19.
//  Copyright © 2019 Brad Flubacher. All rights reserved.
//

#include "A3DSDKIncludes.h"
#include "ExchangeToolkit.h"
#include "ExchangeHPSBridge.h"

namespace {
    HPS::RGBAColor getColor( A3DUns32 const &color_idx, double const &a ) {
        if( A3D_DEFAULT_COLOR_INDEX == color_idx ) {
            return HPS::RGBAColor( 1., 0., 0., 1. );
        }
        A3DGraphRgbColorData rgb_color_data;
        A3D_INITIALIZE_DATA( A3DGraphRgbColorData, rgb_color_data );
        A3DGlobalGetGraphRgbColorData( color_idx, &rgb_color_data );
        auto const &r = rgb_color_data.m_dRed;
        auto const &g = rgb_color_data.m_dGreen;
        auto const &b = rgb_color_data.m_dBlue;
        return HPS::RGBAColor( r, g, b, a );
    }
}

QHash<A3DEntity*, HPS::SegmentKey> ts3d::createSegmentForPartDefinition( A3DAsmPartDefinition *part ) {
    QHash<A3DEntity*, HPS::SegmentKey> result;
    if( nullptr == part ) {
        return result;
    }
    
    auto root_segment = HPS::Database::CreateRootSegment();
    auto const all_rep_items = getLeafInstances( part, kA3DTypeRiRepresentationItem );
    for( auto ri_path : all_rep_items ) {
        RepresentationItemInstance const ri( ri_path );
        auto tess3d = std::dynamic_pointer_cast<Tess3DInstance>( ri.getTessellation() );
        if( nullptr == tess3d ) {
            continue;
        }
        auto segment_for_this_ri = root_segment.Subsegment();
        result[ri_path.back()] = segment_for_this_ri;
        auto const exchange_coords = tess3d->coords();
        auto const exchange_normals = tess3d->normals();
        for( auto idx = 0u; idx < tess3d->faceSize(); ++idx ) {
            HPS::PointArray vertex_positions;
            HPS::VectorArray vertex_normals;
            auto const face_mesh = tess3d->getIndexMeshForFace( idx );
            QHash<A3DUns32, size_t> m;
            HPS::IntArray face_list;
            for(auto idx = 0u; idx < face_mesh.vertices().size(); ++idx ) {
                if( 0 == idx % 3 ) {
                    face_list.push_back( 3 );
                }
                auto const exchange_pindex = face_mesh.vertices()[idx];
                auto const it = m.find( exchange_pindex );
                if( std::end( m ) == it ) {
                    auto const next_index = vertex_positions.size();
                    vertex_positions.emplace_back( HPS::Point( exchange_coords[exchange_pindex],
                                                               exchange_coords[exchange_pindex+1],
                                                               exchange_coords[exchange_pindex+2]) );
                    m[exchange_pindex] = next_index;
                    face_list.push_back( next_index );
                } else {
                    face_list.push_back( it.value() );
                }
                
                auto const exchange_nindex = face_mesh.normals()[idx];
                auto const vertex_normal = HPS::Vector( exchange_normals[exchange_nindex],
                                                        exchange_normals[exchange_nindex+1],
                                                        exchange_normals[exchange_nindex+2] );
                vertex_normals.emplace_back( vertex_normal );
            }
            HPS::ShellKit shell_kit_for_this_face;
            shell_kit_for_this_face.SetPoints( vertex_positions );
            shell_kit_for_this_face.SetVertexNormalsByRange( 0, vertex_normals );
            shell_kit_for_this_face.SetFacelist( face_list );
            segment_for_this_ri.InsertShell( shell_kit_for_this_face );
            
            for( auto const loop : face_mesh.loops() ) {
                for( auto const edge : loop._edges ) {
                    if( ! edge._visible ) {
                        continue;
                    }
                    HPS::PointArray line_points;
                    for( auto idx = 0u; idx < edge._vertices.size(); ++idx ) {
                        auto const exchange_idx = edge._vertices[idx];
                        line_points.emplace_back( HPS::Point( exchange_coords[exchange_idx],
                                                              exchange_coords[exchange_idx+1],
                                                              exchange_coords[exchange_idx+2]) );
                    }
                    segment_for_this_ri.InsertLine( line_points );
                }
            }
        }
    }
    result[part] = root_segment;
    return result;
}

HPS::MaterialKit ts3d::getMaterialKit( A3DGraphStyleData const &style_data ) {
    HPS::MaterialKit mk;
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
            
            // TODO push the texture data into the portfolio
            // apply style to segment
            
        } else {
            A3DGraphMaterialData material_data;
            A3D_INITIALIZE_DATA( A3DGraphMaterialData, material_data );
            A3DGlobalGetGraphMaterialData( style_data.m_uiRgbColorIndex, &material_data );
            auto const ambient_color = getColor( material_data.m_uiAmbient, material_data.m_dAmbientAlpha );
            auto const diffuse_color = getColor( material_data.m_uiDiffuse, material_data.m_dDiffuseAlpha );
            if( ambient_color.alpha == 1. && diffuse_color.alpha == 0. ) {
                mk.SetDiffuse( ambient_color );
            } else if( ambient_color.alpha == 0. && diffuse_color.alpha == 1. ) {
                mk.SetDiffuse( diffuse_color );
            } else {
                mk.SetDiffuse( HPS::Interpolate( ambient_color, diffuse_color, 0.5 ) );
            }
            mk.SetEmission( getColor( material_data.m_uiEmissive, material_data.m_dEmissiveAlpha ) );
            mk.SetSpecular( getColor( material_data.m_uiSpecular, material_data.m_dSpecularAlpha ) );
        }
    } else {
        auto const a = static_cast<double>(style_data.m_bIsTransparencyDefined ? style_data.m_ucTransparency : 255u) / 255.;
        auto const color = getColor( style_data.m_uiRgbColorIndex, a );
        mk.SetDiffuse( color );
        mk.SetSpecular( HPS::RGBAColor::Nothing() );
        mk.SetEmission( HPS::RGBAColor::Nothing() );
    }
    return mk;
}
