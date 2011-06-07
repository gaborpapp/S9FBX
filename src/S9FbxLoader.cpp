/*
                       __  .__              ________ 
   ______ ____   _____/  |_|__| ____   ____/   __   \
  /  ___// __ \_/ ___\   __\  |/  _ \ /    \____    /
  \___ \\  ___/\  \___|  | |  (  <_> )   |  \ /    / 
 /____  >\___  >\___  >__| |__|\____/|___|  //____/  .co.uk
	  \/     \/     \/                    \/         
 
 THE GHOST IN THE CSH
 
 
 S9FbxLoader.cpp | Part of Canned | Created 03/02/2011
 
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

#include "S9FbxLoader.h"

namespace S9{
	
	
#pragma mark setup and destroy functions
	
	S9FbxLoader::S9FbxLoader() {
		mCurrentTime = 0; // TODO - should be set to start when animation is done
		mDrawMethod = FBX_DRAW_TEXTURED;
		mDrawBones = false;
	}
	
	S9FbxLoader::~S9FbxLoader() {
	}
	
	bool S9FbxLoader::load(string fileName){
		
		
		mFileName = fileName.substr(fileName.find_last_of("/") + 1); // Guess is linux / mac only then!
		mPath = fileName.substr(0, fileName.find_last_of("/") ); 
		
		InitializeSdkObjects(mSdkManager, mScene);
		
		if (mSdkManager) {
			
			int lFileFormat = -1;
			mImporter = KFbxImporter::Create(mSdkManager,"");
			if (!mSdkManager->GetIOPluginRegistry()->DetectReaderFileFormat(mFileName.c_str(), lFileFormat) ){
				lFileFormat = mSdkManager->GetIOPluginRegistry()->FindReaderIDByDescription( "FBX binary (*.fbx)" );
			}
			
			// Initialize the importer by providing a filename.
			if(mImporter->Initialize(fileName.c_str(), lFileFormat) == true) {
				app::console() << "Importing FBX file " << fileName << endl;
				
				// Make sure that the scene is ready to load.
				
				if(mImporter->Import(mScene) == true)
				{
					
					KFbxAxisSystem SceneAxisSystem = mScene->GetGlobalSettings().GetAxisSystem();
					KFbxAxisSystem OurAxisSystem(KFbxAxisSystem::YAxis, KFbxAxisSystem::ParityOdd, KFbxAxisSystem::RightHanded);
					
					if( SceneAxisSystem != OurAxisSystem ){
						OurAxisSystem.ConvertScene(mScene);
					}
					
					KFbxSystemUnit SceneSystemUnit = mScene->GetGlobalSettings().GetSystemUnit();
					
					if( SceneSystemUnit.GetScaleFactor() != 1.0 ){
						KFbxSystemUnit OurSystemUnit(1.0);
						OurSystemUnit.ConvertScene(mScene);
					}
					
					
					triangulateScene(mSdkManager, mScene);
					
					preparePointCacheData(mScene);
					
					setupInternalRefs(mScene->GetRootNode());
					
					setupBones();
					
					loadSupportedMaterials();
					
					// TODO - animation and poses from the previous example need to be here
					
					
					// Initialize the frame period.
					
				}
				else{
					app::console() << "Unable to import file " << endl;
					return false;
				}
				
				mImporter->Destroy();
				
				
			}
			else{
				app::console() << "Unable to open file " << mImporter->GetLastErrorString();
				return false;
			}
		}
		else{
			app::console() << "Unable to create the FBX SDK manager"  << endl;
			return false;
		}
		
		// When mPoseIndex is not -1, draw the scene at that pose
		// TODO - Multiple pose support
		mPoseIndex = -1;
		
		return true;
		
	}
	
	
#pragma mark Trianglulation Methods
	
	// ********************************************************************************
	// TRIANGULATION Methods
	// ********************************************************************************
	
	void S9FbxLoader::triangulateScene(KFbxSdkManager* pSdkManager, KFbxScene* pScene) {
		int i, lCount = pScene->GetRootNode()->GetChildCount();
		
		for (i = 0; i < lCount; i++){
			triangulateNodeRecursive(pSdkManager, pScene->GetRootNode()->GetChild(i));
		}
	}
	
	
	void S9FbxLoader::triangulateNode(KFbxSdkManager* pSdkManager, KFbxNode* pNode) {
		KFbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();
		
		if (lNodeAttribute)
		{
			if (lNodeAttribute->GetAttributeType() == KFbxNodeAttribute::eMESH)
			{
				KFbxMesh* lMesh = (KFbxMesh*) pNode->GetNodeAttribute();
				
				if (!lMesh->IsTriangleMesh())
				{
					KFbxGeometryConverter converter( pSdkManager );
					//lMesh = converter.TriangulateMesh( lMesh ); //doesn't change source geometry
					converter.TriangulateInPlace( pNode );
				}
			}
		}
		
	}
	
	
	void S9FbxLoader::triangulateNodeRecursive(KFbxSdkManager* pSdkManager, KFbxNode* pNode) {
		triangulateNode(pSdkManager, pNode);
		
		int i, lCount = pNode->GetChildCount();
		
		for (i = 0; i < lCount; i++)
		{
			triangulateNodeRecursive(pSdkManager, pNode->GetChild(i));
		}
	}
	
	
#pragma mark drawing methods
	
	// ********************************************************************************
	// DRAWING Methods
	// ********************************************************************************
	
	void S9FbxLoader::draw() {
		
		KFbxXMatrix lDummyGlobalPosition;
		
		int i, lCount = mScene->GetRootNode()->GetChildCount();
		
		for (i = 0; i < lCount; i++){
			drawNodeRecursive(mScene->GetRootNode()->GetChild(i), lDummyGlobalPosition,1);
		}
		
	}
	
	void S9FbxLoader::drawNodeRecursive(KFbxNode* pNode, KFbxXMatrix& pParentGlobalPosition, int lvl) {
		
		KFbxXMatrix lGlobalPosition = GetGlobalPosition(pNode, mCurrentTime, &pParentGlobalPosition);
		KFbxXMatrix lGeometryOffset = GetGeometry(pNode);
		KFbxXMatrix lGlobalOffPosition = lGlobalPosition * lGeometryOffset;	
		
		drawNode(pNode, pParentGlobalPosition, lGlobalOffPosition);
		
		int i, lCount = pNode->GetChildCount();
		
		for (i = 0; i < lCount; i++){
			drawNodeRecursive(pNode->GetChild(i), lGlobalPosition,lvl+1);
		}
	}
	
	
	// Draw the node following the content of it's node attribute.
	// TODO - This did have a POSE option passed to it but it doesnt at the moment
	
	void S9FbxLoader::drawNode(KFbxNode* pNode,  KFbxXMatrix& pParentGlobalPosition, KFbxXMatrix& pGlobalPosition) {
		KFbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();
		
		if (lNodeAttribute) {
			
			if (lNodeAttribute->GetAttributeType() == KFbxNodeAttribute::eMESH) {
				drawMesh(pNode, pGlobalPosition);
			}
			else if (lNodeAttribute->GetAttributeType() == KFbxNodeAttribute::eSKELETON && mDrawBones)
			{
				drawSkeleton(pNode, pParentGlobalPosition, pGlobalPosition);
			}
		}
	}
	
	
	void S9FbxLoader::drawMesh(KFbxNode* pNode, KFbxXMatrix& pGlobalPosition) {
		KFbxMesh* lMesh = (KFbxMesh*) pNode->GetNodeAttribute();
		
		int lClusterCount = 0;
		int lSkinCount= 0;
		int lVertexCount = lMesh->GetControlPointsCount();
		
		// No vertex to draw.
		if (lVertexCount == 0)
			return;
		
		// Create a copy of the vertex array to receive vertex deformations.
		KFbxVector4* lVertexArray = new KFbxVector4[lVertexCount];
		memcpy(lVertexArray, lMesh->GetControlPoints(), lVertexCount * sizeof(KFbxVector4));
		
		// Active vertex cache deformer will overwrite any other deformer
		if (lMesh->GetDeformerCount(KFbxDeformer::eVERTEX_CACHE) &&
			(static_cast<KFbxVertexCacheDeformer*>(lMesh->GetDeformer(0, KFbxDeformer::eVERTEX_CACHE)))->IsActive())
		{
			readVertexCacheData(lMesh, lVertexArray);
		}
		else
		{
			
			// TODO - This is probably not needed for our simple mesh
			/*   if (lMesh->GetShapeCount()){
			 computeShapeDeformation(pNode, lMesh, pTime, lVertexArray);
			 }*/
			
			
			lSkinCount = lMesh->GetDeformerCount(KFbxDeformer::eSKIN);
			for( int i=0; i< lSkinCount; i++)
				lClusterCount += ((KFbxSkin *)(lMesh->GetDeformer(i, KFbxDeformer::eSKIN)))->GetClusterCount();
			
			if (lClusterCount){
				computeClusterDeformation(pGlobalPosition, lMesh, lVertexArray);
			}
		}
		switch (mDrawMethod) {
			case FBX_DRAW_TEXTURED:
				drawMeshGLTex(pNode, pGlobalPosition, lMesh, lVertexArray);
				break;
			case FBX_DRAW_FILLED:
				drawMeshGLSolid(pNode, pGlobalPosition, lMesh, lVertexArray);
				break;
			case FBX_DRAW_WIREFRAME:
				drawMeshGLWire(pNode, pGlobalPosition, lMesh, lVertexArray);
				drawMeshExtents(pNode, pGlobalPosition, lMesh, lVertexArray);
				break;
			default:
				break;
		}    
		
		delete [] lVertexArray;
	}
	
	
