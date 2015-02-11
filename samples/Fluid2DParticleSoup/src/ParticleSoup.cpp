/*

Copyright (c) 2012-2013 Hai Nguyen
All rights reserved.

Distributed under the Boost Software License, Version 1.0.
http://www.boost.org/LICENSE_1_0.txt
http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt

*/

#include "ParticleSoup.h"
//
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
using namespace cinder;

const float kBorder = 1.0f;
#if defined( CINDER_COCOA_TOUCH )
const float kDampen    = 0.98f;
const int kMaxParticles = 15000;
#else
const float kDampen = 0.94f;
const int kMaxParticles = 75000;
#endif
const float kPointSize = 2.25f;

void Particle::reset( const vec2& aPos, float aLife, const Colorf& aColor )
{
	mPos = mPrevPos = aPos;
	mAccel = vec2( 0, 0 );
	mLife = aLife;
	mAge = 0;
	mColor = aColor;
}

void Particle::update( float simDt, float ageDt )
{
	vec2 vel = mPos - mPrevPos;
	mPos += vel*simDt;
	mPos += mAccel*simDt*simDt;
	mAccel *= kDampen;
	mPrevPos = mPos;
	mAge += ageDt;
}

void ParticleSoup::setup( Fluid2D* aFluid )
{
	
	mFluid = aFluid;
	
	Rectf bounds = ci::app::getWindowBounds();
	for( int n = 0; n < kMaxParticles; ++n ) {
		vec2 P;
		P.x = Rand::randFloat( bounds.x1 + 5.0f, bounds.x2 - 5.0f );
		P.y = Rand::randFloat( bounds.y1 + 5.0f, bounds.y2 - 5.0f );
		float life = Rand::randFloat( 2.0f, 3.0f );
		mParticles.push_back( Particle( P, life, mColor ) );
	}
}

void ParticleSoup::update()
{
	static float prevTime = (float)app::getElapsedSeconds();
	float curTime = (float)app::getElapsedSeconds();
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
			vec2 P;
			P.x = Rand::randFloat( bounds.x1 + 5.0f, bounds.x2 - 5.0f );
			P.y = Rand::randFloat( bounds.y1 + 5.0f, bounds.y2 - 5.0f );
			float life = Rand::randFloat( 2.0f, 3.0f );
			part.reset( P, life, mColor );
		}

		float x = part.pos().x*dx + 2.0f;
		float y = part.pos().y*dy + 2.0f;
		vec2 vel = mFluid->velocity().bilinearSampleChecked( x, y, vec2( 0.0f, 0.0f ) );
		part.addForce( vel );
		part.update( mFluid->dt(), dt );
	}
}

void ParticleSoup::draw()
{

	glPointSize( kPointSize );
#if defined ( CINDER_COCOA_TOUCH )
	
	GLushort indices[ numParticles() ];
	GLfloat colors[ numParticles() * 4 ];
	GLfloat vertices[ numParticles() * 2 ];

	for( GLushort i = 0; i < (GLushort)numParticles(); ++i ) {
		
		const Particle& part = mParticles.at( i );
		
		indices[ i ] = i;
		
		float alpha = std::min( part.age()/1.0f, 0.75f );
		ColorAf color( 1.0f, 0.4f, 0.1f, alpha );
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
		float alpha = std::min( part.age()/1.0f, 0.75f );
		gl::color( ColorAf( 1.0f, 0.4f, 0.1f, alpha ) );
		gl::vertex( part.pos() );
	}
	gl::end();
		
#endif
	
}
