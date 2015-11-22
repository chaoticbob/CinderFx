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
#include "cinder/gl/Texture.h"
#include "cinder/ip/Resize.h"
#include "cinder/params/Params.h"
#include "cinder/Capture.h"

#include "cinderfx/Fluid2D.h"

#include "CinderOpenCV.h"

const int   kFlowScale = 8;
const float kDrawScale = 4;

class Fluid2DCamAppApp : public ci::app::App {
public:
	static void prepare(ci::app::App::Settings *settings);
	void setup();
	void mouseDown( ci::app::MouseEvent event );	
	void mouseDrag( ci::app::MouseEvent event );
	void update();
	void draw();

private:
	ci::CaptureRef				mCapture;
	//ci::Surface8uRef			mFlipped;
	ci::gl::TextureRef			mTexCam;

	ci::Surface8u				mPrvScaled, mCurScaled;
	cv::Mat						mPrvCvData, mCurCvData;
	cv::Mat						mFlow;
	float						mVelThreshold;
    
	int							mNumActiveFlowVectors;
	std::vector<std::pair<ci::ivec2, ci::ivec2> >	mFlowVectors;

	int							mFluid2DResX;
	int							mFluid2DResY;
	float						mVelScale;
	float						mDenScale;
	ci::vec2					mPrevPos;
	cinderfx::Fluid2D			mFluid2D;

	ci::Surface32fRef			mSurfVel0, mSurfVel1;
	ci::Channel32fRef			mChanDen0, mChanDen1;
	ci::Channel32fRef			mChanDiv;
	ci::Channel32fRef			mChanPrs;
	ci::Channel32fRef			mChanCurl;
	ci::Channel32fRef			mChanCurlLen;
	
	ci::gl::TextureRef			mTexVel0, mTexVel1;
	ci::gl::TextureRef			mTexDen0, mTexDen1;
	ci::gl::TextureRef			mTexDiv;
	ci::gl::TextureRef			mTexPrs;
	ci::gl::TextureRef			mTexCurl;
	ci::gl::TextureRef			mTexCurlLen;
	
	ci::params::InterfaceGl		mParams;
};

using namespace ci;
using namespace ci::app;
using namespace cinderfx;
using namespace std;

void Fluid2DCamAppApp::prepare( Settings *settings )
{
}

void Fluid2DCamAppApp::setup()
{
	mFluid2DResX = 130;
	mFluid2DResY = (int)(mFluid2DResX/1.3333333f + 0.5f);
	setWindowSize( (int)(4*kDrawScale*mFluid2DResX), (int)(3*kDrawScale*mFluid2DResY) );
	setFrameRate( 1000 );
	glEnable( GL_TEXTURE_2D );
	
	mVelThreshold = 0.75f;
	mNumActiveFlowVectors = 0;
#if defined( CINDER_MSW )
	mVelScale = 0.5f;
	mDenScale = 0.0025f;
#elif defined( CINDER_MAC )
	mVelScale = 2.0f;
	mDenScale = 0.007f;
#endif

	mFluid2D.set( mFluid2DResX, mFluid2DResY );
	mFluid2D.enableDensity();
	mFluid2D.enableVorticityConfinement();
	mFluid2D.setNumPressureIters( 24 );
	mFluid2D.initSimData();
	
	// Create these so we can create the textures ahead of time
	mSurfVel0		= Surface32f::create(mFluid2DResX, mFluid2DResY, false, SurfaceChannelOrder::RGB);
	mSurfVel1		= Surface32f::create(mFluid2DResX, mFluid2DResY, false, SurfaceChannelOrder::RGB);
	mChanDen0		= Channel32f::create(mFluid2DResX, mFluid2DResY);
	mChanDen1		= Channel32f::create(mFluid2DResX, mFluid2DResY);
	mChanDiv		= Channel32f::create(mFluid2DResX, mFluid2DResY);
	mChanPrs		= Channel32f::create(mFluid2DResX, mFluid2DResY);
	mChanCurl		= Channel32f::create(mFluid2DResX, mFluid2DResY);
	mChanCurlLen	= Channel32f::create(mFluid2DResX, mFluid2DResY);
	mTexVel0		= gl::Texture::create(*mSurfVel0);
	mTexVel1		= gl::Texture::create(*mSurfVel1);
	mTexDen0		= gl::Texture::create(*mChanDen0);
	mTexDen1		= gl::Texture::create(*mChanDen1);
	mTexDiv			= gl::Texture::create(*mChanDiv);
	mTexPrs			= gl::Texture::create(*mChanPrs);
	mTexCurl		= gl::Texture::create(*mChanCurl);
	mTexCurlLen		= gl::Texture::create(*mChanCurlLen);
	
	mParams = params::InterfaceGl( "Params", ivec2( 300, 400 ) );
	mParams.addParam( "Stam Step", mFluid2D.stamStepAddr() );
	mParams.addSeparator();
	mParams.addParam( "Velocity Threshold", &mVelThreshold, "min=0 max=2 step=0.001" );
	mParams.addSeparator();
	mParams.addParam( "Velocity Input Scale", &mVelScale, "min=0 max=10 step=0.001" );
	mParams.addParam( "Density Input Scale", &mDenScale, "min=0 max=1 step=0.0001" );
	mParams.addSeparator();
	mParams.addParam( "Velocity Dissipation", mFluid2D.velocityDissipationAddr(), "min=0 max=1 step=0.0001" );
	mParams.addParam( "Density Dissipation", mFluid2D.densityDissipationAddr(), "min=0 max=1 step=0.0001" );
	mParams.addSeparator();
	mParams.addParam( "Velocity Viscosity", mFluid2D.velocityViscosityAddr(), "min=0 max=10 step=0.000001" );
	mParams.addParam( "Density Viscosity", mFluid2D.densityViscosityAddr(), "min=0 max=10 step=0.000001" );
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
	
	// Camera
	try {
		mCapture = Capture::create( 640, 480 );
		mCapture->start();
	}
	catch( ... ) {
		console() << "Failed to initialize capture" << std::endl;
	}
}