#pragma mark Draw MeshGL Wireframe
	
	void S9FbxLoader::drawMeshGLWire(KFbxNode* pNode, KFbxXMatrix& pGlobalPosition, KFbxMesh* pMesh, KFbxVector4* pVertexArray)
	{	
		
		//get normals
		KFbxLayerElementArrayTemplate<KFbxVector4> *lNormals = NULL;
		bool normalStatus = pMesh->GetNormals(&lNormals);
		
		glPushMatrix();
		glMultMatrixd((double*) pGlobalPosition);
		
		int lPolygonIndex;
		int lPolygonCount = pMesh->GetPolygonCount();
		
		for (lPolygonIndex = 0; lPolygonIndex < lPolygonCount; lPolygonIndex++) {
			
			glColor3f(0,0,0);
			
			int lVerticeIndex;
			int lVerticeCount = pMesh->GetPolygonSize(lPolygonIndex);
			
			glLineWidth(1.0f);
			
			glBegin(GL_LINE_LOOP);
			
			for (lVerticeIndex = 0; lVerticeIndex < lVerticeCount; lVerticeIndex++){
				
				if ( /*normalStatus &&*/ lNormals /*&& lNormals->GetCount() == lControlPointsCount*/) {
					//GetPolygonVertexNormal(int pPolyIndex, int pVertexIndex, KFbxVector4 &pNormal)
					KFbxVector4 pNormal;
					pMesh->GetPolygonVertexNormal(lPolygonIndex, lVerticeIndex, pNormal);
					
					//printf("normals!");
					glNormal3d(pNormal[0], pNormal[1] /* * -1.0 */, pNormal[2]);
					//					glNormal3d(lNormals->GetAt(lCurrentUVIndex)[0], lNormals->GetAt(lCurrentUVIndex)[1], lNormals->GetAt(lCurrentUVIndex)[2]);
				}
				
				glVertex3dv((GLdouble *)pVertexArray[pMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex)]);
			}
			glEnd();		
		}
		glPopMatrix();
	}
	
	
