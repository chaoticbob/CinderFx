/*

Copyright (c) 2012, Hai Nguyen
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#pragma once

#include <vector>
//
#include "cinder/Color.h"
#include "cinder/Rect.h"
#include "cinder/Vector.h"
using ci::Colorf;
using ci::Rectf;
using ci::vec2;
//
#include "cinderfx/Fluid2D.h"
using cinderfx::Fluid2D;

/**
 *
 *
 */
class Particle {
public:

	Particle() : mAccel( 0, 0 ) {}
	Particle( const vec2& aPos, float aLife, const Colorf& aColor ) 
		: mPos( aPos ), mPrevPos( aPos ), mAccel( 0, 0 ), mLife( aLife ), mAge( 0 ), mColor( aColor ) {}

	vec2&			pos() { return mPos; }
	const vec2&	pos() const { return mPos; }
	void			setPos( const vec2& aPos ) { mPos = aPos; mPrevPos = aPos; }
	void			addForce( const vec2& aForce ) { mAccel += aForce; }

	float			life() const { return mLife; }
	float			age() const { return mAge; }

	const Colorf&	color() const { return mColor; }
	void			setColor( const Colorf& aColor ) { mColor = aColor; }

	void			reset( const vec2& aPos, float aLife, const Colorf& aColor );

	void			update( float simDt, float ageDt );

private:
	vec2	mPos;
	vec2	mPrevPos;
	vec2	mAccel;
	float	mLife;
	float	mAge;
	Colorf	mColor;
};

/**
 *
 *
 */
class ParticleSoup {
public:

	ParticleSoup() : mColor( ci::hsvToRGB( ci::vec3( 0.0f, 1.0f, 1.0f ) ) ) {}

	int				numParticles() const { return (int)mParticles.size(); }
	Particle&		at( int n ) { return *( mParticles.begin() + (size_t)n ); }
	const Particle&	at( int n ) const { return *( mParticles.begin() + (size_t)n ); }
	void			append( const Particle& aParticle ) { mParticles.push_back( aParticle ); }

	const Colorf&	color() const { return mColor; }
	void			setColor( const Colorf& aColor ) { mColor = aColor; }

	void			setup( Fluid2D* aFluid );
	void			update();
	void			draw();

private:
	Fluid2D*				mFluid;
	std::vector<Particle>	mParticles;
	Colorf					mColor;
};
