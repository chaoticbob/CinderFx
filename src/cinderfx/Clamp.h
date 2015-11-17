/*

Copyright (c) 2012-2013 Hai Nguyen
All rights reserved.

Distributed under the Boost Software License, Version 1.0.
http://www.boost.org/LICENSE_1_0.txt
http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt

*/

#pragma once

#include "glm/detail/type_vec2.hpp"

namespace cinderfx {

/**
 * \fn Clamp
 *
 */
template <typename T>
T Clamp( const T& x, const T& a = (T)0, const T& b = (T)1 )
{
	return x < a ? a : ( x > b ? b : x );
	//return std::max( a, std::min( x, b ) );
}

/**
 * \fn Clamp:Vec2
 *
 */
template <typename T>
glm::tvec2<T, glm::highp> Clamp(
	const glm::tvec2<T, glm::highp>& v,
	const glm::tvec2<T, glm::highp>& a = glm::tvec2<T, glm::highp>( 0, 0 ),
	const glm::tvec2<T, glm::highp>& b = glm::tvec2<T, glm::highp>( 1, 1 )
) 
{
	return glm::tvec2<T, glm::highp>(
		Clamp( v.x, a.x, b.x ),
		Clamp( v.y, a.y, b.y )
	);
}

/**
 * \fn Clamp:Colorf
 *
 */
inline Colorf Clamp( 
	const Colorf& c, 
	const Colorf& a = Colorf( 0, 0, 0 ), 
	const Colorf& b = Colorf( 1, 1, 1 ) 
) 
{
	return Colorf(
		Clamp( c.r, a.r, b.r ),
		Clamp( c.g, a.g, b.g ),
		Clamp( c.b, a.b, b.b )
	);
}

/**
 * \fn ClampLower
 *
 */
template <typename T>
T ClampLower( const T& x, const T& a = (T)0 )
{
	return x < a ? a : x;
}

/**
 * \fn ClampLower:Colorf
 *
 */
inline Colorf ClampLower( 
	const Colorf& x, 
	const Colorf& a = Colorf( 0, 0, 0 )
)
{
	return Colorf( 
		ClampLower( (float)x.r, (float)a.r ),
		ClampLower( (float)x.g, (float)a.g ),
		ClampLower( (float)x.b, (float)a.b )
	);
}

/**
 * \fn ClampUpper
 *
 */
template <typename T>
T ClampUpper( const T& x, const T& b = (T)1 )
{
	return x > b ? b : x;
}

/**
 * \fn ClampUpper:Colorf
 *
 */
inline Colorf ClampUpper( 
	const Colorf& x,
	const Colorf& b = Colorf( 1, 1, 1 )
)
{
	return Colorf( 
		ClampUpper( (float)x.r, (float)b.r ),
		ClampUpper( (float)x.g, (float)b.g ),
		ClampUpper( (float)x.b, (float)b.b )
	);
}

} /* namespace cinderfx */
