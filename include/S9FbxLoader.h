/*
 __  .__              ________ 
 ______ ____   _____/  |_|__| ____   ____/   __   \
 /  ___// __ \_/ ___\   __\  |/  _ \ /    \____    /
 \___ \\  ___/\  \___|  | |  (  <_> )   |  \ /    / 
 /____  >\___  >\___  >__| |__|\____/|___|  //____/  .co.uk
 \/     \/     \/                    \/         
 
 THE GHOST IN THE CSH
 
 */

/**
 * @brief	The NITE and OpenNI Version of the Kinect Stuff. You need to have OpenNI all installed
 *			and in the right place!
 *			Library Search Paths need to point to where you have the Kinect OpenNI Linux x86 libs
 *			OPENNI_LIBS custom setting should point to the lib dir of this project
 *			There are quite a few XML Files that are needed as well that you should configure
 *
 * @file	S9FbxLoader.h
 * @author	Benjamin Blundell <oni@section9.co.uk>
 * @date	13/04/2011
 * Part of  FBX Block
 * 
 * @section LICENSE 
 * 
 * Copyright (c) 2010 Benjamin Blundell, www.section9.co.uk
 * Section9 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *	Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *  
 *  Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *  
 *  Neither the name of Section9 nor the names of its contributors
 *  may be used to endorse or promote products derived from this software
 *  without specific prior written permission.
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
 */


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
#include <string>
#include <map>
#include "Common.h"

using namespace cinder;
using namespace ci;

namespace S9 {
	
	
	class FbxRotation{
	public:
		
		Matrix44d					baseMatrix;
		std::shared_ptr<Matrix44d>	realMatrix;	// Shared pointer here too keeps the right matrix in the right place
		Matrix44d					rotMatrix;
		
		std::shared_ptr<FbxRotation>	parent;
		bool					targeted;
		
	};
	
	class FbxCluster {
		
	public:
		Matrix44d					pretransform;	// For skinning with bones, our matrix comes inbetween
		Matrix44d					posttransform;
		std::shared_ptr<Matrix44d>		transform;
		
		KFbxCluster::ELinkMode		mode;
		
		std::shared_ptr<FbxRotation>	bone;
		std::vector<int>				indicies;
		std::vector<int>				normalindicies;	// Annoyingly a vertex can have multiple normals >< 
		std::vector<float>				weights;
		
		// Extents of this cluster
		Vec3d mMax;
		Vec3d mMin;
		Vec3d mCentre;	
		
	};
	
	
	class FbxMaterial {
	public:
		ci::Vec3d		colour;
		gl::Texture		tex;	// Set as a reference to the other textures in our internal struct
		bool			isTextured;
		
	};
	
	
	// Hold in a structure so we don't recurse. Gives us nice vectors/ arrays and indicies.
	
	class FbxMesh {
		
	public:
		std::vector<Vec3d>		vertices;
		std::vector<Vec3d>		skinvertices;
		std::vector<float>		skinweights;
		std::vector<Matrix44d>	skinmatrices;
		
		
		std::vector<Vec3d>		normals;
		std::vector<Vec3d>		skinnormals;
		std::vector<Vec2d>		texcoords;
		std::vector<int>		indicies;				// Maybe need a long for that?
		std::vector<int>		texindicies;			// Apparently we need this too!
		std::vector<int>		indiciesToIterative;	// Map from vertex indicies to iterative array position
		std::vector<int>		matindicies;			// per triangle! (we assume triangles here)
		
		ci::Matrix44d		offset;
		int					numtris;
		int					numverts;
		
		std::vector< std::shared_ptr<FbxCluster> > clusters;
		std::vector< std::shared_ptr<FbxRotation> > bones;
		
		std::map< std::string, int > boneNameToIndex; // Map from bone name to bone index

		// Extents of this Mesh
		Vec3d mMax;
		Vec3d mMin;
		
		bool mDeform;
		
	};
	
	// Potentially, there could be many meshes in this FBX :S
	
	class FbxDrawable {
	public:
		
		virtual ~FbxDrawable();
		
		std::vector< std::shared_ptr<FbxMesh> > meshes;
		std::vector< std::shared_ptr<FbxMaterial> > materials;
		std::map<std::string, gl::Texture>	mMapToTex;
		
	};
	
	
	class S9FbxLoader {
		
		// TODO - This should be a factory for churning out CinderModels or something
		
	public:
		
		S9FbxLoader();
		std::shared_ptr<FbxDrawable> load(std::string fileName);
		virtual ~S9FbxLoader();
		bool	isApplyMatrices() {return mApplyMatrices;};
		
		ci::Matrix44d	getGlobalOffset();
		
	protected:
		
		// Triangulate Recursive Functions
		void triangulateScene(KFbxSdkManager* pSdkManager, KFbxScene* pScene);
		void triangulateNode(KFbxSdkManager* pSdkManager, KFbxNode* pNode);
		void triangulateNodeRecursive(KFbxSdkManager* pSdkManager, KFbxNode* pNode);
		
		// Setup for Cinder Functions
		
		void setupForCinder(std::shared_ptr<FbxDrawable>  pDrawable);
		void setupForCinderRecursive(std::shared_ptr<FbxDrawable>  pDrawable, KFbxNode* pNode, KFbxXMatrix& pParentGlobalPosition);
		void setupForCinderSkeleton(std::shared_ptr<FbxDrawable>  pDrawable, KFbxNode* pNode, KFbxXMatrix& pParentGlobalPosition);
		void setupForCinderMesh(std::shared_ptr<FbxDrawable>  pDrawable, KFbxNode* pNode, KFbxXMatrix& pParentGlobalPosition,  std::shared_ptr<FbxMesh> pMesh);
		void setupForCinderDeformations(std::shared_ptr<FbxDrawable>  pDrawable,KFbxXMatrix& pGlobalPosition,  KFbxMesh* pMesh, std::shared_ptr<FbxMesh> pCinderMesh); 
		
		// texture and materials
		void loadSupportedMaterials(std::shared_ptr<FbxDrawable>  pDrawable);
		void loadSupportedMaterialsRecursive(KFbxNode* pNode, std::shared_ptr<FbxDrawable>  pDrawable);
		gl::Texture loadTexture(KFbxTexture* pTexture, std::shared_ptr<FbxDrawable>  pDrawable);
		
		KArrayTemplate<gl::Texture>		mTextureArray;
		
		// Cinder Related
		ci::Matrix44d				toCinderMatrix(KFbxXMatrix m);
		KFbxXMatrix					toFBXMatrix(ci::Matrix44d  m);
		
		std::string			mFileName;
		std::string			mPath;
		
		bool			mApplyMatrices;
		
		KFbxSdkManager* mSdkManager;
		KFbxImporter*	mImporter;
		KFbxScene*		mScene;
		
		
		KTime mPeriod, mStart, mStop, mCurrentTime; // TODO - Not used yet but will be for animation
		
	};
	
} // End Namespace S9
