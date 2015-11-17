/*

Copyright (c) 2012-2013 Hai Nguyen
All rights reserved.

Distributed under the Boost Software License, Version 1.0.
http://www.boost.org/LICENSE_1_0.txt
http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt

*/

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/Rand.h"
#include "cinder/params/Params.h"
#include "cinder/app/RendererGl.h"

#include "cinderfx/Fluid2D.h"

#include "Particles.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Fluid2DParticlesApp : public App {
public:
	void setup();
	void keyDown( ci::app::KeyEvent event );
	void mouseDown( ci::app::MouseEvent event );	
	void mouseDrag( ci::app::MouseEvent event );
	void touchesBegan( ci::app::TouchEvent event );
	void touchesMoved( ci::app::TouchEvent event );
	void touchesEnded( ci::app::TouchEvent event );
	void update();
	void draw();

private:
	float						mVelScale;
	float						mDenScale;
	float						mRgbScale;
	ci::vec2					mPrevPos;
	cinderfx::Fluid2D			mFluid2D;
	ci::gl::Texture2dRef		mTex;
	ci::params::InterfaceGl		mParams;
	//
	ParticleSystem				mParticles;
	ci::Colorf					mColor;
	std::map<int, ci::Colorf>	mTouchColors;	
};

void Fluid2DParticlesApp::setup()
{
	glEnable( GL_TEXTURE_2D );
	gl::enableAlphaBlending();
	gl::enableAdditiveBlending();
	
	mRgbScale = 50;
	mDenScale = 50;
	
	mFluid2D.set( 192, 192 );
   	mFluid2D.setDensityDissipation( 0.99f );
	mFluid2D.setRgbDissipation( 0.99f ); 
	mVelScale = 3.0f*std::max( mFluid2D.resX(), mFluid2D.resY() );
	
	mParams = params::InterfaceGl( "Params", ivec2( 300, 400 ) );
	mParams.addParam( "Stam Step", mFluid2D.stamStepAddr() );
	mParams.addSeparator();
	mParams.addParam( "Velocity Input Scale", &mVelScale, "min=0 max=10000 step=1" );
	mParams.addParam( "Density Input Scale", &mDenScale, "min=0 max=1000 step=1" );
	mParams.addParam( "Rgb Input Scale", &mRgbScale, "min=0 max=1000 step=1" );
	mParams.addSeparator();
	mParams.addParam( "Velocity Dissipation", mFluid2D.velocityDissipationAddr(), "min=0.0001 max=1 step=0.0001" );
	mParams.addParam( "Density Dissipation", mFluid2D.densityDissipationAddr(), "min=0.0001 max=1 step=0.0001" );
	mParams.addParam( "Rgb Dissipation", mFluid2D.rgbDissipationAddr(), "min=0.0001 max=1 step=0.0001" );   
	mParams.addSeparator();
	mParams.addParam( "Velocity Viscosity", mFluid2D.velocityViscosityAddr(), "min=0.000001 max=10 step=0.000001" );
	mParams.addParam( "Density Viscosity", mFluid2D.densityViscosityAddr(), "min=0.000001 max=10 step=0.000001" );
	mParams.addParam( "Rgb Viscosity", mFluid2D.rgbViscosityAddr(), "min=0.000001 max=10 step=0.000001" );
	mParams.addSeparator();
	mParams.addSeparator();
	mParams.addParam( "Vorticity Confinement", mFluid2D.enableVorticityConfinementAddr() );
	mParams.addSeparator();
	std::vector<std::string> boundaries;
	boundaries.push_back( "None" ); boundaries.push_back( "Wall" ); boundaries.push_back( "Wrap" );
	mParams.addParam( "Boundary Type", boundaries, mFluid2D.boundaryTypeAddr() );
	mParams.addSeparator();
	mParams.addParam( "Enable Buoyancy", mFluid2D.enableBuoyancyAddr() );
	mParams.addParam( "Buoyancy Scale", mFluid2D.buoyancyScaleAddr(), "min=0 max=100 step=0.001" );
	mParams.addParam( "Vorticity Scale", mFluid2D.vorticityScaleAddr(), "min=0 max=1 step=0.001" );

	mFluid2D.setDt( 0.1f );
	mFluid2D.enableDensity();
	mFluid2D.enableRgb();
	mFluid2D.enableVorticityConfinement();
       
	mParticles.setup( getWindowBounds(), &mFluid2D );
}

void Fluid2DParticlesApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
	case KeyEvent::KEY_r:
		mFluid2D.initSimData();
		break;
	}
}