void Fluid2DCamAppApp::mouseDown( MouseEvent event )
{
	mPrevPos = event.getPos();
}

void Fluid2DCamAppApp::mouseDrag( MouseEvent event )
{
	float x = (event.getX()/(float)getWindowWidth())*mFluid2D.resX();
	float y = (event.getY()/(float)getWindowHeight())*mFluid2D.resY();	
	
	if( event.isLeftDown() ) {
		vec2 dv = vec2(event.getPos().x - mPrevPos.x, event.getPos().y - mPrevPos.y);
		mFluid2D.splatVelocity( x, y, mVelScale*dv );
		mFluid2D.splatDensity( x, y, mDenScale );
	}

	mPrevPos = event.getPos();
}

void Fluid2DCamAppApp::update()
{
	
	if (mCapture && mCapture->checkNewFrame()) {
		if( ! mTexCam ) {
			mTexCam = gl::Texture::create(*mCapture->getSurface(), gl::Texture::Format().loadTopDown());
		}

		// Flip the image
		/*if( ! mFlipped ) {
			Surface8uRef srcImg = mCapture->getSurface();
			mFlipped = Surface8u::create(srcImg->getWidth(), srcImg->getHeight(), srcImg->hasAlpha(), srcImg->getChannelOrder());
		}
		Surface8uRef srcImg = mCapture->getSurface();
		mFlipped = Surface8u::create(srcImg->getWidth(), srcImg->getHeight(), srcImg->hasAlpha(), srcImg->getChannelOrder());
		for (int y = 0; y < mCapture->getHeight(); ++y) {
			const Color8u* src = (const Color8u*)(srcImg->getData() + (y + 1)*srcImg->getRowBytes() - srcImg->getPixelInc());
			Color8u* dst = (Color8u*)(mFlipped->getData() + y*mFlipped->getRowBytes());
			for (int x = 0; x < mCapture->getWidth(); ++x) {
				*dst = *src;
				++dst;
				--src;
			} 
		}*/
		
		// Create scaled image
		if( ! &mCurScaled  ) {
			//mCurScaled = Surface8u(mFlipped->getWidth() / kFlowScale, mFlipped->getHeight() / kFlowScale, mFlipped->hasAlpha(), mFlipped->getChannelOrder());
			mCurScaled = Surface8u(mTexCam->getWidth() / kFlowScale, mTexCam->getHeight() / kFlowScale, mTexCam->hasAlpha(), ImageIo::RGB);
		}
		//ip::resize(*mFlipped, &mCurScaled );
		//ip::resize(mTexCam, &mCurScaled);

		// Optical flow 
		if( &mCurScaled && &mPrvScaled ) {
			mPrvCvData = mCurCvData;
			mCurCvData = cv::Mat( toOcv( Channel( mCurScaled ) ) );

			if( mPrvCvData.data && mCurCvData.data ) {
				int pyrLvels		= 3;
				int winSize			= 3;
				int iters			= 5;
				int poly_n			= 7;
				double poly_sigma	= 1.5;
				cv::calcOpticalFlowFarneback( mPrvCvData, mCurCvData, mFlow, 0.5, pyrLvels, 2*winSize + 1, iters, poly_n, poly_sigma, cv::OPTFLOW_FARNEBACK_GAUSSIAN );

				if( mFlow.data ) {
					if( mFlowVectors.empty() ) {
						mFlowVectors.resize(mCurScaled.getWidth()*mCurScaled.getHeight());
					}
					
					//memset( &mFlowVectors[0], 0, mCurScaled.getWidth()*mCurScaled.getHeight()*sizeof( vec2 ) );
					mNumActiveFlowVectors = 0;
					for (int j = 0; j < mCurScaled.getHeight(); ++j) {
						for (int i = 0; i < mCurScaled.getWidth(); ++i) {
							const float* fptr = reinterpret_cast<float*>(mFlow.data + j*mFlow.step + i*sizeof(float)*2);
							//
							vec2 v = vec2( fptr[0], fptr[1] ); 
							float lengthSquared = v.x*v.x + v.y*v.y;
							if( lengthSquared >= mVelThreshold ) {
								if( mNumActiveFlowVectors >= (int)mFlowVectors.size() ) {
									mFlowVectors.push_back( std::make_pair( ivec2( i, j ), v ) );
								}
								else {
									mFlowVectors[mNumActiveFlowVectors] = std::make_pair( ivec2( i, j ), v );
								}
								++mNumActiveFlowVectors;
							}
						}
					}
				}
			}
		}

		// Update texture
		//mTexCam->update(*mFlipped);
		mTexCam->update(*mCapture->getSurface());

		// Save previous frame
		if( ! &mPrvScaled ) {
			mPrvScaled = Surface8u(mCurScaled.getWidth(), mCurScaled.getHeight(), mCurScaled.hasAlpha(), ImageIo::RGB);// mCurScaled.getChannelOrder());
		}
		memcpy(mPrvScaled.getData(), mCurScaled.getData(), mCurScaled.getHeight()*mCurScaled.getRowBytes());
	}

	// Update fluid
	float dx = (mFluid2DResX - 2)/(float)(640/kFlowScale);
	float dy = (mFluid2DResY - 2)/(float)(480/kFlowScale);
	for( int i = 0; i < mNumActiveFlowVectors; ++i ) {
		vec2 P = mFlowVectors[i].first;
		const vec2& v = mFlowVectors[i].second;
		float lengthSquared = v.x*v.x + v.y*v.y;
		
		mFluid2D.splatDensity( P.x*dx + 1, P.y*dy + 1, mDenScale*lengthSquared );
		mFluid2D.splatVelocity( P.x*dx + 1, P.y*dy + 1, v*mVelScale );
	}
	mFluid2D.step();

	// Update velocity
	const vec2* srcVel0 = mFluid2D.dbgVel0().data();
	const vec2* srcVel1 = mFluid2D.dbgVel1().data();
	Colorf* dstVel0 = (Colorf*)mSurfVel0->getData();
	Colorf* dstVel1 = (Colorf*)mSurfVel1->getData();
	for( int j = 0; j < mFluid2DResY; ++j ) {
		for( int i = 0; i < mFluid2DResX; ++i ) {
			*dstVel0 = Colorf( srcVel0->x, srcVel0->y, 0.0f );
			*dstVel1 = Colorf( srcVel1->x, srcVel1->y, 0.0f );
			++srcVel0;
			++srcVel1;
			++dstVel0;
			++dstVel1;
		}
	}
	
	// Update Density
	mChanDen0 = Channel32f::create( mFluid2DResX, mFluid2DResY, mFluid2DResX*sizeof(float), 1, mFluid2D.dbgDen0().data() );
	mChanDen1 = Channel32f::create( mFluid2DResX, mFluid2DResY, mFluid2DResX*sizeof(float), 1, mFluid2D.dbgDen1().data() );
	
	mTexDen0->update(*mChanDen0);
	mTexDen1->update(*mChanDen1);
	
	// Update velocity textures
	mTexVel0->update(*mSurfVel0);
	mTexVel1->update(*mSurfVel1);
	
	// Update Divergence
	mChanDiv = Channel32f::create( mFluid2DResX, mFluid2DResY, mFluid2DResX*sizeof(float), 1, mFluid2D.dbgDivergence().data() );
	mTexDiv->update(*mChanDiv);

	// Update Divergence
	mChanPrs = Channel32f::create( mFluid2DResX, mFluid2DResY, mFluid2DResX*sizeof(float), 1, mFluid2D.dbgPressure().data() );
	mTexPrs->update(*mChanPrs);

	// Update Curl, Curl Length
	mChanCurl = Channel32f::create( mFluid2DResX, mFluid2DResY, mFluid2DResX*sizeof(float), 1, mFluid2D.dbgCurl().data() );
	mTexCurl->update(*mChanCurl);
	mChanCurlLen = Channel32f::create( mFluid2DResX, mFluid2DResY, mFluid2DResX*sizeof(float), 1, mFluid2D.dbgCurlLength().data() );
	mTexCurlLen->update(*mChanCurlLen);
}

