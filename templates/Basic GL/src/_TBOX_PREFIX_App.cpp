#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#include "cinderfx/Fluid2D.h"
using namespace cinderfx;

class _TBOX_PREFIX_App : public ci::app::AppNative {
  public:
	void prepareSettings( ci::app::AppNative::Settings *settings );
	void setup();
	void mouseDown( ci::app::MouseEvent event );	
	void mouseDrag( ci::app::MouseEvent event );
	void update();
	void draw();

  private:
	float					mVelScale;
	float					mDenScale;
	ci::Vec2f				mPrevPos;
	cinderfx::Fluid2D		mFluid2D;
	ci::gl::Texture			mTex;
};

void _TBOX_PREFIX_App::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 700, 700 );
    settings->setResizable( false ); 
	settings->setFrameRate( 1000 );
}

void _TBOX_PREFIX_App::setup()
{
	glEnable( GL_TEXTURE_2D );
	
	mDenScale = 25;    
	
	mFluid2D.set( 192, 192 );
 	mFluid2D.setDensityDissipation( 0.99f );
	mVelScale = 3.0f*std::max( mFluid2D.resX(), mFluid2D.resY() );
   	
	mFluid2D.enableDensity();
	mFluid2D.enableVorticityConfinement();
	mFluid2D.initSimData();	
}

void _TBOX_PREFIX_App::mouseDown( MouseEvent event )
{
	mPrevPos = event.getPos();
}

void _TBOX_PREFIX_App::mouseDrag( MouseEvent event )
{
	float x = (event.getX()/(float)getWindowWidth())*mFluid2D.resX();
	float y = (event.getY()/(float)getWindowHeight())*mFluid2D.resY();	
	
	if( event.isLeftDown() ) {
		Vec2f dv = event.getPos() - mPrevPos;
		mFluid2D.splatVelocity( x, y, mVelScale*dv );
		mFluid2D.splatDensity( x, y, mDenScale );
	}

	mPrevPos = event.getPos();
}

void _TBOX_PREFIX_App::update()
{
	mFluid2D.step();
}

void _TBOX_PREFIX_App::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 

	Channel32f chan( mFluid2D.resX(), mFluid2D.resY(), mFluid2D.resX()*sizeof(float), 1, const_cast<float*>( mFluid2D.density().data() ) );

	if( ! mTex ) {
		mTex = gl::Texture( chan );
	} else {
		mTex.update( chan );
	}
	gl::color( Color( 1, 1, 1 ) );
	gl::draw( mTex, getWindowBounds() );
}

CINDER_APP_NATIVE( _TBOX_PREFIX_App, RendererGl )