#pragma mark Draw MeshGL Extents
	
	void S9FbxLoader::drawMeshExtents(KFbxNode* pNode, KFbxXMatrix& pGlobalPosition, KFbxMesh* pMesh, KFbxVector4* pVertexArray)
	{			
		int lPolygonIndex;
		int lPolygonCount = pMesh->GetPolygonCount();
	
		mMaxX = mMaxY = mMaxZ = 0.0;
		mMinX = mMinY = mMinZ = 10000.0;
		
		for (lPolygonIndex = 0; lPolygonIndex < lPolygonCount; lPolygonIndex++) {
			
			int lVerticeIndex;
			int lVerticeCount = pMesh->GetPolygonSize(lPolygonIndex);
			
			for (lVerticeIndex = 0; lVerticeIndex < lVerticeCount; lVerticeIndex++){
				
				KFbxVector4 pv = pVertexArray[pMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex)];
				mMaxX = pv[0] > mMaxX ? pv[0] : mMaxX;
				mMaxY = pv[1] > mMaxY ? pv[1] : mMaxY;
				mMaxZ = pv[2] > mMaxZ ? pv[2] : mMaxZ;
				
				mMinX = pv[0] < mMinX ? pv[0] : mMinX;
				mMinY = pv[1] < mMinY ? pv[1] : mMinY;
				mMinZ = pv[2] < mMinZ ? pv[2] : mMinZ;
				
			}
		}
		
		
		glColor3f(1,0,1);
		glLineWidth(3.0f);
		glPushMatrix();
		glMultMatrixd((double*) pGlobalPosition);
		
		glBegin(GL_LINES);
		
		// Top
		glVertex3f(mMinX, mMaxY, mMinZ); glVertex3f(mMaxX, mMaxY, mMinZ);
		glVertex3f(mMaxX, mMaxY, mMinZ); glVertex3f(mMaxX, mMaxY, mMaxZ);
		glVertex3f(mMaxX, mMaxY, mMaxZ); glVertex3f(mMinX, mMaxY, mMaxZ);
		glVertex3f(mMinX, mMaxY, mMaxZ); glVertex3f(mMinX, mMaxY, mMinZ);
		
		
		// Bottom
		
		glVertex3f(mMinX, mMinY, mMinZ); glVertex3f(mMaxX, mMinY, mMinZ);
		glVertex3f(mMaxX, mMinY, mMinZ); glVertex3f(mMaxX, mMinY, mMaxZ);
		glVertex3f(mMaxX, mMinY, mMaxZ); glVertex3f(mMinX, mMinY, mMaxZ);
		glVertex3f(mMinX, mMinY, mMaxZ); glVertex3f(mMinX, mMinY, mMinZ);
		
		
		// Front
		glVertex3f(mMinX, mMaxY, mMaxZ); glVertex3f(mMinX, mMinY, mMaxZ);
		glVertex3f(mMaxX, mMaxY, mMaxZ); glVertex3f(mMaxX, mMinY, mMaxZ);
		
		// Back
		glVertex3f(mMinX, mMaxY, mMinZ); glVertex3f(mMinX, mMinY, mMinZ);
		glVertex3f(mMaxX, mMaxY, mMinZ); glVertex3f(mMaxX, mMinY, mMinZ);
		
		
		glEnd();
		
		glPopMatrix();
	}
	
	
	
#pragma mark Draw MeshGL Solid
	
	
	void S9FbxLoader::drawMeshGLSolid(KFbxNode* pNode, KFbxXMatrix& pGlobalPosition, KFbxMesh* pMesh, KFbxVector4* pVertexArray)
	{	
		
		//get normals
		KFbxLayerElementArrayTemplate<KFbxVector4> *lNormals = NULL;
		bool normalStatus = pMesh->GetNormals(&lNormals);
		
		
		glPushMatrix();
		glMultMatrixd((double*) pGlobalPosition);
		
		int lPolygonIndex;
		int lPolygonCount = pMesh->GetPolygonCount();
		
		for (lPolygonIndex = 0; lPolygonIndex < lPolygonCount; lPolygonIndex++) {
			
			int lMatId = -1;
			KFbxLayerElement::EMappingMode lMappingMode = KFbxLayerElement::eNONE;
			KFbxLayerElementArrayTemplate<KFbxVector2>* lUVArray = NULL;
			pMesh->GetTextureUV(&lUVArray, KFbxLayerElement::eDIFFUSE_TEXTURES);
			
			for (int l = 0; l < pMesh->GetLayerCount(); l++) {
				KFbxLayerElementMaterial* lLayerMaterial = pMesh->GetLayer(l)->GetMaterials();
				if (lLayerMaterial) {
					
					KFbxSurfaceMaterial* lMaterial = NULL;
					pNode->GetMaterial(lLayerMaterial->GetIndexArray().GetAt(l));
					lMatId = lLayerMaterial->GetIndexArray().GetAt(lPolygonIndex);
				}
			}
			
			if (lMatId >=0) {
				glColor3f(mMaterials[lMatId].colour.x, mMaterials[lMatId].colour.y,mMaterials[lMatId].colour.z);
			}
			
			int lVerticeIndex;
			int lVerticeCount = pMesh->GetPolygonSize(lPolygonIndex);
			
			
			glBegin(GL_TRIANGLES);
			
			for (lVerticeIndex = 0; lVerticeIndex < lVerticeCount; lVerticeIndex++){
				int lCurrentUVIndex;
				
				if (lMappingMode == KFbxLayerElement::eBY_POLYGON_VERTEX){
					lCurrentUVIndex = pMesh->GetTextureUVIndex(lPolygonIndex, lVerticeIndex);
				}
				else {
					lCurrentUVIndex = pMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
				}
				
				if ( /*normalStatus &&*/ lNormals /*&& lNormals->GetCount() == lControlPointsCount*/) {
					//GetPolygonVertexNormal(int pPolyIndex, int pVertexIndex, KFbxVector4 &pNormal)
					KFbxVector4 pNormal;
					pMesh->GetPolygonVertexNormal(lPolygonIndex, lVerticeIndex, pNormal);
					
					//printf("normals!");
					glNormal3d(pNormal[0], pNormal[1] /* * -1.0 */, pNormal[2]);
					//					glNormal3d(lNormals->GetAt(lCurrentUVIndex)[0], lNormals->GetAt(lCurrentUVIndex)[1], lNormals->GetAt(lCurrentUVIndex)[2]);
				}
				
				glVertex3dv((GLdouble *)pVertexArray[pMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex)]);
			}
			glEnd();		
		}
		glPopMatrix();
	}
	
	