void Fluid2DCamAppApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
	gl::color( Colorf::white() );
	gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
	
	if( mTexCam ) {
		gl::draw( mTexCam, Rectf( 0*kDrawScale*mFluid2DResX, 0*kDrawScale*mFluid2DResY, 1*kDrawScale*mFluid2DResX, 1*kDrawScale*mFluid2DResY ) );
	}

	gl::draw( mTexVel0, Rectf( 0*kDrawScale*mFluid2DResX, 1*kDrawScale*mFluid2DResY, 1*kDrawScale*mFluid2DResX, 2*kDrawScale*mFluid2DResY ) );
	gl::draw( mTexVel1, Rectf( 1*kDrawScale*mFluid2DResX, 1*kDrawScale*mFluid2DResY, 2*kDrawScale*mFluid2DResX, 2*kDrawScale*mFluid2DResY ) );
	gl::draw( mTexDen0, Rectf( 2*kDrawScale*mFluid2DResX, 1*kDrawScale*mFluid2DResY, 3*kDrawScale*mFluid2DResX, 2*kDrawScale*mFluid2DResY ) );
	gl::draw( mTexDen1, Rectf( 3*kDrawScale*mFluid2DResX, 1*kDrawScale*mFluid2DResY, 4*kDrawScale*mFluid2DResX, 2*kDrawScale*mFluid2DResY ) );

	gl::draw( mTexDiv,     Rectf( 0*kDrawScale*mFluid2DResX, 2*kDrawScale*mFluid2DResY, 1*kDrawScale*mFluid2DResX, 4*kDrawScale*mFluid2DResY ) );
	gl::draw( mTexPrs,     Rectf( 1*kDrawScale*mFluid2DResX, 2*kDrawScale*mFluid2DResY, 2*kDrawScale*mFluid2DResX, 4*kDrawScale*mFluid2DResY ) );
	gl::draw( mTexCurl,    Rectf( 2*kDrawScale*mFluid2DResX, 2*kDrawScale*mFluid2DResY, 3*kDrawScale*mFluid2DResX, 4*kDrawScale*mFluid2DResY ) );
	gl::draw( mTexCurlLen, Rectf( 3*kDrawScale*mFluid2DResX, 2*kDrawScale*mFluid2DResY, 4*kDrawScale*mFluid2DResX, 4*kDrawScale*mFluid2DResY ) );

	mTexCurlLen->unbind();

	gl::color( Color( 1, 0, 0 ) );
	glLineWidth( 0.5f );
	/*glBegin( GL_LINES );
		glVertex2f( vec2( 0, 1*kDrawScale*mFluid2DResY ) );
		glVertex2f( vec2( (float)getWindowWidth(), 1*kDrawScale*mFluid2DResY ) );
		glVertex2f( vec2( 0, 2*kDrawScale*mFluid2DResY ) );
		glVertex2f( vec2( (float)getWindowWidth(), 2*kDrawScale*mFluid2DResY ) );

		glVertex2f( vec2( 1*kDrawScale*mFluid2DResX, 0 ) );
		glVertex2f( vec2( 1*kDrawScale*mFluid2DResX, (float)getWindowHeight() ) );
		glVertex2f( vec2( 2*kDrawScale*mFluid2DResX, 0 ) );
		glVertex2f( vec2( 2*kDrawScale*mFluid2DResX, (float)getWindowHeight() ) );
		glVertex2f( vec2( 3*kDrawScale*mFluid2DResX, 0 ) );
		glVertex2f( vec2( 3*kDrawScale*mFluid2DResX, (float)getWindowHeight() ) );
	glEnd();*/
	mParams.draw();
}

CINDER_APP(Fluid2DCamAppApp, RendererGl, &Fluid2DCamAppApp::prepare)
