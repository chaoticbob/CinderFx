#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/Utilities.h"

#include "cinderfx/Fluid2D.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class _TBOX_PREFIX_App : public ci::app::App {
  public:
	static void prepareSettings( Settings *settings );
	
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void update() override;
	void draw() override;

  private:
	float					mVelScale;
	float					mDenScale;
	ci::ivec2				mPrevPos;
	cinderfx::Fluid2D		mFluid2D;
	ci::gl::TextureRef		mTex;
};

void _TBOX_PREFIX_App::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 700, 700 );
    settings->setResizable( false ); 
	settings->setFrameRate( 1000 );
}

void _TBOX_PREFIX_App::setup()
{
	mDenScale = 25;    
	
	mFluid2D.set( 192, 192 );
 	mFluid2D.setDensityDissipation( 0.99f );
	mVelScale = 3 * std::max( mFluid2D.resX(), mFluid2D.resY() );
   	
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
	float x = (event.getX()/(float)getWindowWidth()) * mFluid2D.resX();
	float y = (event.getY()/(float)getWindowHeight()) * mFluid2D.resY();
	
	if( event.isLeftDown() ) {
		vec2 dv = event.getPos() - mPrevPos;
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
	gl::clear(); 

	Channel32f chan( mFluid2D.resX(), mFluid2D.resY(), mFluid2D.resX()*sizeof(float), 1, const_cast<float*>( mFluid2D.density().data() ) );
	if( ! mTex ) {
		mTex = gl::Texture::create( chan );
	}
	else {
		mTex->update( chan );
	}

	gl::draw( mTex, getWindowBounds() );
}

CINDER_APP( _TBOX_PREFIX_App, app::RendererGl, _TBOX_PREFIX_App::prepareSettings )