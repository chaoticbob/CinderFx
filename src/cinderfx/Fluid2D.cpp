/*

Copyright (c) 2012-2013 Hai Nguyen
All rights reserved.

Distributed under the Boost Software License, Version 1.0.
http://www.boost.org/LICENSE_1_0.txt
http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt

*/

#if defined( WIN32 )
#pragma warning( disable : 4996 )
#endif 

#include "cinderfx/Fluid2D.h"
#include "cinderfx/Clamp.h"

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Channel.h"
#include "cinder/CinderMath.h"
#include "cinder/Surface.h"
using namespace ci;

#if defined( CINDER_MSW ) || defined( CINDER_MAC )
#  include <xmmintrin.h>
#endif

namespace cinderfx {

/**
 * \fn CheckAndInitGrid2D
 *
 */
template <typename GridT>
void CheckAndInitGrid2D( int x, int y, std::shared_ptr<GridT>& inOut ) 
{
	if( ! inOut ) {
        inOut = std::shared_ptr<GridT>( new GridT( x, y ) );
#if defined( DEBUG )
		ci::app::console() << "Allocated: res=(" << x << ", " << y << ")" << std::endl;
#endif
	}
}

/**
 * \fn Advect2D
 *
 */
template <typename T, typename RealT>
void Advect2D
( 
	RealT						aDissipation, 
	RealT						aDt, 
	const Grid2D<T>&			aSrc, 
	const Grid2D<Vec2<RealT> >&	aVel, 
	Grid2D<T>&					aDst,
	int							aBorder = 1
)
{
	// Range
	int iStart = aBorder;
	int iEnd   = aSrc.resX() - aBorder;
	int jStart = aBorder;
	int jEnd   = aSrc.resY() - aBorder;

	// Boundary
	const RealT xMin = (RealT)0.5;
	const RealT xMax = (RealT)aSrc.resX() - (RealT)1.5;
	const RealT yMin = (RealT)0.5;
	const RealT yMax = (RealT)aSrc.resY() - (RealT)1.5;

	// Process
	for( int j = jStart; j < jEnd; ++j ) {
		for( int i = iStart; i < iEnd; ++i ) {
			// Velocity
			const Vec2<RealT>& vel = aVel.at( i, j );

			// Previous
			RealT dx = aDt*vel.x;
			RealT dy = aDt*vel.y;
			RealT iPrev = i - dx;
			RealT jPrev = j - dy;
			iPrev = Clamp( iPrev, xMin, xMax );
			jPrev = Clamp( jPrev, yMin, yMax );

			// Advected value
			T advected = aSrc.bilinearSample( iPrev, jPrev );

			// Update
			aDst.at( i, j ) = aDissipation*advected;
		}
	}
}

/**
 * \fn AdvectAndDiffuse2D
 *
 * Combines the advection and diffusion process.
 *
 */
template <typename T, typename RealT>
void AdvectAndDiffuse2D
(
	RealT						aDissipation,
	RealT						aCellSizeX, 
	RealT						aCellSizeY, 
	RealT						aVisc,
	RealT						aDt, 
	const Grid2D<T>&			aSrc, 
	const Grid2D<Vec2<RealT> >&	aVel, 
	Grid2D<T>&					aDst,
	int							aBorder = 1
)
{
	// Range
	int iStart = aBorder;
	int iEnd   = aSrc.resX() - aBorder;
	int jStart = aBorder;
	int jEnd   = aSrc.resY() - aBorder;

	// Boundary
	const RealT xMin = (RealT)0.5;
	const RealT xMax = (RealT)aSrc.resX() - (RealT)1.5;
	const RealT yMin = (RealT)0.5;
	const RealT yMax = (RealT)aSrc.resY() - (RealT)1.5;

	// Jacobi vars
	RealT alpha = aCellSizeX*aCellSizeY/(aVisc*aDt);
	RealT beta = (RealT)4.0 + alpha;
	RealT invBeta = (RealT)1/beta;
	
	// Process
	for( int j = jStart; j < jEnd; ++j ) {
		for( int i = iStart; i < iEnd; ++i ) {
			// Velocity
			const Vec2<RealT>& vel = aVel.at( i, j );

			// Previous
			RealT dx = aDt*vel.x;
			RealT dy = aDt*vel.y;
			RealT iPrev = i - dx;
			RealT jPrev = j - dy;
			iPrev = Clamp( iPrev, xMin, xMax );
			jPrev = Clamp( jPrev, yMin, yMax );

			// Advected value
			T advected = aSrc.bilinearSample( iPrev, jPrev );

			// Diffusion - single step Jacobi
			const T& xL = aSrc.at( i - 1, j );	// Left
			const T& xR = aSrc.at( i + 1, j );	// Right
			const T& xB = aSrc.at( i, j - 1 );	// Bottom
			const T& xT = aSrc.at( i, j + 1 );	// Top
			const T& bC = aSrc.at( i, j );		// Center
			T diffused = (xL + xR + xB + xT + alpha*bC)*invBeta;

			// Update
			aDst.at( i, j ) = aDissipation*((RealT)0.75*advected + (RealT)0.25*diffused);
		}
	}
}

/**
 * \fn SetZeroBoundary2D
 * 
 * Sets a Dirichlet boundary - boundary is based on a value, usually zero
 * or a necessary minimum value.
 *
 */
template <typename T>
void SetZeroBoundary2D
(
	Grid2D<T>&	inOut
)
{
	if( inOut.empty() ) {
		return;
	}

	// X Boundaries
	int m = inOut.resX() - 1;
	for( int j = 0; j < inOut.resY(); ++j ) {
		inOut.at( 0, j ) = ZeroSelector<T>::Value();
		inOut.at( m, j ) = ZeroSelector<T>::Value();
	}

	// Y Boundaries
	int n = inOut.resY() - 1;	
	for( int i = 0; i < inOut.resX(); ++i ) {
		inOut.at( i, 0 ) = ZeroSelector<T>::Value();
		inOut.at( i, n ) = ZeroSelector<T>::Value();
	}
}

/**
 * \fn SetCopyBoundary2D
 *
 * Wraps both horizontally and vertically.
 *
 */
template <typename T>
void SetCopyBoundary2D
(
	Grid2D<T>&	inOut
)
{
	if( inOut.empty() ) {
		return;
	}

	// X Boundaries
	int m = inOut.resX() - 1;
	int x0 = 1;
	int x1 = m - 1;
	for( int j = 0; j < inOut.resY(); ++j ) {
		inOut.at( 0, j ) = inOut.at( x0, j );
		inOut.at( m, j ) = inOut.at( x1, j );
	}

	// Y Boundaries
	int n = inOut.resY() - 1;
	int y0 = 1;
	int y1 = n - 1;
	for( int i = 0; i < inOut.resX(); ++i ) {
		inOut.at( i, 0 ) = inOut.at( i, y0 );
		inOut.at( i, n ) = inOut.at( i, y1 );
	}

	// Corners
	inOut.at( 0, 0 ) = inOut.at( x0, y0 );
	inOut.at( m, 0 ) = inOut.at( x1, y0 );
	inOut.at( 0, n ) = inOut.at( x0, y1 );
	inOut.at( m, n ) = inOut.at( x1, y1 );
}

/**
 * \fn SetWrapBoundary2D
 *
 * Wraps both horizontally and vertically.
 *
 */
template <typename T>
void SetWrapBoundary2D
(
	Grid2D<T>&	inOut
)
{
	if( inOut.empty() ) {
		return;
	}

	// X Boundaries
	int m = inOut.resX() - 1;
	int x0 = 1;
	int x1 = m - 1;
	for( int j = 0; j < inOut.resY(); ++j ) {
		inOut.at( 0, j ) = inOut.at( x1, j );
		inOut.at( m, j ) = inOut.at( x0, j );
	}

	// Y Boundaries
	int n = inOut.resY() - 1;
	int y0 = 1;
	int y1 = n - 1;
	for( int i = 0; i < inOut.resX(); ++i ) {
		inOut.at( i, 0 ) = inOut.at( i, y1 );
		inOut.at( i, n ) = inOut.at( i, y0 );
	}

	// Corners
	inOut.at( 0, 0 ) = inOut.at( x1, y1 );
	inOut.at( m, 0 ) = inOut.at( x0, y1 );
	inOut.at( 0, n ) = inOut.at( x1, y0 );
	inOut.at( m, n ) = inOut.at( x0, y0 );
}

/**
 * \fn SetWallBoundary2D
 * 
 */
template <typename T>
void SetBoundary2D
(
	int			aBoundaryType,
	Grid2D<T>&	inOut
)
{
	if( inOut.empty() ) {
		return;
	}

	if( Fluid2D::BOUNDARY_TYPE_WALL == aBoundaryType ) {
		SetCopyBoundary2D( inOut );		
	}
	else if( Fluid2D::BOUNDARY_TYPE_WRAP == aBoundaryType ) {
		SetWrapBoundary2D( inOut );
	}
	else {
		SetZeroBoundary2D( inOut );
	}
}

/**
 * \fn SetVelocityBoundary2D
 * 
 */
template <typename T>
void SetVelocityBoundary2D
(
	int					aBoundaryType,
	Grid2D<Vec2<T> >&	inOutVel
)
{
	typedef typename Vec2<T>::value_type RealT;

	if( inOutVel.empty() ) {
		return;
	}

	if( Fluid2D::BOUNDARY_TYPE_WALL == aBoundaryType ) {
		RealT s = (RealT)-1;

		// X Boundaries
		int m = inOutVel.resX() - 1;
		int x0 = 1;
		int x1 = m - 1;
		for( int j = 0; j < inOutVel.resY(); ++j ) {
			const Vec2<T>& v0 = inOutVel.at( x0, j );
			const Vec2<T>& v1 = inOutVel.at( x1, j );
			inOutVel.at( 0, j ) = Vec2<T>( s*v0.x, v0.y );
			inOutVel.at( m, j ) = Vec2<T>( s*v1.x, v1.y );
		}

		// Y Boundaries
		int n = inOutVel.resY() - 1;	
		int y0 = 1;
		int y1 = n - 1;
		for( int i = 0; i < inOutVel.resX(); ++i ) {
			const Vec2<T>& v0 = inOutVel.at( i, y0 );
			const Vec2<T>& v1 = inOutVel.at( i, y1 );
			inOutVel.at( i, 0 ) = Vec2<T>( v0.x, s*v0.y );
			inOutVel.at( i, n ) = Vec2<T>( v1.x, s*v1.y );
		}

		// Corners
		inOutVel.at( 0, 0 ) = inOutVel.at( x0, y0 );
		inOutVel.at( m, 0 ) = inOutVel.at( x1, y0 );
		inOutVel.at( 0, n ) = inOutVel.at( x0, y1 );
		inOutVel.at( m, n ) = inOutVel.at( x1, y1 );
	}
	else if( Fluid2D::BOUNDARY_TYPE_WRAP == aBoundaryType ) {
		SetWrapBoundary2D( inOutVel );
	}
	else {
		SetZeroBoundary2D( inOutVel );
	}
}

/**
 * \fn Jacobi2D
 *
 * Jacobi is used for both the pressure solve and the diffusion processes. It's
 * a very straight forward way to do a Poisson solve.
 *
 * For the pressure solve:
 *    alpha = -(dx*dy)
 *    beta  = 4
 *    NOTE: dx and dy are the respective dimensions of the cell. It's simply
 *          the boundary of the fluid divided by the grid resolution for each 
 *          respective dimension. 4 is more or less the "averager" for 2D. The 
 *          left, right, bottom, top cells are added and then divided
 *          through by beta, which is 4 in this case.
 *
 * For diffusion:
 *    alpha = (dx*dy)/(v*dt)  
 *    beta  = 4 + alpha
 *    NOTE: dx and dy are the same as described above. v is the kinematic
 *          viscosity, also just called 'viscosity in the literature. dt is 
 *          the time step of the simulation. The "averager" is adjusted by 
 *          alpha for diffusion. 
 *
 */
template <typename T, typename RealT>
void JacobiSingleStep2D
( 
	RealT				alpha, 
	RealT				beta, 
	const Grid2D<T>&	xMat, 
	const Grid2D<T>&	bMat, 
	Grid2D<T>&			outMat
)
{
	// Range
	int border = 1;
	int iStart = border;
	int iEnd   = xMat.resX() - border;
	int jStart = border;
	int jEnd   = xMat.resY() - border;

	// Run the Jacobi!
	RealT invBeta = (RealT)1/beta;
	for( int j = jStart; j < jEnd; ++j ) {
		for( int i = iStart; i < iEnd; ++i ) {
			const T& xL = xMat.at( i - 1, j );	// Left
			const T& xR = xMat.at( i + 1, j );	// Right
			const T& xB = xMat.at( i, j - 1 );	// Bottom
			const T& xT = xMat.at( i, j + 1 );	// Top
			const T& bC = bMat.at( i, j );		// Center
			outMat.at( i, j ) = (xL + xR + xB + xT + alpha*bC)*invBeta;
		}
	}
}

template <typename T, typename RealT>
void Jacobi2D
( 
	RealT				alpha, 
	RealT				beta, 
	const Grid2D<T>&	xMat, 
	const Grid2D<T>&	bMat, 
	Grid2D<T>&			outMat, 
	int					aNumIters 
)
{
	// Range
	int border = 1;
	int iStart = border;
	int iEnd   = xMat.resX() - border;
	int jStart = border;
	int jEnd   = xMat.resY() - border;

	// Run the Jacobi!
	RealT invBeta = (RealT)1/beta;
	for( int solveIter = 0; solveIter < aNumIters; ++solveIter ) {
		for( int j = jStart; j < jEnd; ++j ) {
			for( int i = iStart; i < iEnd; ++i ) {
				const T& xL = xMat.at( i - 1, j );	// Left
				const T& xR = xMat.at( i + 1, j );	// Right
				const T& xB = xMat.at( i, j - 1 );	// Bottom
				const T& xT = xMat.at( i, j + 1 );	// Top
				const T& bC = bMat.at( i, j );		// Center
				outMat.at( i, j ) = (xL + xR + xB + xT + alpha*bC)*invBeta;
			}
		}
	}
}

/**
 * \fn Diffuse2D
 *
 * Makes things easier to digest
 *
 */
template <typename T, typename RealT>
void Diffuse2D
( 
	RealT				mCellSizeX, 
	RealT				mCellSizeY, 
	RealT				aVisc, 
	RealT				aDt, 
	const Grid2D<T>&	aSrc, 
	Grid2D<T>&			aDst, 
	int					aNumIters = 1 
)
{
	RealT alpha = mCellSizeX*mCellSizeY/(aVisc*aDt);
	RealT beta = (RealT)4.0 + alpha;
	Jacobi2D( alpha, beta, aSrc, aSrc, aDst, aNumIters );
}

/**
 * \fn Buoyancy2D
 *
 */
template <typename RealT>
void Buoyancy2D(
	RealT					aAmbTmp,		// Ambient temperature
	RealT					aSigma,			// Buoyancy constant
	RealT					aKappa,			// Density weight
	const Vec2<RealT>&		aGravityDir,	// Gravity direction
	RealT					aDt,			// Time step
	const Grid2D<RealT>&	aTmp,			// Temperature grid
	const Grid2D<RealT>&	aDen,			// Density grid
	Grid2D<Vec2<RealT> >&	outVel			// out: Velocity grid
)
{
	// Range
	int border = 1;
	int iStart = border;
	int iEnd   = aTmp.resX() - border;
	int jStart = border;
	int jEnd   = aTmp.resY() - border;

	// Calculate buoyancy
	Vec2<RealT> forceDir = -aGravityDir;
	for( int j = jStart; j < jEnd; ++j ) {
		for( int i = iStart; i < iEnd; ++i ) {
			RealT curTmp = aTmp.at( i, j );
			if( curTmp > aAmbTmp ) {
				RealT den = aDen.at( i, j );
				RealT buoy = (aDt*(curTmp - aAmbTmp)*aSigma - den*aKappa);
				outVel.at( i, j ) += buoy*forceDir;
			}
		}
	}
}

/**
 * \fn ComputeDivergence2D
 *
 */
template <typename RealT>
void ComputeDivergence2D
( 
	RealT						aHalfDivCellSizeX, 
	RealT						aHalfDivCellSizeY, 
	const Grid2D<Vec2<RealT> >&	aVel, 
	Grid2D<RealT>&				outDiv 
)
{
	// Range
	int border = 1;
	int iStart = border;
	int iEnd   = aVel.resX() - border;
	int jStart = border;
	int jEnd   = aVel.resY() - border;

	// Compute divergence
	outDiv.clearToZero();
	for( int j = jStart; j < jEnd; ++j ) {
		for( int i = iStart; i < iEnd; ++i ) {
			RealT diffX = aVel.at( i + 1, j ).x - aVel.at( i - 1, j ).x;
			RealT diffY = aVel.at( i, j + 1 ).y - aVel.at( i, j - 1 ).y;
			outDiv.at( i, j ) = aHalfDivCellSizeX*diffX + aHalfDivCellSizeY*diffY;
		}
	}
}

/**
 * \fn SolvePressure2D
 *
 */
template <typename RealT>
void SolvePressure2D
(
	RealT					aCellSizeX, 
	RealT					aCellSizeY,
	int						aNumIters,
	int						aBoundaryType,
	const Grid2D<RealT>&	aDiv,
	Grid2D<RealT>&			inOutPressure
)
{
	// alpha - in the case of pressure, this is -(dx^2) or -(dx*dx). This is the cell 
	//         size, it's not always obvious from the literature.
	//
	// beta - for 2D, this value is 4, for 3D this value is 6. It's more or less 
	//        the divisor to "average" out the values. 
	//
	RealT alpha = -aCellSizeX*aCellSizeY;
	RealT beta = (RealT)4.0;

	// Clear out the pressure
	inOutPressure.clearToZero();
	if( Fluid2D::BOUNDARY_TYPE_WALL == aBoundaryType ) {
		for( int i = 0; i < aNumIters; ++i ) {
			JacobiSingleStep2D( alpha, beta, inOutPressure, aDiv, inOutPressure );
			SetZeroBoundary2D( inOutPressure );
		}
	}
	else {
		Jacobi2D( alpha, beta, inOutPressure, aDiv, inOutPressure, aNumIters );
	}
}

/**
 * \fn SubtractGradient2D
 *
 */
template <typename RealT>
void SubtractGradient2D
(
	RealT					aHalfDivCellSizeX, 
	RealT					aHalfDivCellSizeY, 
	const Grid2D<RealT>&	aPressure, 
	Grid2D<Vec2<RealT> >&	outVel 
)
{
	// Range
	int border = 1;
	int iStart = border;
	int iEnd   = aPressure.resX() - border;
	int jStart = border;
	int jEnd   = aPressure.resY() - border;

	// Subtract gradient
	for( int j = jStart; j < jEnd; ++j ) {
		for( int i = iStart; i < iEnd; ++i ) {
			RealT diffX = aPressure.at( i + 1, j ) - aPressure.at( i - 1, j );
			RealT diffY = aPressure.at( i, j + 1 ) - aPressure.at( i, j - 1 );
			outVel.at( i, j ) -= Vec2<RealT>( aHalfDivCellSizeX*diffX, aHalfDivCellSizeY*diffY );
		}
	}
}

/**
 * \fn Curl2D
 *
 */
template <typename RealT>
RealT Curl2D( int i, int j, const Grid2D<Vec2<RealT> >& aVel )
{
	RealT dudy = aVel.at( i, j + 1 ).x - aVel.at( i, j - 1).x;
	RealT dvdx = aVel.at( i + 1, j ).y - aVel.at( i - 1, j).y;
	return (dudy - dvdx)*(RealT)0.5;
}

/**
 * \fn CalculateCurlField2D( 
 *
 */
template <typename RealT>
void CalculateCurlField2D( 
	const Grid2D<Vec2<RealT> >&	inVel, 
	Grid2D<RealT>&				outCurl,
	Grid2D<RealT>&				outCurlLength
)
{
	// Range
	int border = 1;
	int iStart = border;
	int iEnd   = inVel.resX() - border;
	int jStart = border;
	int jEnd   = inVel.resY() - border;

	// Curl
	for( int j = jStart; j < jEnd; ++j ) {
		for( int i = iStart; i < iEnd; ++i ) {
			RealT curlVal = Curl2D( i, j, inVel );
			outCurlLength.at( i, j ) = curlVal;
			outCurl.at( i, j ) = fabs( curlVal );
		}
	}
}

/**
 * \fn VorticityConfinement2D
 *
 */
template <typename RealT>
void VorticityConfinement2D( 
	RealT						aVorticityScale,
	const Grid2D<Vec2<RealT> >&	inVel, 
	const Grid2D<RealT>&		inCurl,
	const Grid2D<RealT>&		inCurlLength,
	Grid2D<Vec2<RealT> >&		outVel 
)
{
	// Range
	int border = 1;
	int iStart = border;
	int iEnd   = inVel.resX() - border;
	int jStart = border;
	int jEnd   = inVel.resY() - border;

	// Vorticity confinement
	for( int j = jStart; j < jEnd; ++j ) {
		for( int i = iStart; i < iEnd; ++i ) {
			// Find derivative of the magnitude (n = del |w|)
			RealT dwdx = ( inCurl.at( i + 1, j ) - inCurl.at( i - 1, j ) )*((RealT)0.5);
			RealT dwdy = ( inCurl.at( i, j + 1 ) - inCurl.at( i, j - 1 ) )*((RealT)0.5);
			// Calculate vector length: (|n|). The add small factor to prevent divide by zeros.
			RealT lengthSq = dwdx*dwdx + dwdy*dwdy;			
			RealT length = ci::math<RealT>::sqrt( lengthSq ) + (RealT)0.000001;
			length = (RealT)1.0/length;
			dwdx *= length;
			dwdy *= length;
			RealT v = inCurlLength.at( i, j );
			outVel.at( i, j ) = inVel.at( i, j ) + aVorticityScale*Vec2<RealT>( dwdy*-v, dwdx*v );
		}
	}
}

/**
 * \fn ClampGrid2D
 *
 */
template <typename T>
void ClampGrid2D( Grid2D<T>& inOut, const T& lower, const T& upper )
{
	// Range
	int border = 1;
	int iStart = border;
	int iEnd   = inOut.resX() - border;
	int jStart = border;
	int jEnd   = inOut.resY() - border;

	// Clamp
	for( int j = jStart; j < jEnd; ++j ) {
		for( int i = iStart; i < iEnd; ++i ) {
			const T& val = inOut.at( i, j );
			T clamped = Clamp( val, lower, upper );
			inOut.at( i, j ) = clamped;
		}
	}
}

/**
 * \fn ClampGrid2DLower
 *
 */
template <typename T>
void ClampGrid2DLower( Grid2D<T>& inOut, const T& lower )
{
	// Range
	int border = 1;
	int iStart = border;
	int iEnd   = inOut.resX() - border;
	int jStart = border;
	int jEnd   = inOut.resY() - border;

	// Clamp lower
	for( int j = jStart; j < jEnd; ++j ) {
		for( int i = iStart; i < iEnd; ++i ) {
			const T& val = inOut.at( i, j );
			T clamped = ClampLower( val, lower );
			inOut.at( i, j ) = clamped;			
		}
	}
}

/**
 * \fn ClampGrid2DUpper
 *
 */
template <typename T>
void ClampGrid2DUpper( Grid2D<T>& inOut, const T& upper )
{
	// Range
	int border = 1;
	int iStart = border;
	int iEnd   = inOut.resX() - border;
	int jStart = border;
	int jEnd   = inOut.resY() - border;

	// Clamp lower
	for( int j = jStart; j < jEnd; ++j ) {
		for( int i = iStart; i < iEnd; ++i ) {
			const T& val = inOut.at( i, j );
			T clamped = ClampUpper( val, upper );
			inOut.at( i, j ) = clamped;			
		}
	}
}

/**
 * \class Fluid2D
 *
 */
Fluid2D::Fluid2D()
{
	initDefaultVars();
}

Fluid2D::Fluid2D( int aResX, int aResY, const Rectf& aBounds )
{
	initDefaultVars();
	set( aResX, aResY, aBounds );
}

void Fluid2D::initDefaultVars()
{
	// Time step and time
	mDt = 0.1f;
	mTime = 0.0f;

	// Default to 10 for pressure solves
	mNumPressureIters = 10;

	// Defaults to none
	mBoundaryType = Fluid2D::BOUNDARY_TYPE_NONE;

	// Dissipation
	mVelDissipation = 0.995000f;
	mDenDissipation = 0.995000f;
	mTexDissipation = 1.000000f;
	mRgbDissipation = 0.995000f;

	// Viscosity
	mVelViscosity	= 0.000001f;
	mDenViscosity	= 0.000001f;
	mTexViscosity	= 0.000001f;
	mRgbViscosity	= 0.000001f;

	mEnableBuoy			= false;
	mAmbTmp				= 0.00001f;
	mMaterialBuoyancy	= 1.00f;
	mMaterialWeight		= 0.05f;
	mMinColor			= Colorf( 0.0f, 0.0f, 0.0f );
	mMaxColor			= Colorf( 1.0f, 1.0f, 1.0f );
	mBuoyancyScale		= 2.00f;
	mVorticityScale		= 0.25f;
	mGravityDir			= Vec2f( 0, 1 );

	mEnableDen	= true;
	mEnableTex	= false;
	mEnableRgb	= false;

	mDiffuseTex = false;
	mStamStep   = false;
	mEnableVc   = false;
}

void Fluid2D::initSimVars()
{
}

void Fluid2D::set( int aResX, int aResY, const Rectf& aBounds )
{
	mRes = Vec2i( aResX, aResY );
	mBounds = aBounds;

	mCellSize.x = mBounds.getWidth()  / (float)mRes.x;
	mCellSize.y = mBounds.getHeight() / (float)mRes.y;
	mHalfDivCellSize.x = 0.5f/mCellSize.x;
	mHalfDivCellSize.y = 0.5f/mCellSize.y;

	CheckAndInitGrid2D( mRes.x, mRes.y, mVel0 );
	CheckAndInitGrid2D( mRes.x, mRes.y, mVel1 );
	CheckAndInitGrid2D( mRes.x, mRes.y, mDen0 );
	CheckAndInitGrid2D( mRes.x, mRes.y, mDen1 );	
	CheckAndInitGrid2D( mRes.x, mRes.y, mTex0 );
	CheckAndInitGrid2D( mRes.x, mRes.y, mTex1 );
	CheckAndInitGrid2D( mRes.x, mRes.y, mRgb0 );
	CheckAndInitGrid2D( mRes.x, mRes.y, mRgb1 );
	CheckAndInitGrid2D( mRes.x, mRes.y, mDivergence );
	CheckAndInitGrid2D( mRes.x, mRes.y, mPressure );
	CheckAndInitGrid2D( mRes.x, mRes.y, mCurl );
	CheckAndInitGrid2D( mRes.x, mRes.y, mCurlLength );

	mVel0->setRes( mRes.x, mRes.y );
	mVel1->setRes( mRes.x, mRes.y );
	mDen0->setRes( mRes.x, mRes.y );
	mDen1->setRes( mRes.x, mRes.y );	
	mTex0->setRes( mRes.x, mRes.y );
	mTex1->setRes( mRes.x, mRes.y );	
	mRgb0->setRes( mRes.x, mRes.y );
	mRgb1->setRes( mRes.x, mRes.y );	
	mDivergence->setRes( mRes.x, mRes.y );	
	mPressure->setRes( mRes.x, mRes.y );	
	mCurl->setRes( mRes.x, mRes.y );
	mCurlLength->setRes( mRes.x, mRes.y );

	mVel0->clearToZero();
	mVel1->clearToZero();
	mDen0->clearToZero();
	mDen1->clearToZero();	
	mTex0->clearToZero();
	mTex1->clearToZero();	
	mRgb0->clearToZero();
	mRgb1->clearToZero();	
	mDivergence->clearToZero();	
	mPressure->clearToZero();	
	mCurl->clearToZero();
	mCurlLength->clearToZero();

	resetTexCoords();

//ci::app::console() << "Fluid2D::set() mRes=" << mRes << ", mBounds=" << mBounds << std::endl;
}

void Fluid2D::setBoundaryType( BoundaryType val )
{
	bool validBound = (val >= Fluid2D::BOUNDARY_TYPE_NONE && val < Fluid2D::TOTAL_BOUNDARY_TYPE ); 
	mBoundaryType = validBound ? val : Fluid2D::BOUNDARY_TYPE_NONE;
}

void Fluid2D::addVelocity( int aX, int aY, const Vec2f& aVal )
{
	const int kBorder = 1;
	if( mVel0 && mVel0->contains( aX, aY, kBorder ) ) {
		mVel0->at( aX, aY ) = aVal;
	}	
}

void Fluid2D::splatVelocity( float aX, float aY, const Vec2f& aVal )
{
	const int kBorder = 1;
	if( mVel0 ) {
		mVel0->additiveSplat( aX, aY, aVal, kBorder );
	}
}

void Fluid2D::clearVelocity()
{
	if( mVel0 ) {
		mVel0->clearToZero();
	}

	if( mVel1 ) {
		mVel1->clearToZero();
	}
}

void Fluid2D::addDensity( int aX, int aY, float aVal )
{
	const int kBorder = 1;
	if( mDen0 && mDen0->contains( aX, aY, kBorder ) ) {
		mDen0->at( aX, aY ) = aVal;
	}
}

void Fluid2D::splatDensity( float aX, float aY, float aVal )
{
	const int kBorder = 1;
	if( mDen0 ) {
		mDen0->additiveSplat( aX, aY, aVal, kBorder );
	}
}

void Fluid2D::clearDensity()
{
	if( mDen0 ) {
		mDen0->clearToZero();
	}

	if( mDen1 ) {
		mDen1->clearToZero();
	}
}

void Fluid2D::addTexCoord( int aX, int aY, const Vec2f& aVal )
{
	if( mTex0 && mTex0->contains( aX, aY ) ) {
		mTex0->at( aX, aY ) = aVal;
	}	
}

void Fluid2D::splatTexCoord( float aX, float aY, const Vec2f& aVal )
{
	if( mTex0 ) {
		mTex0->splat( aX, aY, aVal );
	}
}

void Fluid2D::clearTexCoord()
{
	if( mTex0 ) {
		mTex0->clearToZero();
	}

	if( mTex1 ) {
		mTex1->clearToZero();
	}
}

void Fluid2D::addRgb( int aX, int aY, const Fluid2D::RgbT& aVal )
{
	if( mRgb0 && mRgb0->contains( aX, aY ) ) {
		mRgb0->at( aX, aY ) = aVal;
	}	
}

void Fluid2D::splatRgb( float aX, float aY, const Fluid2D::RgbT& aVal )
{
	if( mRgb0 ) {
		mRgb0->splat( aX, aY, aVal );
	}
}

void Fluid2D::clearRgb()
{
	if( mRgb0 ) {
		mRgb0->clearToZero();
	}

	if( mRgb1 ) {
		mRgb1->clearToZero();
	}
}

void Fluid2D::clearAll()
{
	clearVelocity();
	clearDensity();
	clearTexCoord();
	clearRgb();
	resetTexCoords();
}

void Fluid2D::step()
{  
	bool aFtzOff = false, aDazOff = false;
	beginSimStepParams( aFtzOff, aDazOff );   
	if( mStamStep ) {
		stepStam();
	}
	else {
		stepCombined();
	}
	mTime += mDt;
	endSimStepParams( aFtzOff, aDazOff );
}

void Fluid2D::stepCombined()
{
	// Velocity
	AdvectAndDiffuse2D( mVelDissipation, mCellSize.x, mCellSize.y, mVelViscosity, mDt, *mVel0, *mVel0, *mVel1 );
	SetVelocityBoundary2D( mBoundaryType, *mVel1 ); 

	// Density
	if( mEnableDen ) {
		AdvectAndDiffuse2D( mDenDissipation, mCellSize.x, mCellSize.y, mDenViscosity, mDt, *mDen0, *mVel0, *mDen1 );
		SetBoundary2D( mBoundaryType, *mDen1 );
	}

	// TexCoords
	if( mEnableTex ) {
		Advect2D( mTexDissipation, mDt, *mTex0, *mVel0, *mTex1 );
		ClampGrid2D( *mTex1, Vec2f( 0.0f, 0.0f ), Vec2f( 1.0f, 1.0f ) );
		SetCopyBoundary2D( *mTex1 );
	}

	// Rgb
	if( mEnableRgb ) {
		AdvectAndDiffuse2D( mRgbDissipation, mCellSize.x, mCellSize.y, mRgbViscosity, mDt, *mRgb0, *mVel0, *mRgb1 );
		SetBoundary2D( mBoundaryType, *mRgb1 );
	}

	// Buoyancy
	if( mEnableBuoy ) {
		Buoyancy2D( mAmbTmp, mMaterialBuoyancy, mMaterialWeight, mBuoyancyScale*mGravityDir, mDt, *mDen1, *mDen1, *mVel1 );
		SetVelocityBoundary2D( mBoundaryType, *mVel1 ); 
	}

	// Calculate divergence
	ComputeDivergence2D( mHalfDivCellSize.x, mHalfDivCellSize.y, *mVel1, *mDivergence );
	SetBoundary2D( mBoundaryType, *mDivergence );

	// Solve pressure
	SolvePressure2D( mCellSize.x, mCellSize.y, mNumPressureIters, mBoundaryType, *mDivergence, *mPressure );
	SetBoundary2D( mBoundaryType, *mPressure );

	// Subtract gradient
	SubtractGradient2D( mHalfDivCellSize.x, mHalfDivCellSize.y, *mPressure, *mVel1 );

	// Vorticity confinement
	if( mEnableVc ) {
		// Calculate curl field
		CalculateCurlField2D( *mVel1, *mCurl, *mCurlLength );
		SetBoundary2D( mBoundaryType, *mCurl );
		SetBoundary2D( mBoundaryType, *mCurlLength );
		// Vorticity confinement
		mVel0.swap( mVel1 );
		VorticityConfinement2D( mVorticityScale, *mVel0, *mCurl, *mCurlLength, *mVel1 );
	}

	// Velocity boundary
	SetVelocityBoundary2D( mBoundaryType, *mVel1 ); 

	// Swap
	mVel0.swap( mVel1 );
	mDen0.swap( mDen1 );
	mTex0.swap( mTex1 );
	mRgb0.swap( mRgb1 );
}

void Fluid2D::stepStam()
{
	// Velocity	
	Diffuse2D( mCellSize.x, mCellSize.y, mVelViscosity, mDt, *mVel0, *mVel1 );
	mVel0.swap( mVel1 );
	Advect2D( mVelDissipation, mDt, *mVel0, *mVel0, *mVel1 );
	SetVelocityBoundary2D( mBoundaryType, *mVel1 );

	// Density
	if( mEnableDen ) {
		Diffuse2D( mCellSize.x, mCellSize.y, mDenViscosity, mDt, *mDen0, *mDen1 );
		mDen0.swap( mDen1 );
		Advect2D( mDenDissipation, mDt, *mDen0, *mVel0, *mDen1 );
		SetBoundary2D( mBoundaryType, *mDen1 );

		if( Fluid2D::BOUNDARY_TYPE_WRAP == mBoundaryType ) {
			mDen0.swap( mDen1 );
			Diffuse2D( mCellSize.x, mCellSize.y, mDenViscosity, mDt, *mDen0, *mDen1 );
		}
	}

	// TexCoord
	if( mEnableTex ) {
		Advect2D( mTexDissipation, mDt, *mTex0, *mVel0, *mTex1 );
		ClampGrid2D( *mTex1, Vec2f( 0.0f, 0.0f ), Vec2f( 1.0f, 1.0f ) );
		SetCopyBoundary2D( *mTex1 );
	}

	// Rgb
	if( mEnableRgb ) {
		Diffuse2D( mCellSize.x, mCellSize.y, mRgbViscosity, mDt, *mRgb0, *mRgb1 );
		mRgb0.swap( mRgb1 );
		Advect2D( mRgbDissipation, mDt, *mRgb0, *mVel0, *mRgb1 );
		SetBoundary2D( mBoundaryType, *mRgb1 );

		if( Fluid2D::BOUNDARY_TYPE_WRAP == mBoundaryType ) {
			mRgb0.swap( mRgb1 );
			Diffuse2D( mCellSize.x, mCellSize.y, mRgbViscosity, mDt, *mRgb0, *mRgb1 );
		}		
	}

	// Buoyancy
	if( mEnableBuoy ) {
		Buoyancy2D( mAmbTmp, mMaterialBuoyancy, mMaterialWeight, mBuoyancyScale*mGravityDir, mDt, *mDen1, *mDen1, *mVel1 );
		SetVelocityBoundary2D( mBoundaryType, *mVel1 ); 
	}

	// Calculate divergence
	ComputeDivergence2D( mHalfDivCellSize.x, mHalfDivCellSize.y, *mVel1, *mDivergence );
	SetBoundary2D( mBoundaryType, *mDivergence );

	// Solve pressure
	SolvePressure2D( mCellSize.x, mCellSize.y, mNumPressureIters, mBoundaryType, *mDivergence, *mPressure );
	SetBoundary2D( mBoundaryType, *mPressure );

	// Subtract gradient
	SubtractGradient2D( mHalfDivCellSize.x, mHalfDivCellSize.y, *mPressure, *mVel1 );

	// Vorticity confinement
	if( mEnableVc ) {
		// Calculate curl field
		CalculateCurlField2D( *mVel1, *mCurl, *mCurlLength );
		SetBoundary2D( mBoundaryType, *mCurl );
		SetBoundary2D( mBoundaryType, *mCurlLength );
		// Vorticity confinement
		mVel0.swap( mVel1 );
		VorticityConfinement2D( mVorticityScale, *mVel0, *mCurl, *mCurlLength, *mVel1 );
	}

	// Velocity boundary
	SetVelocityBoundary2D( mBoundaryType, *mVel1 ); 

	// Swap
	mVel0.swap( mVel1 );
	mDen0.swap( mDen1 );
	mTex0.swap( mTex1 );
	mRgb0.swap( mRgb1 );
}

void Fluid2D::initSimData()
{
	// Clear all the fields
	set( resX(), resY() );
	
	float dist = 0.15f*std::min( resX(), resY() );
	float distSq = dist*dist;

	Vec2f center = 0.5f*Vec2f( (float)resX(), (float)resY() );
	
	VecGrid& velGrid = *mVel0;
	RealGrid& denGrid = *mDen0;

	for( int j = 0; j < resY(); ++j ) {
		for( int i = 0; i < resX(); ++i ) {
			Vec2f dv = Vec2f( (float)i, (float)j ) - center;
			if( dv.lengthSquared() > distSq )
				continue;

			velGrid.at( i, j + (int)dist ) = Vec2f( 0.0f, -25.0f );
			denGrid.at( i, j + (int)dist ) = 5.0f;
		}
	}
}

void Fluid2D::resetTexCoords()
{
	float dx = 1.0f/(float)(mRes.x - 1);
	float dy = 1.0f/(float)(mRes.y - 1);
	for( int j = 0; j < mRes.y; ++j ) {
		for( int i = 0; i < mRes.x; ++i ) {
			mTex0->at( i, j ) = Vec2f( dx*i, dy*j );
			mTex1->at( i, j ) = Vec2f( dx*i, dy*j );
		}
	}
}

void Fluid2D::beginSimStepParams( bool& aFtzOff, bool& aDazOff )
{
#if defined( CINDER_MSW )
  #if _MSC_VER < 1700
	aFtzOff = ( _MM_FLUSH_ZERO_OFF == _MM_GET_FLUSH_ZERO_MODE( _MM_FLUSH_ZERO_MASK ) );
  #else
	aFtzOff = ( _MM_FLUSH_ZERO_OFF == _MM_GET_FLUSH_ZERO_MODE() );
  #endif
	aDazOff = ( _MM_DENORMALS_ZERO_OFF == _MM_GET_DENORMALS_ZERO_MODE() );
	_MM_SET_FLUSH_ZERO_MODE( _MM_FLUSH_ZERO_ON );
	_MM_SET_DENORMALS_ZERO_MODE( _MM_DENORMALS_ZERO_ON );
#elif defined( CINDER_MAC )
	aFtzOff = ( _MM_FLUSH_ZERO_OFF == _MM_GET_FLUSH_ZERO_MODE() );
	_MM_SET_FLUSH_ZERO_MODE( _MM_FLUSH_ZERO_ON );
#endif
}

void Fluid2D::endSimStepParams( bool aFtzOff, bool aDazOff )
{
#if defined( CINDER_MSW ) 
	_MM_SET_FLUSH_ZERO_MODE( aFtzOff ? _MM_FLUSH_ZERO_OFF : _MM_FLUSH_ZERO_ON );
	_MM_SET_DENORMALS_ZERO_MODE( aDazOff ? _MM_DENORMALS_ZERO_OFF : _MM_DENORMALS_ZERO_ON );
#elif defined( CINDER_MAC )
	_MM_SET_FLUSH_ZERO_MODE( aFtzOff ? _MM_FLUSH_ZERO_OFF : _MM_FLUSH_ZERO_ON );
#endif
}

} /* namespace cinderfx */