# pragma mark Draw MeshGL Textured
	
	void S9FbxLoader::drawMeshGLTex(KFbxNode* pNode, KFbxXMatrix& pGlobalPosition, KFbxMesh* pMesh, KFbxVector4* pVertexArray)
	{	
	//	glEnable(GL_TEXTURE_2D);
	//	glEnable(GL_COLOR_MATERIAL);
		
		//get normals
		KFbxLayerElementArrayTemplate<KFbxVector4> *lNormals = NULL;
		bool normalStatus = pMesh->GetNormals(&lNormals);
		
		
		glPushMatrix();
		glMultMatrixd((double*) pGlobalPosition);
		
		int lPolygonIndex;
		int lPolygonCount = pMesh->GetPolygonCount();
		
		for (lPolygonIndex = 0; lPolygonIndex < lPolygonCount; lPolygonIndex++) {
			int lMatId = -1;
			KFbxLayerElement::EMappingMode lMappingMode = KFbxLayerElement::eNONE;
			KFbxLayerElementArrayTemplate<KFbxVector2>* lUVArray = NULL;
			pMesh->GetTextureUV(&lUVArray, KFbxLayerElement::eDIFFUSE_TEXTURES);
			
			for (int l = 0; l < pMesh->GetLayerCount(); l++) {
				KFbxLayerElementMaterial* lLayerMaterial = pMesh->GetLayer(l)->GetMaterials();
				if (lLayerMaterial) {
					
					KFbxSurfaceMaterial* lMaterial = NULL;
					pNode->GetMaterial(lLayerMaterial->GetIndexArray().GetAt(l));
					lMatId = lLayerMaterial->GetIndexArray().GetAt(lPolygonIndex);
				}
			}
			
			if (lMatId >=0) {
				
				if(pMesh->GetLayer(0) && pMesh->GetLayer(0)->GetUVs())
					lMappingMode = pMesh->GetLayer(0)->GetUVs()->GetMappingMode();
				
				if (mMaterials[lMatId].isTextured) {
					mMaterials[lMatId].tex.bind();
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				}
				
				float r = mMaterials[lMatId].colour.x;
				float g = mMaterials[lMatId].colour.y;
				float b = mMaterials[lMatId].colour.z;
 				
				glColor3f(r,g,b);
//				GLfloat mat[]= {r, g, b};

				// This step loses us around 10fps!!
//				glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,mMaterials[lMatId].glMat);
				
			}
			
			int lVerticeIndex;
			int lVerticeCount = pMesh->GetPolygonSize(lPolygonIndex);
			
			
			glBegin(GL_TRIANGLES);
			
			for (lVerticeIndex = 0; lVerticeIndex < lVerticeCount; lVerticeIndex++){
				int lCurrentUVIndex;
				
				if (lMappingMode == KFbxLayerElement::eBY_POLYGON_VERTEX){
					lCurrentUVIndex = pMesh->GetTextureUVIndex(lPolygonIndex, lVerticeIndex);
				}
				else {
					lCurrentUVIndex = pMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
				}
				if(lUVArray) {
					//(U,1-V)
					//flipping UV's as texture is flipped
					glTexCoord2d(lUVArray->GetAt(lCurrentUVIndex)[0], (double)((double)1.0 - lUVArray->GetAt(lCurrentUVIndex)[1]));
				}
				
				if ( /*normalStatus &&*/ lNormals /*&& lNormals->GetCount() == lControlPointsCount*/) {
					//GetPolygonVertexNormal(int pPolyIndex, int pVertexIndex, KFbxVector4 &pNormal)
					KFbxVector4 pNormal;
					pMesh->GetPolygonVertexNormal(lPolygonIndex, lVerticeIndex, pNormal);
					
					//printf("normals!");
					glNormal3d(pNormal[0], pNormal[1] /* * -1.0 */, pNormal[2]);
					//					glNormal3d(lNormals->GetAt(lCurrentUVIndex)[0], lNormals->GetAt(lCurrentUVIndex)[1], lNormals->GetAt(lCurrentUVIndex)[2]);
				}
				
				glVertex3dv((GLdouble *)pVertexArray[pMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex)]);
			}
			
			glEnd();
			
			if(mMaterials[lMatId].isTextured)
				mMaterials[lMatId].tex.unbind();
			
		}
		
		glPopMatrix();
	//	glDisable(GL_TEXTURE_2D);
	//	glDisable(GL_COLOR_MATERIAL);
	}
	
	
	void S9FbxLoader::drawSkeleton(KFbxNode* pNode, KFbxXMatrix& pParentGlobalPosition, KFbxXMatrix& pGlobalPosition) {
		KFbxSkeleton* lSkeleton = (KFbxSkeleton*) pNode->GetNodeAttribute();
		
		// Only draw the skeleton if it's a limb node and if 
		// the parent also has an attribute of type skeleton.
		if (/*lSkeleton->GetSkeletonType() == KFbxSkeleton::eLIMB_NODE && */
			pNode->GetParent() &&
			pNode->GetParent()->GetNodeAttribute() &&
			pNode->GetParent()->GetNodeAttribute()->GetAttributeType() == KFbxNodeAttribute::eSKELETON)
			
		{
			
			glPushMatrix();
			
			KFbxVector4 e;
			KFbxVector4 s;
			
			KFbxXMatrix sm;
			KFbxXMatrix em;
			
			sm.SetIdentity();
			em.SetIdentity();
			
			glColor3f(0.0, 1.0, 0.0); // TODO - Again bad really 
			
			int idx = getCustomRotation(pNode->GetParent());
			if (idx != -1){
				sm  = mBones[idx].matrix;
				if ( mBones[idx].targeted)
					glColor3f(1.0, 0.0, 0.0);
			}
			
			
			idx = getCustomRotation(pNode);
			if (idx != -1){
				em  = mBones[idx].matrix;
			}
			
			e = em.GetT();
			s = sm.GetT();
			
			KFbxVector4 d = e;
			d = d - s;
			
			glTranslated(s[0],s[1],s[2]);
			glLineWidth(5.0);
			glBegin(GL_LINES);
			glVertex3d(0,0,0);
			if (mBones[idx].targeted) {
				glColor3f(1.0, 0.0, 0.0);
			}
			glVertex3d(d[0],d[1],d[2] );
			
			glEnd();			
			
			glPopMatrix();
			
		}
		
	}
	
	
	// ********************************************************************************
	// Cluster Deformation
	// ********************************************************************************
	
