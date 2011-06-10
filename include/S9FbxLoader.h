/*
                       __  .__              ________ 
   ______ ____   _____/  |_|__| ____   ____/   __   \
  /  ___// __ \_/ ___\   __\  |/  _ \ /    \____    /
  \___ \\  ___/\  \___|  | |  (  <_> )   |  \ /    / 
 /____  >\___  >\___  >__| |__|\____/|___|  //____/  .co.uk
      \/     \/     \/                    \/         
 
 THE GHOST IN THE CSH
 
 
 S9FbxLoader.h | Part of S9FBX | Created 03/02/2011
 
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

/* Based on the work by Anthony Scavarelli on 11-01-04 and myself */

/* http://forum.libcinder.org/topic/fbx-models-in-cinder */
/* http://area.autodesk.com/forum/autodesk-fbx/fbx-sdk/help-with-skeletal-animation--bone-data/ */

#pragma once

#include "cinder/app/AppBasic.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Matrix.h"
#include "cinder/Font.h"
#include "cinder/Utilities.h"

#include <fbxsdk.h>
#include <list>
#include "Common.h"

using namespace std;
using namespace cinder;
using namespace ci;

namespace S9 {
	
	
	class FbxRotation{
	public:
		
		Matrix44f				baseMatrix;
		shared_ptr<Matrix44f>	realMatrix;	// Shared pointer here too keeps the right matrix in the right place
		Matrix44f				rotMatrix;
		
		shared_ptr<FbxRotation>	parent;
		bool					targeted;
		
	};
	
	class FbxCluster {
		
	public:
		Matrix44f					pretransform;	// For skinning with bones, our matrix comes inbetween
		Matrix44f					posttransform;
		shared_ptr<Matrix44f>		transform;
		
		KFbxCluster::ELinkMode		mode;
		
		shared_ptr<FbxRotation>		bone;
		vector<int>					indicies;
		vector<float>				weights;
		
		// Extents of this cluster
		Vec3f mMax;
		Vec3f mMin;
		Vec3f mCentre;	
		
	};
	
	
	class FbxMaterial {
	public:
		ci::Vec3f		colour;
		gl::Texture		tex;	// Set as a reference to the other textures in our internal struct
		bool			isTextured;
		
	};
	
	
	// Hold in a structure so we don't recurse. Gives us nice vectors/ arrays and indicies.
	
	class FbxMesh {
		
	public:
		vector<Vec3f>		vertices;
		vector<Vec3f>		skinvertices;
		vector<float>		skinweights;
		vector<Matrix44f>	skinmatrices;
		
		float*				floats;
	
		vector<Vec3f>		normals;
		vector<Vec2f>		texcoords;
		vector<int>			indicies;		// Maybe need a long for that?
		vector<int>			matindicies;	// per triangle! (we assume triangles here)
	
		ci::Matrix44f		offset;
		int					numtris;
		int					numverts;
		
		vector< shared_ptr<FbxCluster> > clusters;
		vector< shared_ptr<FbxRotation> > bones;
		
		// Extents of this Mesh
		Vec3f mMax;
		Vec3f mMin;
		
		bool applyMatrices;
		
	};
	
	// Potentially, there could be many meshes in this FBX :S
	
	class FbxDrawable {
	public:
		
		virtual ~FbxDrawable();
		
		vector< shared_ptr<FbxMesh> > meshes;
		vector< shared_ptr<FbxMaterial> > materials;
		map<std::string, gl::Texture>	mMapToTex;
		
	};
	
	
	class S9FbxLoader {
		
		// TODO - This should be a factory for churning out CinderModels or something
		
	public:
		
		S9FbxLoader();
		shared_ptr<FbxDrawable> load(string fileName);
		virtual ~S9FbxLoader();
		bool	isApplyMatrices() {return mApplyMatrices;};

		ci::Matrix44f	getGlobalOffset();
		
	protected:
		
		// Triangulate Recursive Functions
		void triangulateScene(KFbxSdkManager* pSdkManager, KFbxScene* pScene);
		void triangulateNode(KFbxSdkManager* pSdkManager, KFbxNode* pNode);
		void triangulateNodeRecursive(KFbxSdkManager* pSdkManager, KFbxNode* pNode);
		
		// Setup for Cinder Functions
		
		void setupForCinder(shared_ptr<FbxDrawable>  pDrawable);
		void setupForCinderRecursive(shared_ptr<FbxDrawable>  pDrawable, KFbxNode* pNode, KFbxXMatrix& pParentGlobalPosition);
		void setupForCinderSkeleton(shared_ptr<FbxDrawable>  pDrawable, KFbxNode* pNode, KFbxXMatrix& pParentGlobalPosition);
		void setupForCinderMesh(shared_ptr<FbxDrawable>  pDrawable, KFbxNode* pNode, KFbxXMatrix& pParentGlobalPosition,  shared_ptr<FbxMesh> pMesh);
		void setupForCinderDeformations(shared_ptr<FbxDrawable>  pDrawable,KFbxXMatrix& pGlobalPosition,  KFbxMesh* pMesh, shared_ptr<FbxMesh> pCinderMesh); 
		
		// texture and materials
		void loadSupportedMaterials(shared_ptr<FbxDrawable>  pDrawable);
		void loadSupportedMaterialsRecursive(KFbxNode* pNode, shared_ptr<FbxDrawable>  pDrawable);
		gl::Texture loadTexture(KFbxTexture* pTexture, shared_ptr<FbxDrawable>  pDrawable);
					
		
		KArrayTemplate<gl::Texture>		mTextureArray;
	
		// Cinder Related
		ci::Matrix44f				toCinderMatrix(KFbxXMatrix m);
		KFbxXMatrix					toFBXMatrix(ci::Matrix44f  m);
		
		string			mFileName;
		string			mPath;
		
		bool			mApplyMatrices;
		
		KFbxSdkManager* mSdkManager;
		KFbxImporter*	mImporter;
		KFbxScene*		mScene;
		
		
		KTime mPeriod, mStart, mStop, mCurrentTime; // TODO - Not used yet but will be for animation
	
	};
	
} // End Namespace S9