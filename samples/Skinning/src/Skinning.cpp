/*
                       __  .__              ________ 
   ______ ____   _____/  |_|__| ____   ____/   __   \
  /  ___// __ \_/ ___\   __\  |/  _ \ /    \____    /
  \___ \\  ___/\  \___|  | |  (  <_> )   |  \ /    / 
 /____  >\___  >\___  >__| |__|\____/|___|  //____/  .co.uk
      \/     \/     \/                    \/         
 
 THE GHOST IN THE CSH
 
 
skinning.cpp | Part of Skinning | Created 13/05/2011
 
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


#include "Skinning.h"


ConfigScene gConfigScene;

void staticMouseDownHandler( MouseEvent event );
void staticMouseUpHandler( MouseEvent event );

void Skinning::setup() {
	
	mShaderPhong = gl::GlslProg(	loadResource( RES_PHONG_VERT_GLSL ),	loadResource( RES_PHONG_FRAG_GLSL ) );
			
	// Set params
	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 400 ) );
	mParams.addParam( "Scene Rotation", &gConfigScene.orientation );
	
	gConfigScene.eye = Vec3f(0,0,10);
	
	mParams.addParam( "Bone Rotation", &mTestBoneRot );
	mParams.addParam( "BoneID", &mBoneID);
	mParams.addParam( "Zoom ", &gConfigScene.eye.z, "min=5.0 max=100.0 step=1.0");
	mParams.addParam( "Scale ", &gConfigScene.scale, "min=0.001 max=10.0 step=0.001");
	
	gConfigScene.scale = 0.04;

	mShowParams = false;
	mBoneID = mPrevBoneID = -1;
	mDrawFilled = true;
	mDrawNormals = false;
	// OpenGL Constants

	gl::enableDepthRead();
	gl::enableDepthWrite();

	glEnable(GL_TEXTURE_2D);

	cout << getAppPath().string() + "/Contents/Resources/skinning_config.xml" << endl;

#ifdef CINDER_MAC
	XmlTree doc( loadFile(getAppPath().string() + "/Contents/Resources/skinning_config.xml"));
#else if CINDER_MSW
	XmlTree doc( loadFile(getAppPath().string() + "../../Resources/skinning_config.xml"));
#endif
	string fbxpath = doc.getChild("/skinning/fbx").getValue();

	console() << fbxpath << endl;
#ifdef CINDER_MAC
	pDrawable = mFBXLoader.load(getAppPath().string() + "/Contents/Resources/" + fbxpath.c_str());
#else if CINDER_MSW
	pDrawable = mFBXLoader.load(getAppPath().string() + "../" + fbxpath.c_str());
#endif
}


void Skinning::resize( ResizeEvent event ){
}

void Skinning::mouseDrag( MouseEvent event ){

}

void Skinning::shutdown() {
}

void Skinning::keyDown( KeyEvent event ) {
	if( event.getChar() == 'f' )
		setFullScreen( ! isFullScreen() );
	if( event.getChar() == 'r' )
		mFBXDrawer.resetRotations(pDrawable->meshes[0]);
	if( event.getChar() == 'p' )
		mShowParams = !mShowParams;
	if( event.getChar() == 'n' )
		mDrawNormals = !mDrawNormals;
}

void Skinning::update() {
	
	// Bone rotations
	Matrix44d m = mTestBoneRot.toMatrix44();
	mFBXDrawer.rotateBone(pDrawable->meshes[0], mBoneID, m );
}
					

void Skinning::resetBones() {
	//mFBXLoader.resetRotations();
}

void Skinning::drawGeometry() {

	glPushMatrix();
	gl::rotate(gConfigScene.orientation);

	gl::color(Color(0,0,0));

	// Scale it, cos its massive!
	glPushMatrix();
	glScalef(gConfigScene.scale, gConfigScene.scale, gConfigScene.scale);
	mFBXDrawer.draw(pDrawable);
	glPopMatrix();
	glPopMatrix();
}

void Skinning::drawNormals() {

	glPushMatrix();
	// Draw Normals
	gl::rotate(gConfigScene.orientation);
	glPushMatrix();
	glScalef(gConfigScene.scale, gConfigScene.scale, gConfigScene.scale);
	mFBXDrawer.drawNormals(pDrawable);
	glPopMatrix();
	glPopMatrix();
	
}


void Skinning::draw()
{
	gl::clear( Color( 1.0f, 1.0f, 1.0f ) );	
	
	mCam.lookAt( gConfigScene.eye, Vec3f::zero() );
	mCam.setPerspective( 60, getWindowAspectRatio(), 0.01, 500 );
	gl::setMatrices( mCam );
	
	
	glEnable(GL_LIGHT0);
	
	GLfloat pos[] = {0.0, 2.0, 15.0, 1.0};
	GLfloat diff[] = {1.0, 1.0, 1.0, 1.0};
	
	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
	
	mShaderPhong.bind();
	mShaderPhong.uniform("shininess",128.0f);
	mShaderPhong.uniform("tex",0);
	
	
	drawGeometry();
	
	mShaderPhong.unbind();
	glDisable(GL_LIGHT0);
	
	if (mDrawNormals)
		drawNormals();
	
	// Params
	if (mShowParams){
		params::InterfaceGl::draw();
	}
	
}

// This line tells Flint to actually create the application
CINDER_APP_BASIC( Skinning, RendererGl )
