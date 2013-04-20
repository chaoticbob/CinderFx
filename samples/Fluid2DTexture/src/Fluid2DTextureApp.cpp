/*

Copyright (c) 2012-2013 Hai Nguyen
All rights reserved.

Distributed under the Boost Software License, Version 1.0.
http://www.boost.org/LICENSE_1_0.txt
http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt

*/

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "cinder/TriMesh.h"
#include "cinder/params/Params.h"

#include "Resources.h"

#include "cinderfx/Fluid2D.h"

class Fluid2DTextureApp : public ci::app::AppNative {
public:
	void prepareSettings( ci::app::AppNative::Settings *settings );
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
	ci::Vec2f					mPrevPos;
	cinderfx::Fluid2D			mFluid2D;
	ci::gl::Texture				mTex;
	ci::TriMesh2d				mTriMesh;

	ci::params::InterfaceGl		mParams;
	float						mFrameRate;
};

using namespace ci;
using namespace ci::app;
using namespace cinderfx;
using namespace std;

void Fluid2DTextureApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 700, 700 );
   	settings->setResizable( false ); 
	settings->setFrameRate( 1000 );
	settings->enableMultiTouch();
}

void Fluid2DTextureApp::setup()
{
	glEnable( GL_TEXTURE_2D );
	
	mFrameRate = 0.0f;

	mTex = gl::Texture( loadImage( loadResource( RES_IMAGE ) ) );

	mFluid2D.enableTexCoord();
	mFluid2D.setTexCoordViscosity( 1.0f );

	mDenScale = 50;

	mFluid2D.set( 192, 192 );
   	mFluid2D.setDensityDissipation( 0.99f );
	mVelScale = 0.50f*std::max( mFluid2D.resX(), mFluid2D.resY() );
    
	mParams = params::InterfaceGl( "Params", Vec2i( 300, 400 ) );
	mParams.addParam( "Stam Step", mFluid2D.stamStepAddr() );
	mParams.addSeparator();
	mParams.addParam( "Velocity Input Scale", &mVelScale, "min=0 max=10000 step=1" );
	mParams.addParam( "Density Input Scale", &mDenScale, "min=0 max=1000 step=1" );
	mParams.addSeparator();
	mParams.addParam( "Velocity Dissipation", mFluid2D.velocityDissipationAddr(), "min=0.0001 max=1 step=0.0001" );
	mParams.addParam( "Density Dissipation", mFluid2D.densityDissipationAddr(), "min=0.0001 max=1 step=0.0001" );
	mParams.addParam( "TexCoord Dissipation", mFluid2D.texCoordDissipationAddr(), "min=0.0001 max=1 step=0.0001" );
	mParams.addSeparator();
	mParams.addParam( "Velocity Viscosity", mFluid2D.velocityViscosityAddr(), "min=0.000001 max=10 step=0.000001" );
	mParams.addParam( "Density Viscosity", mFluid2D.densityViscosityAddr(), "min=0.000001 max=10 step=0.000001" );
	mParams.addParam( "TexCoord Viscosity", mFluid2D.texCoordViscosityAddr(), "min=0.000001 max=10 step=0.000001" );
	mParams.addSeparator();
	//mParams.addParam( "Vorticity Confinement", mFluid2D.enableVorticityConfinementAddr() );
	//mParams.addSeparator();
	std::vector<std::string> boundaries;
	boundaries.push_back( "None" ); boundaries.push_back( "Wall" ); boundaries.push_back( "Wrap" );
	mParams.addParam( "Boundary Type", boundaries, mFluid2D.boundaryTypeAddr() );
	mParams.addSeparator();
	mParams.addParam( "Enable Buoyancy", mFluid2D.enableBuoyancyAddr() );
	mParams.addParam( "Buoyancy Scale", mFluid2D.buoyancyScaleAddr(), "min=0 max=100 step=0.001" );
	mParams.addParam( "Vorticity Scale", mFluid2D.vorticityScaleAddr(), "min=0 max=1 step=0.001" );

	// Points and texture coordinates
	for( int j = 0; j < mFluid2D.resY(); ++j ) {
		for( int i = 0; i < mFluid2D.resX(); ++i ) {
			mTriMesh.appendVertex( Vec2f( 0.0f, 0.0f ) );
			mTriMesh.appendTexCoord( Vec2f( 0.0f, 0.0f ) );
		}
	}
	// Triangles
	for( int j = 0; j < mFluid2D.resY() - 1; ++j ) {
		for( int i = 0; i < mFluid2D.resX() - 1; ++i ) {
			int idx0 = (j + 0)*mFluid2D.resX() + (i + 0 );
			int idx1 = (j + 1)*mFluid2D.resX() + (i + 0 );
			int idx2 = (j + 1)*mFluid2D.resX() + (i + 1 );
			int idx3 = (j + 0)*mFluid2D.resX() + (i + 1 );
			mTriMesh.appendTriangle( idx0, idx1, idx2 );
			mTriMesh.appendTriangle( idx0, idx2, idx3 );
		}
	}
	
	//console() << mFluid2D << std::endl;
}