void Fluid2DParticlesApp::mouseDown( MouseEvent event )
{
	mPrevPos = event.getPos();
	mColor.r = Rand::randFloat( 0.25f, 1.0f );
	mColor.g = Rand::randFloat( 0.25f, 1.0f );
	mColor.b = Rand::randFloat( 0.25f, 1.0f );
}

void Fluid2DParticlesApp::mouseDrag( MouseEvent event )
{
	float x = (event.getX()/(float)getWindowWidth())*mFluid2D.resX();
	float y = (event.getY()/(float)getWindowHeight())*mFluid2D.resY();	
	float s = 10;
	
	if( event.isLeftDown() ) {
		vec2 dv = vec2( event.getPos() ) - mPrevPos;
		mFluid2D.splatVelocity( x, y, mVelScale*dv );
		mFluid2D.splatRgb( x, y, mRgbScale*mColor );
		if( mFluid2D.isBuoyancyEnabled() ) {
			mFluid2D.splatDensity( x, y, mDenScale );
		}
		//
		for( int i = 0; i < 10; ++i ) {
			vec2 partPos = vec2( event.getPos() ) + vec2( Rand::randFloat( -s, s ), Rand::randFloat( -s, s ) );
			float life = Rand::randFloat( 2.0f, 4.0f );
			mParticles.append( Particle( partPos, life, mColor ) );
		}
	}
	
	mPrevPos = event.getPos();
}

void Fluid2DParticlesApp::touchesBegan( TouchEvent event )
{
	const std::vector<TouchEvent::Touch>& touches = event.getTouches();
	for( std::vector<TouchEvent::Touch>::const_iterator cit = touches.begin(); cit != touches.end(); ++cit ) {
		Colorf color;
		color.r = Rand::randFloat();
		color.g = Rand::randFloat();
		color.b = Rand::randFloat();
		mTouchColors[cit->getId()] = color;
	}
}

void Fluid2DParticlesApp::touchesMoved( TouchEvent event )
{
	float s = 10;

	const std::vector<TouchEvent::Touch>& touches = event.getTouches();
	for( std::vector<TouchEvent::Touch>::const_iterator cit = touches.begin(); cit != touches.end(); ++cit ) {
		if( mTouchColors.find( cit->getId() ) == mTouchColors.end() )
			continue;	
		vec2 prevPos = cit->getPrevPos();
		vec2 pos = cit->getPos();
		float x = (pos.x/(float)getWindowWidth())*mFluid2D.resX();
		float y = (pos.y/(float)getWindowHeight())*mFluid2D.resY();	
		vec2 dv = pos - prevPos;
		mFluid2D.splatVelocity( x, y, mVelScale*dv );
		mFluid2D.splatRgb( x, y, mRgbScale*mTouchColors[cit->getId()] );
		if( mFluid2D.isBuoyancyEnabled() ) {
			mFluid2D.splatDensity( x, y, mDenScale );
		}
		for( int i = 0; i < 5; ++i ) {
			vec2 partPos = pos + vec2( Rand::randFloat( -s, s ), Rand::randFloat( -s, s ) );
			float life = Rand::randFloat( 3.0f, 6.0f );
			mParticles.append( Particle( partPos, life, mTouchColors[cit->getId()] ) );
		}

	}
}

void Fluid2DParticlesApp::touchesEnded( TouchEvent event )
{
	const std::vector<TouchEvent::Touch>& touches = event.getTouches();
	for( std::vector<TouchEvent::Touch>::const_iterator cit = touches.begin(); cit != touches.end(); ++cit ) {
		mTouchColors.erase( cit->getId() );
	}
}

void Fluid2DParticlesApp::update()
{
	mFluid2D.step();
	mParticles.update();
}

void Fluid2DParticlesApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) );

	gl::color( ColorAf( 1.0f, 1.0f, 1.0f, 0.999f ) );
	float* data = const_cast<float*>( (float*)mFluid2D.rgb().data() );
	Surface32f surf( data, mFluid2D.resX(), mFluid2D.resY(), mFluid2D.resX()*sizeof(Colorf), SurfaceChannelOrder::RGB );
	

	if ( ! mTex ) {
		mTex = gl::Texture::create( surf );
	} else {
		mTex->update( surf );
	}
	gl::draw( mTex, getWindowBounds() );
	mTex->unbind();
	mParticles.draw();
	mParams.draw();
}


void prepareSettings( Fluid2DParticlesApp::Settings *settings )
{
	settings->setWindowSize( 700, 700 );
   	settings->setResizable( false ); 
	settings->setResizable( false );     
	settings->setFrameRate( 1000 );
	settings->setMultiTouchEnabled();
}

CINDER_APP( Fluid2DParticlesApp, RendererGl, prepareSettings )
