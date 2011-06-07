/*
                       __  .__              ________ 
   ______ ____   _____/  |_|__| ____   ____/   __   \
  /  ___// __ \_/ ___\   __\  |/  _ \ /    \____    /
  \___ \\  ___/\  \___|  | |  (  <_> )   |  \ /    / 
 /____  >\___  >\___  >__| |__|\____/|___|  //____/  .co.uk
	  \/     \/     \/                    \/         
 
 THE GHOST IN THE CSH
 
 
 Skinning.h | Part of Skinning | Created 13/05/2011
 
 Copyright (c) 2010 Benjamin Blundell, www.section9.co.uk
 *** Section9 ***
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Section9 nor the names of its contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ***********************************************************************/

#pragma once

#include "cinder/app/AppBasic.h"
#include "cinder/Rand.h"
#include "cinder/Camera.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Perlin.h"
#include "cinder/Vector.h"
#include "cinder/Sphere.h"
#include "cinder/gl/Texture.h"
#include "cinder/Xml.h"
#include "cinder/params/Params.h"
#include "cinder/gl/Vbo.h"
#include "cinder/ObjLoader.h"
#include "cinder/TriMesh.h"
#include "cinder/Font.h"
#include "cinder/Utilities.h"

#include "S9FbxLoader.h"
#include "config.h"


using namespace ci;
using namespace ci::app;
using namespace std;

class Skinning : public AppBasic {
public:

	void setup();
	void mouseDrag( MouseEvent event );
	void keyDown( KeyEvent event );
	void draw();
	void shutdown();
	
protected:
	
	void resize( ResizeEvent event );
	void drawGeometry();
	void update();
	void resetBones();
	
	// Cameras
	CameraPersp			mCam;
	
	// Params
	params::InterfaceGl	mParams;
	bool				mShowParams;
	
	// Geometry
	TriMesh				mCompanionCube;
	gl::VboMesh			mCompanionCubeVBO;
	Sphere				mSphere;
	
	// Font for FPS
	Font				mFont;
	
	// FBX Stuff
	S9FbxLoader			mFBXLoader;
	bool				mDrawFilled;
	
	ci::Quatf			mTestBoneRot;
	float				mTestBoneTrans;
	int					mBoneID;
	int					mPrevBoneID;

};
