#include <string>
#include <sstream>
#include <vector>
#include "PointsOfInterest.h"

namespace {
    struct PointOfInterest {
        PointOfInterest( A3DVector3dData const &p, std::vector<A3DVector3dData> const &d ) : pt( p ), directions( d ) {
        }
        
        PointOfInterest( A3DVector3dData const &p ) : pt( p ) {
        }

        std::string toJson(void) const {
            std::stringstream ss;
            ss << "{";
            ss <<     "pt : {";
            ss <<         "x : " << pt.m_dX << ", ";
            ss <<         "y : " << pt.m_dY << ", ";
            ss <<         "z : " << pt.m_dZ << ", ";
            ss <<     "}, ";
            ss <<     "directions : [";
            for( auto const d : directions ) {
                ss <<     "{";
                ss <<         "x : " << d.m_dX << ", ";
                ss <<         "y : " << d.m_dY << ", ";
                ss <<         "z : " << d.m_dZ << ", ";
                ss <<     "}, ";
            }
            ss <<     "]";
            ss << "}";
            return ss.str();
        }
        A3DVector3dData pt;
        std::vector<A3DVector3dData> directions;
    };
    
    A3DVector3dData cross( A3DVector3dData const &lhs, A3DVector3dData const &rhs ) {
        A3DVector3dData result;
        A3D_INITIALIZE_DATA( A3DVector3dData, result );
        result.m_dX = lhs.m_dY * rhs.m_dZ - lhs.m_dZ * rhs.m_dY;
        result.m_dY = -(lhs.m_dX * rhs.m_dZ - lhs.m_dZ * rhs.m_dX);
        result.m_dZ = lhs.m_dX * rhs.m_dY - lhs.m_dY * rhs.m_dX;
        return result;
    }
    
    A3DVector3dData scale( A3DVector3dData const &lhs, A3DVector3dData const &rhs ) {
        A3DVector3dData result;
        A3D_INITIALIZE_DATA( A3DVector3dData, result );
        result.m_dX = lhs.m_dX * rhs.m_dX;
        result.m_dY = lhs.m_dY * rhs.m_dY;
        result.m_dZ = lhs.m_dZ * rhs.m_dZ;
        return result;
    }

    A3DVector3dData descale( A3DVector3dData const &lhs, A3DVector3dData const &rhs ) {
        A3DVector3dData result;
        A3D_INITIALIZE_DATA( A3DVector3dData, result );
        result.m_dX = lhs.m_dX / rhs.m_dX;
        result.m_dY = lhs.m_dY / rhs.m_dY;
        result.m_dZ = lhs.m_dZ / rhs.m_dZ;
        return result;
    }

    A3DVector3dData computeCentroidOfFace( A3DTopoFace *face, A3DVector3dData const &scale ) {
        std::vector<A3DEntity*> created_entities;
        
        A3DTopoShellData shell_data;
        A3D_INITIALIZE_DATA( A3DTopoShellData, shell_data );
        shell_data.m_bClosed = false;
        shell_data.m_uiFaceSize = 1;
        shell_data.m_ppFaces = &face;
        A3DUns8 orientation = 1;
        shell_data.m_pucOrientationWithShell = &orientation;
        A3DTopoShell *shell = nullptr;
        A3DTopoShellCreate( &shell_data, &shell );
        created_entities.push_back( shell );
        
        A3DTopoConnexData connex_data;
        A3D_INITIALIZE_DATA( A3DTopoConnexData, connex_data );
        connex_data.m_ppShells = &shell;
        connex_data.m_uiShellSize = 1;
        A3DTopoConnex *connex = nullptr;
        A3DTopoConnexCreate( &connex_data, &connex );
        created_entities.push_back( connex );
        
        A3DTopoBrepDataData brep_data_data;
        A3D_INITIALIZE_DATA( A3DTopoBrepDataData, brep_data_data );
        brep_data_data.m_ppConnexes = &connex;
        brep_data_data.m_uiConnexSize = 1;
        A3DTopoBrepData *brep_data = nullptr;
        A3DTopoBrepDataCreate( &brep_data_data, &brep_data );
        created_entities.push_back( brep_data );
        
        A3DRiBrepModelData brep_model_data;
        A3D_INITIALIZE_DATA( A3DRiBrepModelData, brep_model_data );
        brep_model_data.m_bSolid = false;
        brep_model_data.m_pBrepData = brep_data;
        A3DRiBrepModel *brep_model = nullptr;
        A3DRiBrepModelCreate( &brep_model_data, &brep_model );
        created_entities.push_back( brep_data );
        
        A3DPhysicalPropertiesData physical_data;
        A3D_INITIALIZE_DATA( A3DPhysicalPropertiesData, physical_data );
        A3DComputePhysicalProperties( brep_model, &scale, &physical_data );
        
        auto const cog = physical_data.m_sGravityCenter;
        
        for( auto ntt : created_entities ) {
            A3DEntityDelete( ntt );
        }
        
        return cog;
    }
    
