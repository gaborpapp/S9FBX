/*
 __  .__              ________ 
 ______ ____   _____/  |_|__| ____   ____/   __   \
 /  ___// __ \_/ ___\   __\  |/  _ \ /    \____    /
 \___ \\  ___/\  \___|  | |  (  <_> )   |  \ /    / 
 /____  >\___  >\___  >__| |__|\____/|___|  //____/  .co.uk
 \/     \/     \/                    \/         
 
 THE GHOST IN THE CSH
 
 
 S9FbxLoader.h | Part of Skinning | Created 03/02/2011
 
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

/* Based on the work by anthonyScavarelli on 11-01-04 and myself */

/* http://forum.libcinder.org/topic/fbx-models-in-cinder */
/* http://area.autodesk.com/forum/autodesk-fbx/fbx-sdk/help-with-skeletal-animation--bone-data/ */

#include "cinder/app/AppBasic.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Matrix.h"
#include "cinder/Font.h"
#include "cinder/Utilities.h"

#include <fbxsdk.h>
#include "Common.h"

using namespace std;
using namespace cinder;
using namespace ci;


typedef enum {
	FBX_DRAW_WIREFRAME,
	FBX_DRAW_FILLED,
	FBX_DRAW_TEXTURED,
	
} FbxDrawMethod;

typedef struct {
	KFbxXMatrix matrix;
	KFbxXMatrix rmatrix;
	KFbxNode	*node;
	bool		targeted;
	ci::Matrix44f	cindermatrix;	// We are doing comparisions so we dont want to loose precision
	// when we compare matrices
	
}FbxRotation;


class S9FbxLoader {
	
public:
	
	S9FbxLoader();
	bool load(string fileName);
	virtual ~S9FbxLoader();
	void draw();
	void rotateBone(int boneid, ci::Matrix44f &mat); 
	void resetRotations();
	Vec3f	getBoneVector(int boneid);
	ci::Matrix44f getCinderMatrix(int boneid);
	FbxRotation getCustomRotation(int idx);
	int	getCustomRotation(KFbxNode *pNode);
	
	FbxDrawMethod	mDrawMethod;
	bool			mDrawBones;
	
protected:
	
	// Triangulate Recursive Functions
	void triangulateScene(KFbxSdkManager* pSdkManager, KFbxScene* pScene);
	void triangulateNode(KFbxSdkManager* pSdkManager, KFbxNode* pNode);
	void triangulateNodeRecursive(KFbxSdkManager* pSdkManager, KFbxNode* pNode);
	
	
	// Drawing functions
	void drawNodeRecursive(KFbxNode* pNode, KFbxXMatrix& pParentGlobalPosition, int lvl);
	void drawNode(KFbxNode* pNode,  KFbxXMatrix& pParentGlobalPosition, KFbxXMatrix& pGlobalPosition);
	void drawMesh(KFbxNode* pNode, KFbxXMatrix& pGlobalPosition);
	
	void drawMeshGL(KFbxXMatrix& pGlobalPosition, KFbxMesh* pMesh, KFbxVector4* pVertexArray);
	void drawSkeleton(KFbxNode* pNode, KFbxXMatrix& pParentGlobalPosition, KFbxXMatrix& pGlobalPosition);
	
	
	// Deformation
	void computeClusterDeformation(KFbxXMatrix& pGlobalPosition,  KFbxMesh* pMesh,  KFbxVector4* pVertexArray);
	void setupBones();
	void rotateBoneRecursive(KFbxNode *pNode, KFbxXMatrix mat, KFbxXMatrix rmat);
	
	// Misc
	void setupInternalRefs(KFbxNode* pNode);
	void preparePointCacheData(KFbxScene* pScene);
	void readVertexCacheData(KFbxMesh* pMesh, KFbxVector4* pVertexArray);
	
	// texture
	void loadSupportedTextures();
	void loadSupportedTexturesRecursive(KFbxNode* pNode);
	void loadTexture(KFbxTexture* pTexture);
	
	
	// Cinder Related
	ci::Matrix44f					toCinderMatrix(KFbxXMatrix m);
	KFbxXMatrix						toFBXMatrix(ci::Matrix44f  m);
	
	
	// Texture Related
	KArrayTemplate<gl::Texture>		mTextureArray;
	map<std::string, gl::Texture>	mMapToTex;	// Mapping from Cinder tex to FBX Texture for lookups
	
	string			mFileName;
	string			mPath;
	
	KFbxSdkManager* mSdkManager;
	KFbxImporter*	mImporter;
	KFbxScene*		mScene;
	KFbxMesh*		mMesh;
	
	int				mPoseIndex;
	
	KTime mPeriod, mStart, mStop, mCurrentTime; // TODO - Not used yet but will be for animation
	
	vector<FbxRotation > mBones;
};