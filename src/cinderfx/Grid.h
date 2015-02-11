/*

Copyright (c) 2012-2013 Hai Nguyen
All rights reserved.

Distributed under the Boost Software License, Version 1.0.
http://www.boost.org/LICENSE_1_0.txt
http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt

*/

#pragma once

#include "cinder/Vector.h"

#if defined( CINDER_MSW ) || defined( CINDER_MAC )
#  include <xmmintrin.h>
#endif

namespace cinderfx {

using ci::ivec2;
using ci::ivec3;

#if defined( CINDER_MSW ) || defined( CINDER_MAC )

inline int FloatToInt( float x )
{
	return _mm_cvtt_ss2si( _mm_load_ss( &x ) );
}

inline int FloatToInt( double x ) 
{
	return _mm_cvttsd_si32( _mm_load_sd( &x) );
}

#else

inline int FloatToInt( float x )
{
	return (int)x;
}

inline int FloatToInt( double x ) 
{
	return (int)x;
}

#endif

/**
 * \class Grid2D
 *
 */
template <typename DataT>
class Grid2D {
public:

	Grid2D() {}
	Grid2D( int aResX, int aResY ) { setRes( aResX, aResY ); }

	bool empty() const { 
		return mData.empty(); 
	}

	const ivec2& res() const { 
		return mRes; 
	}

	int resX() const { 
		return mRes.x; 
	}	
	
	int	resY() const { 
		return mRes.y; 
	}

	void setRes( int aResX, int aResY ) {
		mRes = ivec2( aResX, aResY );
		int n = mRes.x*mRes.y;
		mData.resize( n );
	}

	int size() const { 
		return (int)mData.size(); 
	}

	int index( int aX, int aY ) const { 
		return aY*resX() + aX; 
	}

	bool contains( int aX, int aY, int aBorder = 0 ) const {
		return ( aX >= aBorder ) && ( aX < ( mRes.x - aBorder ) ) && ( aY >= aBorder ) && ( aY < ( mRes.y - aBorder ) );
	}

	DataT* data() {
		return &mData[0];
	}

	const DataT* data() const {
		return &mData[0];
	}

	DataT* dataAt( int aX, int aY ) {
		size_t n = index( aX, aY );
		return &mData[n];
	}

	const DataT* dataAt( int aX, int aY ) const {
		size_t n = index( aX, aY );
		return &mData[n];
	}

	DataT& at( int aX, int aY ) { 
		size_t n = index( aX, aY );
		return mData[n];
	}

	const DataT& at( int aX, int aY ) const { 
		size_t n = index( aX, aY );
		return mData[n];
	}

	// Does a bilinear write to the grid
	template <typename RealT> 
	void splat( RealT aX, RealT aY, const DataT& aVal, int aBorder = 0 ) {
		int x0 = FloatToInt( aX );
		int y0 = FloatToInt( aY );
		int x1 = x0 + 1;
		int y1 = y0 + 1;
		RealT a1 = aX - (RealT)x0;
		RealT b1 = aY - (RealT)y0;
		RealT a0 = (RealT)1 - a1;
		RealT b0 = (RealT)1 - b1;
		if( contains( x0, y0, aBorder ) ) at( x0, y0 ) = b0*a0*aVal;
		if( contains( x1, y0, aBorder ) ) at( x1, y0 ) = b0*a1*aVal;
		if( contains( x0, y1, aBorder ) ) at( x0, y1 ) = b1*a0*aVal;
		if( contains( x1, y1, aBorder ) ) at( x1, y1 ) = b1*a1*aVal;
	}

	// Does a bilinear add to the grid
	template <typename RealT> 
	void additiveSplat( RealT aX, RealT aY, const DataT& aVal, int aBorder = 0 ) {
		int x0 = FloatToInt( aX );
		int y0 = FloatToInt( aY );
		int x1 = x0 + 1;
		int y1 = y0 + 1;
		RealT a1 = aX - (RealT)x0;
		RealT b1 = aY - (RealT)y0;
		RealT a0 = (RealT)1 - a1;
		RealT b0 = (RealT)1 - b1;
		if( contains( x0, y0, aBorder ) ) at( x0, y0 ) += b0*a0*aVal;
		if( contains( x1, y0, aBorder ) ) at( x1, y0 ) += b0*a1*aVal;
		if( contains( x0, y1, aBorder ) ) at( x0, y1 ) += b1*a0*aVal;
		if( contains( x1, y1, aBorder ) ) at( x1, y1 ) += b1*a1*aVal;
	}

	template <typename RealT>
	DataT bilinearSample( RealT aX, RealT aY ) const {
		int x0 = FloatToInt( aX );
		int y0 = FloatToInt( aY );
		int x1 = x0 + 1;
		int y1 = y0 + 1;
		RealT a1 = aX - (RealT)x0;
		RealT b1 = aY - (RealT)y0;
		RealT a0 = (RealT)1 - a1;
		RealT b0 = (RealT)1 - b1;
		return b0*( a0*at( x0, y0 ) + a1*at( x1, y0 ) ) + 
			   b1*( a0*at( x0, y1 ) + a1*at( x1, y1 ) );
	}

	/**
	 * aX, aY needs to be between 0, and 1
	 * aOobResult - result returned when aX, aY are out of bounds
	 */
	template <typename RealT>
	DataT bilinearSampleChecked( RealT aX, RealT aY, const DataT& aOobResult ) const {
		int x0 = FloatToInt( aX );
		int y0 = FloatToInt( aY );
		DataT result = aOobResult;
		if( x0 >= 0 && y0 >= 0 && x0 < (mRes.x - 1) && y0 < (mRes.y - 1) ) {
			int x1 = x0 + 1;
			int y1 = y0 + 1;
			RealT a1 = aX - (RealT)x0;
			RealT b1 = aY - (RealT)y0;
			RealT a0 = (RealT)1 - a1;
			RealT b0 = (RealT)1 - b1;
			result = b0*( a0*at( x0, y0 ) + a1*at( x1, y0 ) ) + 
                     b1*( a0*at( x0, y1 ) + a1*at( x1, y1 ) );
		}
		else if( x0 == (mRes.x - 1) && y0 == (mRes.y - 1) ) {
			result = at( x0, y0 );
		}
		else if( x0 == (mRes.x - 1) && y0 >= 0 && y0 < (mRes.y - 1 ) ) {
			int y1 = y0 + 1;
			RealT b1 = aY - (RealT)y0;
			RealT b0 = (RealT)1 - b1;
			result = b0*( at( x0, y0 ) ) + 
                     b1*( at( x0, y1 ) );
		}
		else if( x0 >= 0 && x0 < (mRes.x - 1) && y0 == (mRes.y - 1) ) {
			int x1 = x0 + 1;
			RealT a1 = aX - (RealT)x0;
			RealT a0 = (RealT)1 - a1;
			result = a0*at( x0, y0 ) + a1*at( x1, y0 );
		}
		return result;
	}

	void clearToZero() {
		if( ! mData.empty() ) {
			memset( &mData[0], 0, mData.size()*sizeof(DataT) );
		}
	}

protected:
	ivec2				mRes;
	std::vector<DataT>	mData;
};

} /* namespace cinderfx */
