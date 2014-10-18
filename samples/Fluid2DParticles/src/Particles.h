/*

Copyright (c) 2012-2013 Hai Nguyen
All rights reserved.

Distributed under the Boost Software License, Version 1.0.
http://www.boost.org/LICENSE_1_0.txt
http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt

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

	Particle() 
		: mAge( 0 ), mLife( 0 ) {}

	Particle( const vec2& aPos, float aLife, const Colorf& aColor ) 
		: mPos( aPos ), mPrevPos( aPos ), mAccel( 0, 0 ), mAge( 0 ), mLife( aLife ), mInvLife( 1.0f/aLife ), mColor( aColor ) {}

	vec2&			pos() { return mPos; }
	const vec2&	pos() const { return mPos; }
	void			setPos( const vec2& aPos ) { mPos = aPos; mPrevPos = aPos; }
	void			addForce( const vec2& aForce ) { mAccel += aForce; }
	void			clearForce() { mAccel = vec2( 0, 0 ); }

	float			age() const { return mAge; }
	float			life() const { return mLife; }
	float			invLife() const { return mInvLife; }
	void			setLife( float aLife ) { mAge = 0; mLife = aLife; }

	bool			alive() const { return mAge < mLife; }
	void			kill() { mAge = mLife; }

	const Colorf&	color() const { return mColor; }
	void			setColor( const Colorf& aColor ) { mColor = aColor; }

	//void set( const vec2& aPos, float aLife, const Colorf& aColor ) {
	//	setPos( aPos );
	//	clearForce();
	//	setLife( aLife );
	//	setColor( aColor );
	//}

	void			update( float dt );

private:
	vec2		mPos;
	vec2		mPrevPos;
	vec2		mAccel;

	float		mAge;
	float		mLife;
	float		mInvLife;

	Colorf		mColor;
	
};

/**
 *
 *
 */
class ParticleSystem {
public:

	ParticleSystem() 
		: mPartPos( 0 ) 
	{}

	int				numParticles() const { return (int)mParticles.size(); }
	Particle&		at( int n ) { return *( mParticles.begin() + (size_t)n ); }
	const Particle&	at( int n ) const { return *( mParticles.begin() + (size_t)n ); }
	void			append( const Particle& aParticle );

	void			setup( const Rectf& aBounds, Fluid2D* aFluid );
	void			update();
	void			draw();

private:
	Rectf					mBounds;
	Fluid2D*				mFluid;
	int						mMaxParticles;

	std::vector<Particle>	mParticles;
	
	size_t					mPartPos;
};
