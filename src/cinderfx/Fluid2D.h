/*

Copyright (c) 2012-2013 Hai Nguyen
All rights reserved.

Distributed under the Boost Software License, Version 1.0.
http://www.boost.org/LICENSE_1_0.txt
http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt

*/

#pragma once

#include "cinder/Color.h"
#include "cinder/Rect.h"
#include "cinderfx/Grid.h"

namespace cinderfx {

using ci::Colorf;
using ci::Rectf;
using ci::Vec2;
using ci::Vec2i;
using ci::Vec2f;

template <typename T> struct ZeroSelector {
	static T Value() { return (T)0; }
};

template <> struct ZeroSelector<Colorf> {
	static Colorf Value() { return Colorf( 0.0f, 0.0f, 0.0f ); }
};

template <> struct ZeroSelector<Vec2f> {
	static Vec2f Value() { return Vec2f( 0.0f, 0.0f ); }
};


/**
 * \class Fluid2D
 *
 */
class Fluid2D {
public:
	typedef float						RealT;
	typedef Vec2f						VecT;
	typedef Colorf						RgbT;
	typedef Grid2D<RealT>				RealGrid;
	typedef Grid2D<VecT>				VecGrid;
	typedef Grid2D<RgbT>				RgbGrid;
	typedef std::shared_ptr<RealGrid>	RealGridPtr;
	typedef std::shared_ptr<VecGrid>	VecGridPtr;
	typedef std::shared_ptr<RgbGrid>	RgbGridPtr;

	enum BoundaryType {
		BOUNDARY_TYPE_NONE = 0,		// Dirichlet boundary
		BOUNDARY_TYPE_WALL,
		BOUNDARY_TYPE_WRAP,
		TOTAL_BOUNDARY_TYPE
	};

	Fluid2D();
	Fluid2D( int aResX, int aResY, const Rectf& aBounds = Rectf( 0, 0, 1, 1 ) );
	virtual ~Fluid2D() {}

	// Don't call this in constructors - it's virtual.
	virtual void		initSimVars();

	int					resX() const { return mRes.x; }
	int					resY() const { return mRes.y; }
	void				set( int aResX, int aResY, const Rectf& aBounds = Rectf( 0, 0, 1, 1 ) );

	float				dt() const { return mDt; }
	void				setDt( float aDt ) { mDt = aDt; }

	int					numPressureIters() const { return mNumPressureIters; }
	void				setNumPressureIters( int val ) { mNumPressureIters = std::max( 0, val ); }

	int					boundaryType() const { return mBoundaryType; }
	int*				boundaryTypeAddr() { return &mBoundaryType; }
	void				setBoundaryType( BoundaryType val );

	// Enable/disable buoyancy
	bool				isBuoyancyEnabled() const { return mEnableBuoy; }
	bool*				enableBuoyancyAddr() { return &mEnableBuoy; }
	void				enableBuoyancy( bool val = true ) { mEnableBuoy = val; if( mEnableBuoy ) { enableDensity( true ); } }
	// Ambient temperature
	float				ambientTemperature() const { return mAmbTmp; }
	float*				ambientTemperatureAddr() { return &mAmbTmp; }
	void				setAmbientTemperature( float val ) { mAmbTmp = val; }
	// Material buoyancy
	float				materialBuoyancy() const { return mMaterialBuoyancy; }
	float*				materialBuoyancyAddr() { return &mMaterialBuoyancy; }
	void				setMaterialBuoyancy( float val ) { mMaterialBuoyancy = val; }
	// Material weight
	float				materialWeight() const { return mMaterialWeight; }
	float*				materialWeightAddr() { return &mMaterialWeight; }
	void				setMaterialWeight( float val ) { mMaterialWeight = val; }
	// Minimum color value
	const Colorf&		minimumColor() const { return mMinColor; }
	Colorf*				minimumColorAddr() { return &mMinColor; }
	void				setMinimumColor( const Colorf& val ) { mMinColor = val; }
	// Maximum color value
	const Colorf&		maximumColor() const { return mMaxColor; }
	Colorf*				maximumColorAddr() { return &mMaxColor; }
	void				setMaximumColor( const Colorf& val ) { mMaxColor = val; }
	// Buoyancy scale
	float				buoyancyScale() const { return mBuoyancyScale; }
	float*				buoyancyScaleAddr() { return &mBuoyancyScale; }
	void				setBuoyancyScale( float val ) { mBuoyancyScale = val; }
	// Vorticity scale
	float				vorticityScale() const { return mVorticityScale; }
	float*				vorticityScaleAddr() { return &mVorticityScale; }
	void				setVorticityScale( float val ) { mVorticityScale = val; }
	// Gravity direction
	const Vec2f&		gravityDir() const { return mGravityDir; }
	Vec2f*				gravityDirAddr() { return &mGravityDir; }
	void				setGravityDir( const Vec2f& val ) { mGravityDir = val; }