#pragma mark Cluster Deformation
	
	void S9FbxLoader::rotateBone(int boneid, ci::Matrix44f &mat) {
		if (boneid > -1 && boneid < mBones.size()){
			
			if (mat != mBones[boneid].cindermatrix){
				
				for (vector <FbxRotation>::iterator it = mBones.begin(); it != mBones.end(); it ++){
					it->targeted = false;
				}
				
				ci::Matrix44f tmp(mat);
				
				mat =  mBones[boneid].cindermatrix.inverted() * mat;// take the difference. We want ABSOLUTE matrix values passed in
				
				mBones[boneid].cindermatrix = tmp;
				
				mBones[boneid].rmatrix = toFBXMatrix(mat);
				
				mBones[boneid].targeted = true;
				
				// Recursively add the rotations up
				
				KFbxNode *pNode = mBones[boneid].node;
				KFbxNode *pParent = pNode->GetParent();
				
				int pid = getCustomRotation(pParent);
				
				KFbxXMatrix pm = mBones[pid].matrix;
				
				mBones[boneid].matrix *= mBones[boneid].rmatrix;
				//mBones[boneid].matrix = pm * mBones[boneid].rmatrix * pm.Inverse() * mBones[boneid].matrix;
				
				int i, lCount = pNode->GetChildCount();
				
				for (i = 0; i < lCount; i++){
					rotateBoneRecursive(pNode->GetChild(i), mBones[boneid].matrix, mBones[boneid].rmatrix);
				}
			}
		}
	}
	
	void S9FbxLoader::rotateBoneRecursive(KFbxNode *pNode, KFbxXMatrix mat, KFbxXMatrix rmat) {
		
		int idx = getCustomRotation(pNode);
		if (idx != -1){
			mBones[idx].targeted = true;
			KFbxXMatrix cp = pNode->GetScene()->GetEvaluator()->GetNodeGlobalTransform(pNode, mCurrentTime);
			
			
			//mBones[idx].matrix =  mat * rmat * mat.Inverse() * cp;
			mBones[idx].matrix =  mat * rmat * mat.Inverse() * mBones[idx].matrix;
			
			int i, lCount = pNode->GetChildCount();
			
			for (i = 0; i < lCount; i++){
				
				rotateBoneRecursive(pNode->GetChild(i), mat, rmat);
			}
		}
	}
	
	
	void S9FbxLoader::resetRotations() {
		
		for (vector <FbxRotation>::iterator it = mBones.begin(); it != mBones.end(); it ++){
			it->matrix = it->node->GetScene()->GetEvaluator()->GetNodeGlobalTransform(it->node, mCurrentTime);
			it->rmatrix.SetIdentity();
		}
	}
	
	void S9FbxLoader::setupBones() {
		
		int lSkinCount = mMesh->GetDeformerCount(KFbxDeformer::eSKIN);
		for ( int i=0; i<lSkinCount; ++i) {
			
			int lClusterCount =( (KFbxSkin *)mMesh->GetDeformer(i, KFbxDeformer::eSKIN))->GetClusterCount();
			
			for (int j=0; j<lClusterCount; ++j){
				KFbxCluster* lCluster =((KFbxSkin *) mMesh->GetDeformer(i, KFbxDeformer::eSKIN))->GetCluster(j);
				if (!lCluster->GetLink()) // this is actual bone - GetLink
					continue;
				else {
					
					FbxRotation b;
					b.targeted = false;
					b.node = lCluster->GetLink();
					b.matrix = b.node->GetScene()->GetEvaluator()->GetNodeGlobalTransform(b.node, mCurrentTime);
					mBones.push_back(b);
				}			
			}
		}	
	}
	
	
	void S9FbxLoader::computeClusterDeformation(KFbxXMatrix& pGlobalPosition,  KFbxMesh* pMesh,  KFbxVector4* pVertexArray) {
		
		KFbxCluster::ELinkMode lClusterMode = ((KFbxSkin*)pMesh->GetDeformer(0, KFbxDeformer::eSKIN))->GetCluster(0)->GetLinkMode();
		
		int i, j;
		int lClusterCount=0;
		
		int lVertexCount = pMesh->GetControlPointsCount();
		int lSkinCount = pMesh->GetDeformerCount(KFbxDeformer::eSKIN);
		
		KFbxXMatrix* lClusterDeformation = new KFbxXMatrix[lVertexCount];
		memset(lClusterDeformation, 0, lVertexCount * sizeof(KFbxXMatrix));
		double* lClusterWeight = new double[lVertexCount];
		memset(lClusterWeight, 0, lVertexCount * sizeof(double));
		
		if (lClusterMode == KFbxCluster::eADDITIVE) {
			for (i = 0; i < lVertexCount; i++){
				lClusterDeformation[i].SetIdentity();
			}
		}
		
		for ( i=0; i<lSkinCount; ++i) {
			
			
			lClusterCount =( (KFbxSkin *)pMesh->GetDeformer(i, KFbxDeformer::eSKIN))->GetClusterCount();
			
			for (j=0; j<lClusterCount; ++j){
				KFbxCluster* lCluster =((KFbxSkin *) pMesh->GetDeformer(i, KFbxDeformer::eSKIN))->GetCluster(j);
				if (!lCluster->GetLink()) // this is actual bone - GetLink
					continue;
				
				KFbxXMatrix lReferenceGlobalInitPosition;
				KFbxXMatrix lReferenceGlobalCurrentPosition;
				KFbxXMatrix lClusterGlobalInitPosition;
				KFbxXMatrix lClusterGlobalCurrentPosition;
				KFbxXMatrix lReferenceGeometry;
				KFbxXMatrix lClusterGeometry;
				
				KFbxXMatrix lClusterRelativeInitPosition;
				KFbxXMatrix lClusterRelativeCurrentPositionInverse;
				KFbxXMatrix lVertexTransformMatrix;
				
				if (lClusterMode == KFbxLink::eADDITIVE && lCluster->GetAssociateModel()) {
					lCluster->GetTransformAssociateModelMatrix(lReferenceGlobalInitPosition);
					lReferenceGlobalCurrentPosition = GetGlobalPosition(lCluster->GetAssociateModel(), mCurrentTime);
					lReferenceGeometry = GetGeometry(lCluster->GetAssociateModel());
					lReferenceGlobalCurrentPosition *= lReferenceGeometry;
				}
				else {
					lCluster->GetTransformMatrix(lReferenceGlobalInitPosition);
					lReferenceGlobalCurrentPosition = pGlobalPosition;
					lReferenceGeometry = GetGeometry(pMesh->GetNode());				
					lReferenceGlobalInitPosition *= lReferenceGeometry;
				}
				
				
				lCluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);
				
				lClusterGlobalCurrentPosition = GetGlobalPosition(lCluster->GetLink(), mCurrentTime);
				
				
				int idx = getCustomRotation(lCluster->GetLink());
				if (idx != -1){
					lClusterGlobalCurrentPosition  = mBones[idx].matrix;
				}
				
				lClusterRelativeInitPosition = lClusterGlobalInitPosition.Inverse() * lReferenceGlobalInitPosition;
				
				lClusterRelativeCurrentPositionInverse = lReferenceGlobalCurrentPosition.Inverse() * lClusterGlobalCurrentPosition;
				
				
				lVertexTransformMatrix = lClusterRelativeCurrentPositionInverse * lClusterRelativeInitPosition;
				
				
				int k;
				int lVertexIndexCount = lCluster->GetControlPointIndicesCount();
				
				for (k = 0; k < lVertexIndexCount; ++k) 
				{            
					int lIndex = lCluster->GetControlPointIndices()[k];
					if (lIndex >= lVertexCount)
						continue;
					
					double lWeight = lCluster->GetControlPointWeights()[k];
					
					if (lWeight == 0.0){
						continue;
					}	
					
					// Compute the influence of the link on the vertex.
					KFbxXMatrix lInfluence = lVertexTransformMatrix;
					MatrixScale(lInfluence, lWeight);
					
					if (lClusterMode == KFbxCluster::eADDITIVE)
					{   
						// Multiply with to the product of the deformations on the vertex.
						MatrixAddToDiagonal(lInfluence, 1.0 - lWeight);
						lClusterDeformation[lIndex] = lInfluence * lClusterDeformation[lIndex];
						
						// Set the link to 1.0 just to know this vertex is influenced by a link.
						lClusterWeight[lIndex] = 1.0;
					}
					else // lLinkMode == KFbxLink::eNORMALIZE || lLinkMode == KFbxLink::eTOTAL1
					{
						// Add to the sum of the deformations on the vertex.
						MatrixAdd(lClusterDeformation[lIndex], lInfluence);
						
						// Add to the sum of weights to either normalize or complete the vertex.
						lClusterWeight[lIndex] += lWeight;
					}
				}
			}
		}
		
		for (i = 0; i < lVertexCount; i++) 
		{
			KFbxVector4 lSrcVertex = pVertexArray[i];
			KFbxVector4& lDstVertex = pVertexArray[i];
			double lWeight = lClusterWeight[i];
			
			// Deform the vertex if there was at least a link with an influence on the vertex,
			if (lWeight != 0.0) 
			{
				lDstVertex = lClusterDeformation[i].MultT(lSrcVertex);
				
				if (lClusterMode == KFbxCluster::eNORMALIZE) {
					// In the normalized link mode, a vertex is always totally influenced by the links. 
					lDstVertex /= lWeight;
				}	
				else if (lClusterMode == KFbxCluster::eTOTAL1){
					// In the total 1 link mode, a vertex can be partially influenced by the links. 
					lSrcVertex *= (1.0 - lWeight);
					lDstVertex += lSrcVertex;
				}
			} 
		}
		
		delete [] lClusterDeformation;
		delete [] lClusterWeight;
	}
	
	
	
	
	// ********************************************************************************
	// MISC Methods
	// ********************************************************************************
	