    void attachPointsOfInterestAsAttribute( A3DEntity *ntt, std::vector<PointOfInterest> const &points_of_interest ) {
        std::stringstream ss;
        ss << "[";
        for( auto const &poi : points_of_interest ) {
            ss << poi.toJson() << ", ";
        }
        ss << "]";
        auto const json = ss.str();
        
        A3DRootBaseAttributeAdd( ntt, const_cast<char*>("points_of_interst"), const_cast<char*>(json.c_str()) );
    }
}

bool ts3d::attachEdgeAttributes( A3DTopoEdge *edge, std::set<A3DTopoFace*> const &owning_faces, A3DVector3dData const &scale ) {
    auto entity_type = kA3DTypeUnknown;
    A3DEntityGetType( edge, &entity_type );
    if( kA3DTypeTopoEdge != entity_type ) {
        return false;
    }
    
    A3DTopoEdgeData edge_data;
    A3D_INITIALIZE_DATA( A3DTopoEdgeData, edge_data );
    A3DTopoEdgeGet( edge, &edge_data );
    auto const hasTrimDomain = edge_data.m_bHasTrimDomain;
    auto const curve = edge_data.m_p3dCurve;
    auto const interval = edge_data.m_sInterval;
    A3DTopoEdgeGet( nullptr, &edge_data );
    
    std::vector<PointOfInterest> points_of_interest;
    if(hasTrimDomain) {
        std::vector<A3DVector3dData> pts( 2 );
        A3D_INITIALIZE_ARRAY_DATA(A3DVector3dData, pts.data(), pts.size() );
        
        // evaluation begin position, edge direction
        A3DCrvEvaluate( curve, interval.m_dMin, 1, pts.data() );
        points_of_interest.push_back( PointOfInterest( ::scale( pts[0], scale ), { pts[1] } ) );
        
        // evaluation end position, edge direction
        A3DCrvEvaluate( curve, interval.m_dMax, 1, pts.data() );
        points_of_interest.push_back( PointOfInterest( ::scale( pts[0], scale ), { pts[1] } ) );

        // evaluate middle position, edge direction
        auto const t = (interval.m_dMin + interval.m_dMax) * 0.5;
        A3DCrvEvaluate( curve, t, 1, pts.data() );
        points_of_interest.push_back( PointOfInterest( ::scale( pts[0], scale ), { pts[1] } ) );
    }
    
    // evalute face directions for each position
    for( auto const face : owning_faces ) {
        A3DEntityGetType( face, &entity_type );
        if( kA3DTypeTopoFace != entity_type ) {
            continue;
        }

        // get the face's surface for projection/evaluation
        A3DTopoFaceData face_data;
        A3D_INITIALIZE_DATA( A3DTopoFaceData, face_data );
        A3DTopoFaceGet( face, &face_data );
        auto const surface = face_data.m_pSurface;
        A3DTopoFaceGet( nullptr, &face_data );
        
        // project the point onto the face
        A3DUns32 n_solutions = 0u;
        A3DVector2dData *params = nullptr;
        A3DDouble *distances = nullptr;
        for( auto &eval_pt : points_of_interest ) {
            auto const descaled = ::descale( eval_pt.pt, scale );
            A3DSurfProjectPoint( surface, &descaled, &n_solutions, &params, &distances );
            if( 0 == n_solutions ) {
                continue;
            }
            
            // find the parameter corresponding to the closest projection
            auto idx_of_closest = 0;
            auto closest_distance = distances[0];
            for( auto idx = 1u; idx < n_solutions; ++idx ) {
                if( distances[idx] < closest_distance ) {
                    idx_of_closest = idx;
                    closest_distance = distances[idx];
                }
            }
            auto const &closest_param = params[idx_of_closest];
            
            // evaluation the normal of the face at the closest parameter location
            A3DVector3dData normal;
            A3D_INITIALIZE_DATA( A3DVector3dData, normal );
            A3DSurfEvaluateNormal( surface, &closest_param, &normal );
            eval_pt.directions.push_back( normal ); // remove duplicates here
            A3DSurfProjectPoint( nullptr, &descaled, &n_solutions, &params, &distances );
        }
    }
    
    A3DEntityGetType( curve, &entity_type );
    switch( entity_type ) {
        case kA3DTypeCrvCircle:
        {
            A3DCrvCircleData circle_data;
            A3D_INITIALIZE_DATA( A3DCrvCircleData, circle_data );
            A3DCrvCircleGet( curve, &circle_data );
            auto const n = cross( circle_data.m_sTrsf.m_sXVector, circle_data.m_sTrsf.m_sYVector );
            points_of_interest.push_back( PointOfInterest( ::scale( circle_data.m_sTrsf.m_sOrigin, scale ), { n } ) );
            A3DCrvCircleGet( nullptr, &circle_data );
        }
            break;
        case kA3DTypeCrvEllipse:
        {
            A3DCrvEllipseData ellipse_data;
            A3D_INITIALIZE_DATA( A3DCrvEllipseData, ellipse_data );
            A3DCrvEllipseGet( curve, &ellipse_data );
            auto const n = cross( ellipse_data.m_sTrsf.m_sXVector, ellipse_data.m_sTrsf.m_sYVector );
            points_of_interest.push_back( PointOfInterest( ::scale( ellipse_data.m_sTrsf.m_sOrigin, scale ), { n } ) );
            A3DCrvEllipseGet( nullptr, &ellipse_data );
        }
            break;
        default:
            break;
    }
    
    attachPointsOfInterestAsAttribute( edge, points_of_interest );
    
    return true;
}