	// Density enable/disable
	bool				isDensityEnabled() const { return mEnableDen; }
	bool*				enableDensityAddr() { return &mEnableDen; }
	void				enableDensity( bool val = true ) { mEnableDen = val; }
	// TexCoord enable/disable
	bool				isTexCoordEnabled() const { return mEnableTex; }
	bool*				enableTexCoordAddr() { return &mEnableTex; }
	void				enableTexCoord( bool val = true ) { mEnableTex = val; }
	// Rgb enable/disable
	bool				isRgbEnabled() const { return mEnableRgb; }
	bool*				enableRgbAddr() { return &mEnableRgb; }
	void				enableRgb( bool val = true ) { mEnableRgb = val; }
	// Stam step enable/disable
	bool				isStamStep() const { return mStamStep; }
	bool*				stamStepAddr() { return &mStamStep; }
	void				setStamStep( bool val = true ) { mStamStep = val; }
	// Vorticity confinement enable/disable
	bool				isVcEnabled() const { return mEnableVc; }
	bool*				enableVorticityConfinementAddr() { return &mEnableVc; }
	void				enableVorticityConfinement( bool val = true ) { mEnableVc = val; }

	// Velocity dissipation
	float				velocityDissipation() const { return mVelDissipation; }
	float*				velocityDissipationAddr() { return &mVelDissipation; }
	void				setVelocityDissipation( float val ) { mVelDissipation = val; }
	// Density dissipation
	float				densityDissipation() const { return mDenDissipation; }
	float*				densityDissipationAddr() { return &mDenDissipation; }
	void				setDensityDissipation( float val ) { mDenDissipation = val; }
	// TexCoords dissipation
	float				texCoordDissipation() const { return mTexDissipation; }
	float*				texCoordDissipationAddr() { return &mTexDissipation; }
	void				setTexCoordDissipation( float val ) { mTexDissipation = val; }
	// Rgb dissipation
	float				rgbDissipation() const { return mRgbDissipation; }
	float*				rgbDissipationAddr() { return &mRgbDissipation; }
	void				setRgbDissipation( float val ) { mRgbDissipation = val; }

	// Velocity viscosity
	float				velocityViscosity() const { return mVelViscosity; }
	float*				velocityViscosityAddr() { return &mVelViscosity; }
	void				setVelocityViscosity( float val ) { mVelViscosity = val; }
	// Density viscosity
	float				densityViscosity() const { return mDenViscosity; }
	float*				densityViscosityAddr() { return &mDenViscosity; }
	void				setDensityViscosity( float val ) { mDenViscosity = val; }
	// TexCoords viscosity
	float				texCoordViscosity() const { return mTexViscosity; }
	float*				texCoordViscosityAddr() { return &mTexViscosity; }
	void				setTexCoordViscosity( float val ) { mTexViscosity = val; }
	// Rgb viscosity
	float				rgbViscosity() const { return mRgbViscosity; }
	float*				rgbViscosityAddr() { return &mRgbViscosity; }
	void				setRgbViscosity( float val ) { mRgbViscosity = val; }

	// Velocity grid
	VecGrid&			velocity() { return *mVel0; }
	const VecGrid&		velocity() const { return *mVel0; }
	VecT&				velocityAt( int aX, int aY ) { return mVel0->at( aX, aY ); }
	const VecT&			velocityAt( int aX, int aY ) const { return mVel0->at( aX, aY ); }
	void				addVelocity( int aX, int aY, const VecT& aVal );
	void				splatVelocity( float aX, float aY, const VecT& aVal );
	void				clearVelocity();

	// Density grid
	RealGrid&			density() { return *mDen0; }
	const RealGrid&		density() const { return *mDen0; }
	float&				densityAt( int aX, int aY ) { return mDen0->at( aX, aY ); }
	const float&		densityAt( int aX, int aY ) const { return mDen0->at( aX, aY ); }
	void				addDensity( int aX, int aY, float aVal );
	void				splatDensity( float aX, float aY, float aVal );
	void				clearDensity();

	// TexCoord grid
	VecGrid&			texCoord() { return *mTex0; }
	const VecGrid&		texCoord() const { return *mTex0; }
	VecT&				texCoordAt( int aX, int aY ) { return mTex0->at( aX, aY ); }
	const VecT&			texCoordAt( int aX, int aY ) const { return mTex0->at( aX, aY ); }
	void				addTexCoord( int aX, int aY, const VecT& aVal );
	void				splatTexCoord( float aX, float aY, const VecT& aVal );
	void				clearTexCoord();