#pragma mark misc function
	
	
	void S9FbxLoader::preparePointCacheData(KFbxScene* pScene)
	{
		// This function show how to cycle thru scene elements in a linear way.
		int lIndex, lNodeCount = KFbxGetSrcCount<KFbxNode>(pScene);
		
		for (lIndex=0; lIndex<lNodeCount; lIndex++) {
			KFbxNode* lNode = KFbxGetSrc<KFbxNode>(pScene, lIndex);
			
			if (lNode->GetGeometry()) 
			{
				int i, lVertexCacheDeformerCount = lNode->GetGeometry()->GetDeformerCount(KFbxDeformer::eVERTEX_CACHE);
				
				// There should be a maximum of 1 Vertex Cache Deformer for the moment
				lVertexCacheDeformerCount = lVertexCacheDeformerCount > 0 ? 1 : 0;
				
				for (i=0; i<lVertexCacheDeformerCount; ++i ) {
					// Get the Point Cache object
					KFbxVertexCacheDeformer* lDeformer = static_cast<KFbxVertexCacheDeformer*>(lNode->GetGeometry()->GetDeformer(i, KFbxDeformer::eVERTEX_CACHE));
					if( !lDeformer ) continue;
					KFbxCache* lCache = lDeformer->GetCache();
					if( !lCache ) continue;
					
					// Process the point cache data only if the constraint is active
					if (lDeformer->IsActive()) {
						if (lCache->GetCacheFileFormat() == KFbxCache::ePC2) {
							// This code show how to convert from PC2 to MC point cache format
							// turn it on if you need it.
#if 0 
							if (!lCache->ConvertFromPC2ToMC(KFbxCache::eMC_ONE_FILE, 
															KTime::GetFrameRate(pScene->GetGlobalTimeSettings().GetTimeMode()))) {
								
								KString lTheErrorIs = lCache->GetError().GetLastErrorString();
							}
#endif
						}
						else if (lCache->GetCacheFileFormat() == KFbxCache::eMC)
						{
							// This code show how to convert from MC to PC2 point cache format
							// turn it on if you need it.
							//#if 0 
							if (!lCache->ConvertFromMCToPC2(KTime::GetFrameRate(pScene->GetGlobalSettings().GetTimeMode()), 0))
							{
								// Conversion failed, retrieve the error here
								KString lTheErrorIs = lCache->GetError().GetLastErrorString();
							}
							//#endif
						}
						
						
						// Now open the cache file to read from it
						if (!lCache->OpenFileForRead())
						{
							// Cannot open file 
							KString lTheErrorIs = lCache->GetError().GetLastErrorString();
							
							// Set the deformer inactive so we don't play it back
							lDeformer->SetActive(false);
						}
					}
				}
			}
		}
	}
	
	
	void S9FbxLoader::readVertexCacheData(KFbxMesh* pMesh, KFbxVector4* pVertexArray)
	{
		KFbxVertexCacheDeformer* lDeformer     = static_cast<KFbxVertexCacheDeformer*>(pMesh->GetDeformer(0, KFbxDeformer::eVERTEX_CACHE));
		KFbxCache*               lCache        = lDeformer->GetCache();
		int                      lChannelIndex = -1;
		unsigned int             lVertexCount  = (unsigned int)pMesh->GetControlPointsCount();
		bool                     lReadSucceed  = false;
		double*                  lReadBuf      = new double[3*lVertexCount];
		
		if (lCache->GetCacheFileFormat() == KFbxCache::eMC)
		{
			if ((lChannelIndex = lCache->GetChannelIndex(lDeformer->GetCacheChannel())) > -1)
			{
				lReadSucceed = lCache->Read(lChannelIndex, mCurrentTime, lReadBuf, lVertexCount);
			}
		}
		else // ePC2
		{
			lReadSucceed = lCache->Read((unsigned int)mCurrentTime.GetFrame(true), lReadBuf, lVertexCount);
		}
		
		if (lReadSucceed)
		{
			unsigned int lReadBufIndex = 0;
			
			while (lReadBufIndex < 3*lVertexCount)
			{
				// In statements like "pVertexArray[lReadBufIndex/3].SetAt(2, lReadBuf[lReadBufIndex++])", 
				// on Mac platform, "lReadBufIndex++" is evaluated before "lReadBufIndex/3". 
				// So separate them.
				pVertexArray[lReadBufIndex/3].SetAt(0, lReadBuf[lReadBufIndex]); lReadBufIndex++;
				pVertexArray[lReadBufIndex/3].SetAt(1, lReadBuf[lReadBufIndex]); lReadBufIndex++;
				pVertexArray[lReadBufIndex/3].SetAt(2, lReadBuf[lReadBufIndex]); lReadBufIndex++;
			}
		}
		
		delete [] lReadBuf;
	}
	
	
	// Essentially hunts through recursively for things we might need
	
	void S9FbxLoader::setupInternalRefs(KFbxNode* pNode){
		
		KFbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();
		
		if (lNodeAttribute){
			if (lNodeAttribute->GetAttributeType() == KFbxNodeAttribute::eMESH){
				mMesh = (KFbxMesh*) pNode->GetNodeAttribute();
			}
		}
		
		
		int i, lCount = pNode->GetChildCount();
		
		for (i = 0; i < lCount; i++){
			setupInternalRefs(pNode->GetChild(i));
		}
	}
	
	int	S9FbxLoader::getCustomRotation(KFbxNode *pNode) {
		int index = 0;
		for(vector<FbxRotation>::iterator i = mBones.begin(); i != mBones.end(); i ++) {
			if (i->node == pNode)
				return index;
			index ++;
		}
		return -1;
	}
	
	
	FbxRotation	S9FbxLoader::getCustomRotation(int idx) {
		return mBones[idx];
	}
	
	
	
	// TODO - static_cast<T>(x) instead?
	
	ci::Matrix44f S9FbxLoader::toCinderMatrix(KFbxXMatrix m) {
		
		ci::Matrix44f r;
		
		double * dst = (double*) &r;
		float * src = (float*)&m;
		
		for(int i = 0; i < 16; i++){
			*dst = (float) *src;
			dst++;
			src++;
		}
		
		return r;
	}
	
	KFbxXMatrix S9FbxLoader::toFBXMatrix(ci::Matrix44f m) {
		
		KFbxXMatrix r;
		double * dst = (double*)&r;
		float * src = (float*)&m;
		
		for(int i = 0; i < 16; i++){
			*dst = (double) *src;
			dst++;
			src++;
		}
		return r;
	}
	
	ci::Matrix44f S9FbxLoader::getCinderMatrix(int boneid) {
		if (boneid >-1 && boneid < mBones.size()){
			return mBones[boneid].cindermatrix;
		}
		return ci::Matrix44f();
	}
	
	Vec3f S9FbxLoader::getBoneVector(int boneid) {
		if (boneid >-1 && boneid < mBones.size()){
			
			KFbxVector4 e;
			KFbxVector4 s;
			
			KFbxXMatrix sm;
			KFbxXMatrix em;
			
			sm.SetIdentity();
			em.SetIdentity();
			
			em  = mBones[boneid].matrix;
			
			int idx = getCustomRotation(mBones[boneid].node->GetParent());
			if (idx != -1){
				sm  = mBones[idx].matrix;
			}
			
			e = em.GetT();
			s = sm.GetT();
			
			KFbxVector4 d = e;
			d = d - s;
			
			Vec3f rd (d[0],d[1],d[2]);
			rd.normalize();
			return rd;
			
		}
		return Vec3f::zero();
	}
	
	
	
	// ********************************************************************************
	// Texture Methods
	// ********************************************************************************
	
