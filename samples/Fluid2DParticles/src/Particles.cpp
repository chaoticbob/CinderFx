/*

Copyright (c) 2012-2013 Hai Nguyen
All rights reserved.

Distributed under the Boost Software License, Version 1.0.
http://www.boost.org/LICENSE_1_0.txt
http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt

*/

#include "Particles.h"
//
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
using namespace cinder;

const float kSimDt     = 0.1f;
const float kSimDt2    = kSimDt*kSimDt;
const float kPointSize = 2.5f;
const float kBorder    = kPointSize;
#if defined ( CINDER_COCOA_TOUCH )
const float kDampen         = 0.988f;
const int   kMaxParticles   = 5000;
#else
const float kDampen         = 0.93f;
const int   kMaxParticles   = 50000;
#endif

void Particle::update( float dt )
{
	vec2 vel = mPos - mPrevPos;
	mPos += vel*kSimDt;
	mPos += mAccel*kSimDt2;
	mAccel *= kDampen;
	mPrevPos = mPos;
	
	mAge += dt;
}

void ParticleSystem::setup( const Rectf& aBounds, Fluid2D* aFluid )
{
	mBounds = aBounds;
	mFluid = aFluid;
	
	mParticles.resize( kMaxParticles );
}

void ParticleSystem::append( const Particle& aParticle )
{
	if ( mPartPos >= mParticles.size() ) {
		mPartPos = 0;
	}
	std::vector<Particle>::iterator it = mParticles.begin() + mPartPos;
	for( ; it != mParticles.end(); ++it ) {
		mPartPos++;
		if( ! it->alive() ) {
			*it = aParticle;
			return;
		}
	}
}

void ParticleSystem::update()
{
	static float prevTime = (float)ci::app::getElapsedSeconds();
	float curTime = (float)ci::app::getElapsedSeconds();
	float dt = curTime - prevTime;
	prevTime = curTime;

	Rectf bounds = ci::app::getWindowBounds();
	float minX = -kBorder;
	float minY = -kBorder;
	float maxX = bounds.getWidth();
	float maxY = bounds.getHeight();

	// Avoid the borders in the remap because the velocity might be zero.
	// Nothing interesting happens where velocity is zero.
	float dx = (float)(mFluid->resX() - 4)/(float)bounds.getWidth();
	float dy = (float)(mFluid->resY() - 4)/(float)bounds.getHeight();
	for( int i = 0; i < numParticles(); ++i ) {
		Particle& part = mParticles.at( i );
		if( part.pos().x < minX || part.pos().y < minY || part.pos().x >= maxX || part.pos().y >= maxY ) {
			part.kill();
		}
		else {
			float x = part.pos().x*dx + 2.0f;
			float y = part.pos().y*dy + 2.0f;
			vec2 vel = mFluid->velocity().bilinearSampleChecked( x, y, vec2( 0.0f, 0.0f ) );
			part.addForce( vel );
			part.update( dt );
		}
	}
}

void ParticleSystem::draw()
{
	glPointSize( kPointSize );
#if defined ( CINDER_COCOA_TOUCH )
	
	GLushort indices[ numParticles() ];
	GLfloat colors[ numParticles() * 4 ];
	GLfloat vertices[ numParticles() * 2 ];
	
	for( int i = 0; i < numParticles(); ++i ) {
		
		const Particle& part = mParticles.at( i );
		
		indices[ i ] = i;
		
		float alpha = part.age()*part.invLife();
		if ( part.alive() ) {
			alpha = 1.0f - std::min( alpha, 1.0f );
			alpha = std::min( alpha, 0.8f );
		} else {
			alpha = 0.0f;
		}
		
		ColorAf color( part.color(), alpha );
		colors[ i * 4 + 0 ] = color.r;
		colors[ i * 4 + 1 ] = color.g;
		colors[ i * 4 + 2 ] = color.b;
		colors[ i * 4 + 3 ] = color.a;
		
		vec2 pos = part.pos();
		vertices[ i * 2 + 0 ] = pos.x;
		vertices[ i * 2 + 1 ] = pos.y;
		
	}
	
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_COLOR_ARRAY );
	
	glVertexPointer( 2, GL_FLOAT, 0, (const GLvoid *)vertices );
	glColorPointer( 4, GL_FLOAT, 0, ( const GLvoid *)colors );
	
	glDrawElements( GL_POINTS, numParticles(), GL_UNSIGNED_SHORT, (const GLvoid *)indices );
	
	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );

#else
	gl::begin( GL_POINTS );
	for( int i = 0; i < numParticles(); ++i ) {
		const Particle& part = mParticles.at( i );
		if( ! part.alive() )
			continue;
		float alpha = part.age()*part.invLife();
		alpha = 1.0f - std::min( alpha, 1.0f );
		alpha = std::min( alpha, 0.8f );
		gl::color( ColorAf( part.color(), alpha ) );
		gl::vertex( part.pos() );
	}
	gl::end();
#endif
}