bool ts3d::attachFaceAttributes( A3DTopoFace *face, A3DVector3dData const &scale ) {
    auto entity_type = kA3DTypeUnknown;
    A3DEntityGetType( face, &entity_type );
    if( kA3DTypeTopoFace != entity_type ) {
        return false;
    }

    A3DTopoFaceData face_data;
    A3D_INITIALIZE_DATA( A3DTopoFaceData, face_data );
    A3DTopoFaceGet( face, &face_data );
    auto const surface = face_data.m_pSurface;
    A3DTopoFaceGet( nullptr, &face_data );
    
    std::vector<PointOfInterest> points_of_interest;
    A3DEntityGetType( surface, &entity_type );
    switch( entity_type ) {
        case kA3DTypeSurfPlane:
        {
            // compute center of planar face
            auto const cog = computeCentroidOfFace( face, scale );
            A3DSurfPlaneData plane_data;
            A3D_INITIALIZE_DATA( A3DSurfPlaneData, plane_data );
            A3DSurfPlaneGet( surface, &plane_data );
            auto const n = cross( plane_data.m_sTrsf.m_sXVector, plane_data.m_sTrsf.m_sYVector );
            A3DSurfPlaneGet( nullptr, &plane_data );
            points_of_interest.push_back( PointOfInterest( cog, { n } ) );
        }
            break;
        case kA3DTypeSurfSphere:
        {
            A3DSurfSphereData sphere_data;
            A3D_INITIALIZE_DATA( A3DSurfSphereData, sphere_data );
            A3DSurfSphereGet( surface, &sphere_data );
            points_of_interest.push_back( PointOfInterest( ::scale( sphere_data.m_sTrsf.m_sOrigin, scale ) ) );
            A3DSurfSphereGet( nullptr, &sphere_data );
        }
            break;
        case kA3DTypeSurfCylinder:
        {
            auto const cog = computeCentroidOfFace( face, scale );
            A3DSurfCylinderData cylinder_data;
            A3D_INITIALIZE_DATA( A3DSurfCylinderData, cylinder_data );
            A3DSurfCylinderGet( surface, &cylinder_data );
            auto const n = cross( cylinder_data.m_sTrsf.m_sXVector, cylinder_data.m_sTrsf.m_sYVector );
            A3DSurfCylinderGet( nullptr, &cylinder_data );
            points_of_interest.push_back( PointOfInterest( cog, { n } ) );

        }
            break;
        default:
            break;
    }

    attachPointsOfInterestAsAttribute( face, points_of_interest );

    return true;
}