#pragma mark Texture and Material Methods
	
	void S9FbxLoader::loadSupportedMaterials() {
		mTextureArray.Clear();
		loadSupportedMaterialsRecursive(mScene->GetRootNode());
	}
	
	void S9FbxLoader::loadSupportedMaterialsRecursive(KFbxNode* pNode){
		if (pNode) {
			int i, lCount;
			KFbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();
			
			if (lNodeAttribute)
			{
				KFbxLayerContainer* lLayerContainer = NULL;
				
				switch (lNodeAttribute->GetAttributeType())
				{
					case KFbxNodeAttribute::eNURB:
						lLayerContainer = pNode->GetNurb();
						break;
						
					case KFbxNodeAttribute::ePATCH:
						lLayerContainer = pNode->GetPatch();
						break;
						
					case KFbxNodeAttribute::eMESH:
						lLayerContainer = pNode->GetMesh();
						break;
				}
				
				if (lLayerContainer){
					int lMaterialIndex;
					int lTextureIndex;
					KFbxProperty lProperty;
					int lNbTex;
					KFbxTexture* lTexture = NULL; 
					KFbxSurfaceMaterial *lMaterial = NULL;
					int lNbMat = pNode->GetSrcObjectCount(KFbxSurfaceMaterial::ClassId);
					for (lMaterialIndex = 0; lMaterialIndex < lNbMat; lMaterialIndex++){
						lMaterial = KFbxCast <KFbxSurfaceMaterial>(pNode->GetSrcObject(KFbxSurfaceMaterial::ClassId, lMaterialIndex));
						if(lMaterial){
							
							// Texture
							FbxMaterial material;
							material.isTextured = false;
							
							lProperty = lMaterial->FindProperty(KFbxSurfaceMaterial::sDiffuse);
							if(lProperty.IsValid()){
								lNbTex = lProperty.GetSrcObjectCount(KFbxTexture::ClassId);
								for (lTextureIndex = 0; lTextureIndex < lNbTex; lTextureIndex++){
									lTexture = KFbxCast <KFbxTexture> (lProperty.GetSrcObject(KFbxTexture::ClassId, lTextureIndex)); 
									if(lTexture){
										material.tex = loadTexture(lTexture);
										material.isTextured = true;
									}
								}
							}
							
							// Diffuse Colour
							
							if (lMaterial->GetClassId().Is(KFbxSurfaceLambert::ClassId)) {
								KFbxPropertyDouble3 lKFbxDouble3 =((KFbxSurfaceLambert *)lMaterial)->GetDiffuseColor();
								material.colour = Vec3f(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
							}
							else if (lMaterial->GetClassId().Is(KFbxSurfacePhong::ClassId)){
								KFbxPropertyDouble3 lKFbxDouble3 =((KFbxSurfacePhong *) lMaterial)->GetDiffuseColor();
								material.colour = Vec3f(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
							}
							
							mMaterials.push_back(material);	// Hopefully this index matches
							
						}
					}
				} 
			}
			
			lCount = pNode->GetChildCount();
			
			for (i = 0; i < lCount; i++){
				loadSupportedMaterialsRecursive(pNode->GetChild(i));
			}
		}
	}
	
	gl::Texture S9FbxLoader::loadTexture(KFbxTexture* pTexture) {
		std::string textureFileName = std::string(pTexture->GetFileName());
		
		int lCount = mTextureArray.GetCount();
		
		map<std::string,gl::Texture>::iterator it;
		it=mMapToTex.find(textureFileName);
		if (it != mMapToTex.end())
			return (*it).second;
		
		// TODO - Fail gracefully (try catch?) if texture not found and disallow texture rendering
		
		app::console() << "Loading texture: " << mPath << "/" << textureFileName << endl;
		gl::Texture texture = loadImage(mPath + "/" + textureFileName); 
		mTextureArray.Add(texture);
		mMapToTex[textureFileName] = texture;
		return texture;
	}
	
#pragma mark Collision Detection
	

	int S9FbxLoader::collisionRecursive(KFbxNode* pNode, KFbxXMatrix& pParentGlobalPosition, int lvl, Vec3f p, float r) {
		int c = 0;
		
		KFbxXMatrix lGlobalPosition = GetGlobalPosition(pNode, mCurrentTime, &pParentGlobalPosition);
		KFbxXMatrix lGeometryOffset = GetGeometry(pNode);
		KFbxXMatrix lGlobalOffPosition = lGlobalPosition * lGeometryOffset;	
		
		KFbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();
		
		if (lNodeAttribute) {
			
			if (lNodeAttribute->GetAttributeType() == KFbxNodeAttribute::eMESH) {
				c += meshCollision(pNode, lGlobalOffPosition,p,r);
			}
		}
		
		int i, lCount = pNode->GetChildCount();
	
		for (i = 0; i < lCount; i++){
			c += collisionRecursive(pNode->GetChild(i), lGlobalPosition,lvl+1,p,r);
		}
		
		return c;
	}
	
	
	int S9FbxLoader::meshCollision(KFbxNode* pNode, KFbxXMatrix& pGlobalPosition, Vec3f p, float r) {
		KFbxMesh* lMesh = (KFbxMesh*) pNode->GetNodeAttribute();
		
		int lClusterCount = 0;
		int lSkinCount= 0;
		int lVertexCount = lMesh->GetControlPointsCount();
		
		int numcollisions = 0;
		
		// No vertex to draw.
		if (lVertexCount == 0)
			return 0;
		
		// Create a copy of the vertex array to receive vertex deformations.
		KFbxVector4* lVertexArray = new KFbxVector4[lVertexCount];
		memcpy(lVertexArray, lMesh->GetControlPoints(), lVertexCount * sizeof(KFbxVector4));
		
		// Active vertex cache deformer will overwrite any other deformer
		if (lMesh->GetDeformerCount(KFbxDeformer::eVERTEX_CACHE) &&
			(static_cast<KFbxVertexCacheDeformer*>(lMesh->GetDeformer(0, KFbxDeformer::eVERTEX_CACHE)))->IsActive())
		{
			readVertexCacheData(lMesh, lVertexArray);
		}
		else
		{
			
			lSkinCount = lMesh->GetDeformerCount(KFbxDeformer::eSKIN);
			for( int i=0; i< lSkinCount; i++)
				lClusterCount += ((KFbxSkin *)(lMesh->GetDeformer(i, KFbxDeformer::eSKIN)))->GetClusterCount();
			
			if (lClusterCount){
				computeClusterDeformation(pGlobalPosition, lMesh, lVertexArray);
			}
		}
		
		
		int lPolygonIndex;
		int lPolygonCount = lMesh->GetPolygonCount();
		
		
		for (lPolygonIndex = 0; lPolygonIndex < lPolygonCount; lPolygonIndex++) {
			
			int lVerticeIndex;
			int lVerticeCount = lMesh->GetPolygonSize(lPolygonIndex);
			
			for (lVerticeIndex = 0; lVerticeIndex < lVerticeCount; lVerticeIndex++){
				
				KFbxVector4 pv = lVertexArray[lMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex)];
				
				// How bloody annoying!
				
				Matrix44f pm(pGlobalPosition.Get(0,0),pGlobalPosition.Get(0,1),pGlobalPosition.Get(0,2), pGlobalPosition.Get(0,3), 
							 pGlobalPosition.Get(1,0), pGlobalPosition.Get(1,1), pGlobalPosition.Get(1,2), pGlobalPosition.Get(1,3), 
							pGlobalPosition.Get(2,0), pGlobalPosition.Get(2,1), pGlobalPosition.Get(2,2), pGlobalPosition.Get(2,3), 
							 pGlobalPosition.Get(3,0), pGlobalPosition.Get(3,1), pGlobalPosition.Get(3,2), pGlobalPosition.Get(3,3)); 
				
				p = pm.inverted() * p;
				
				Vec3f t ((float)pv[0],(float)pv[1],(float)pv[2]);
				float dd = t.distanceSquared(p);
				if (dd <= r * r){
					numcollisions++;
				}
				
				
			}
		}
		delete [] lVertexArray;
		
		return numcollisions;
		
	}
	
	
	bool S9FbxLoader::bruteForceCollision(Vec3f p, float r){
		
		KFbxXMatrix lDummyGlobalPosition;
		
		int i, lCount = mScene->GetRootNode()->GetChildCount();
		int c = 0;
		
		for (i = 0; i < lCount; i++){
			c = collisionRecursive(mScene->GetRootNode()->GetChild(i), lDummyGlobalPosition,1,p,r);
		}
		return c != 0;
		
	}
	
	
} // End Namespace S9
