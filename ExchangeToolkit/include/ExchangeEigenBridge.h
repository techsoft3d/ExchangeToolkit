#pragma once

#include <Eigen/Dense>
#include "ExchangeToolkit.h"

namespace ts3d {
	/*! \brief Alias for a 4x4 matrix type.
	*  \ingroup eigen_bridge
	*/
	using MatrixType = Eigen::Matrix4d;

	/*! \brief Alias for a 4d vector type.
	*  \ingroup eigen_bridge
	*/
	using VectorType = Eigen::Vector4d;

	/*! \brief Alias for a 4d position type.
	*  \ingroup eigen_bridge
	*/
	using PositionType = Eigen::Vector4d;

	/*! \brief Use this function to obtain a vector to be used with the matrix.
	*  \ingroup eigen_bridge
	*/
	static inline VectorType getVector( A3DVector3dData const &vec ) {
		return ts3d::VectorType( vec.m_dX, vec.m_dY, vec.m_dZ, 0. );
	}

	/*! \brief Use this function to obtain a direction.
	*  \ingroup eigen_bridge
	*/
	static inline PositionType getPosition( A3DVector3dData const &vec ) {
		return ts3d::PositionType( vec.m_dX, vec.m_dY, vec.m_dZ, 1. );
	}
}


namespace {
	static inline ts3d::MatrixType getMatrixFromCartesian( A3DMiscCartesianTransformation *xform ) {
		ts3d::A3DMiscCartesianTransformationWrapper d( xform );
		auto const mirror = (d->m_ucBehaviour & kA3DTransformationMirror) ? -1. : 1.;
		auto const s = ts3d::getVector( d->m_sScale );
		auto const o = ts3d::getPosition( d->m_sOrigin );
		auto const x = ts3d::getVector( d->m_sXVector );
		auto const y = ts3d::getVector( d->m_sYVector );
		auto const z = x.cross3( y ) * mirror;

		ts3d::MatrixType result;
		for(auto idx = 0u; idx < 4u; idx++) {
			result( idx, 0 ) = x( idx ) * s( 0 );
			result( idx, 1 ) = y( idx ) * s( 1 );
			result( idx, 2 ) = z( idx ) * s( 2 );
			result( idx, 3 ) = o( idx );
		}

		return result;
	}

	static inline ts3d::MatrixType getMatrixFromGeneralTransformation( A3DMiscGeneralTransformation *xform ) {
		ts3d::A3DMiscGeneralTransformationWrapper d( xform );

		auto const coeff = d->m_adCoeff;
		ts3d::MatrixType result;
		for(auto idx = 0u; idx < 16; ++idx) {
			result( idx ) = coeff[idx];
		}
		return result;
	}

}



namespace ts3d {
	/*! \brief This function returns a matrix corresponding to the A3DMiscTranslformation.
	* Both general and cartesian transformations are handled
	* \ingroup eigen_bridge
	*/
	static inline MatrixType getMatrix( A3DMiscTransformation *xform ) {
		if(nullptr == xform) {
			return ts3d::MatrixType::Identity();
		}

		auto t = kA3DTypeUnknown;
		A3DEntityGetType( xform, &t );

		switch(t) {
		case kA3DTypeMiscCartesianTransformation:
			return getMatrixFromCartesian( xform );
			break;
		case kA3DTypeMiscGeneralTransformation:
			break;
			return getMatrixFromGeneralTransformation( xform );
		default:
			throw std::invalid_argument( "Unexpected argument type provided" );
			break;
		}
		return ts3d::MatrixType::Identity();
	}

	/*! \brief Gets the matrix of the leaf entity.
	*  \ingroup eigen_bridge
	*/
	static inline MatrixType getMatrix( ts3d::Instance const &i ) {
		auto const leaf_type = i.leafType();
        if( kA3DTypeAsmProductOccurrence == leaf_type ) {
            return ts3d::getMatrix( getLocation( i.leaf() ) );
        } else if( isRepresentationItem( leaf_type ) ) {
            A3DRiRepresentationItemWrapper d( i.leaf() );
            A3DRiCoordinateSystemWrapper csw( d->m_pCoordinateSystem );
            return  ts3d::getMatrix( csw->m_pTransformation );
        }
		return MatrixType::Identity();
	}

	/*! \brief Gets the net matrix for a given instance.
	*  \ingroup eigen_bridge
	*
	* The matrix of each entry in the instance path is obtained
	* and accumulated to provide a net resultant transform.
	* \return Net transform matrix from the first to the last
	* entity in the instance path.
	*/
	static inline MatrixType getNetMatrix( ts3d::Instance const &i ) {
		if(i.path().size() > 1) {
			return getNetMatrix( i.owner() ) * getMatrix( i );
		}
		return getMatrix( i );
	}
}