void Fluid2DTextureApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
	case KeyEvent::KEY_r:
		mFluid2D.resetTexCoords();
		break;

	case KeyEvent::KEY_c:
		mFluid2D.clearAll();
		break;
	}
}

void Fluid2DTextureApp::mouseDown( MouseEvent event )
{
	mPrevPos = event.getPos();
}

void Fluid2DTextureApp::mouseDrag( MouseEvent event )
{
	float x = (event.getX()/(float)getWindowWidth())*mFluid2D.resX();
	float y = (event.getY()/(float)getWindowHeight())*mFluid2D.resY();	
	
	if( event.isLeftDown() ) {
		Vec2f dv = event.getPos() - mPrevPos;
		mFluid2D.splatVelocity( x, y, mVelScale*dv );
		if( mFluid2D.isBuoyancyEnabled() ) {
			mFluid2D.splatDensity( x, y, mDenScale );
		}
	}
	
	mPrevPos = event.getPos();
}

void Fluid2DTextureApp::touchesBegan( TouchEvent event )
{
}

void Fluid2DTextureApp::touchesMoved( TouchEvent event )
{
	const std::vector<TouchEvent::Touch>& touches = event.getTouches();
	for( std::vector<TouchEvent::Touch>::const_iterator cit = touches.begin(); cit != touches.end(); ++cit ) {
		Vec2f prevPos = cit->getPrevPos();
		Vec2f pos = cit->getPos();
		float x = (pos.x/(float)getWindowWidth())*mFluid2D.resX();
		float y = (pos.y/(float)getWindowHeight())*mFluid2D.resY();	
		Vec2f dv = pos - prevPos;
		mFluid2D.splatVelocity( x, y, mVelScale*dv );
		if( mFluid2D.isBuoyancyEnabled() ) {
			mFluid2D.splatDensity( x, y, mDenScale );
		}
	}
}

void Fluid2DTextureApp::touchesEnded( TouchEvent event )
{
}

void Fluid2DTextureApp::update()
{
	mFluid2D.step();
	mFrameRate = getAverageFps();
}

void Fluid2DTextureApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
	gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );

	// Update the positions and tex coords
	Rectf drawRect = getWindowBounds();
	int limX = mFluid2D.resX() - 1;
	int limY = mFluid2D.resY() - 1;
	float dx = drawRect.getWidth()/(float)limX;
	float dy = drawRect.getHeight()/(float)limY;
	
	for( int j = 0; j < mFluid2D.resY(); ++j ) {
		for( int i = 0; i < mFluid2D.resX(); ++i ) {
			Vec2f P = Vec2f( i*dx, j*dy );
			Vec2f uv = mFluid2D.texCoordAt( i, j );

			int idx = j*mFluid2D.resX() + i;
			mTriMesh.getVertices()[idx] = P;
			mTriMesh.getTexCoords()[idx] = uv;
			
		}
	}

	mTex.bind();
	gl::draw( mTriMesh ); 
	mTex.unbind();
	
	mParams.draw();	
}

CINDER_APP_NATIVE( Fluid2DTextureApp, RendererGl )