	// Rgb grid
	RgbGrid&			rgb() { return *mRgb0; }
	const RgbGrid&		rgb() const { return *mRgb0; }
	RgbT&				rgbAt( int aX, int aY ) { return mRgb0->at( aX, aY ); }
	const RgbT&			rgbAt( int aX, int aY ) const { return mRgb0->at( aX, aY ); }
	void				addRgb( int aX, int aY, const RgbT& aVal );
	void				splatRgb( float aX, float aY, const RgbT& aVal );
	void				clearRgb();

	// Clear all
	void				clearAll();
	// Step the simulation
	void				beginSimStepParams( bool& aFtzOn, bool& aDazOn );
	void				endSimStepParams( bool aFtzOn, bool aDazOn );
	void				step();

	// Setup the initial state of the fluid
	virtual void		initSimData();
	// Sets/resets the texture coordinates to initial state
	void				resetTexCoords();
    
private:
	Vec2i					mRes;
	Rectf					mBounds;

	// Cell size is the bounds divided by the resolution, it's often
	// referred to as (dx, dy):
	// dx = (right - left)/res.x
	// dy = (bottom - top)/res.y
	Vec2f					mCellSize;

	// Half divide cell size is 0.5 divided by the cell size. Why half?
	// If you look at the divergence equation:
	//    (u[i+1,j] - u[i-1,j])/(2*dx) + (u[i,j+1] - u[i,j-1])/(2*dy)
	// 1/(2*dx), 1/(2*dy) = 0.5/dx, 0.5/dy 
	// Easy huh?
	Vec2f					mHalfDivCellSize;

	// Sim vars
	float					mDt;
	float					mTime;
	// Number of Jacobi iterations for pressure - default is 10
	int						mNumPressureIters;
	int						mBoundaryType;
	bool					mEnableBuoy;
	float					mAmbTmp;
	float					mMaterialBuoyancy;
	float					mMaterialWeight;
	Colorf					mMinColor;
	Colorf					mMaxColor;
	float					mBuoyancyScale;
	// Vorticity scale should be between [0,1]
	float					mVorticityScale;
	Vec2f					mGravityDir;

	// Enable/disable flags
	bool					mEnableDen;
	bool					mEnableTex;
	bool					mEnableRgb;

	bool					mDiffuseTex;
	bool					mStamStep;	
	bool					mEnableVc;

	// Sim grid vars
	float					mVelDissipation;    // Recommended maximum: 1.000000
	float					mDenDissipation;    // Recommended maximum: 1.000000
	float					mTexDissipation;    // Recommended maximum: 1.000000
	float					mRgbDissipation;    // Recommended maximum: 1.000000
	//
	float					mVelViscosity;      // Recommended minimum: 0.000001
	float					mDenViscosity;      // Recommended minimum: 0.000001
	float					mTexViscosity;      // Recommended minimum: 0.000001
	float					mRgbViscosity;      // Recommended minimum: 0.000001

	// Sim grid data
	VecGridPtr				mVel0, mVel1;
	RealGridPtr				mDen0, mDen1;
	VecGridPtr				mTex0, mTex1;
	RgbGridPtr				mRgb0, mRgb1;
	RealGridPtr				mDivergence;
	RealGridPtr				mPressure;
	RealGridPtr				mCurl;
	RealGridPtr				mCurlLength;

	// Initialize default vars
	void					initDefaultVars();

	void					stepCombined();
	void					stepStam();

public:
	friend std::ostream& operator<<( std::ostream& os, const Fluid2D& obj ) {
		os << "res=" << obj.mRes << ", ";
		os << "boundary=" << obj.mBoundaryType << ", ";
		os << "density=" << obj.mEnableDen << ", ";
		os << "rgb=" << obj.mEnableRgb << ", ";
		os << "tex=" << obj.mEnableTex;
		return os;
	}

	// WARNING: Accesses the raw data for debugging - not really meant for 
	//          normal uses cases. But looks pretty.
	//
	VecGrid&				dbgVel0() { return *mVel0; }
	VecGrid&				dbgVel1() { return *mVel1; }
	RealGrid&				dbgDen0() { return *mDen0; }
	RealGrid&				dbgDen1() { return *mDen1; }
	RealGrid&				dbgDivergence() { return *mDivergence; }
	RealGrid&				dbgPressure() { return *mPressure; }
	RealGrid&				dbgCurl() { return *mCurl; }
	RealGrid&				dbgCurlLength() { return *mCurlLength; }
};

} /* namespace cinderfx */
