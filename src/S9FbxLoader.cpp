/*
                       __  .__              ________ 
   ______ ____   _____/  |_|__| ____   ____/   __   \
  /  ___// __ \_/ ___\   __\  |/  _ \ /    \____    /
  \___ \\  ___/\  \___|  | |  (  <_> )   |  \ /    / 
 /____  >\___  >\___  >__| |__|\____/|___|  //____/  .co.uk
      \/     \/     \/                    \/         
 
 THE GHOST IN THE CSH
 
 
 S9FbxLoader.cpp | Part of S9FBX | Created 03/02/2011
 
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

using namespace std;

namespace S9{
	
	
#pragma mark setup and destroy functions
	
	
	FbxDrawable::~FbxDrawable() {
		
		meshes.clear();
		materials.clear();
		mMapToTex.clear();
		
	}
	
	
	
	S9FbxLoader::S9FbxLoader() {
		mCurrentTime = 0; // TODO - should be set to start when animation is done
		mApplyMatrices = true;
	}
	
	S9FbxLoader::~S9FbxLoader() {
	}
	
	shared_ptr<FbxDrawable> S9FbxLoader::load(string fileName){
		
		
		shared_ptr<FbxDrawable> drawable(new FbxDrawable());
		
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
				app::console() << "S9FBXLoader - Importing FBX file " << fileName << endl;
				
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
					loadSupportedMaterials(drawable);
					setupForCinder(drawable);
					
					// TODO - animation and poses from the previous example need to be here
					
				}
				else{
					app::console() << "S9FBXLoader - Unable to import file " << endl;
				}
				
			}
			else{
				app::console() << "S9FBXLoader - Unable to open file " << mImporter->GetLastErrorString();
			}
		}
		else{
			app::console() << "S9FBXLoader - Unable to create the FBX SDK manager"  << endl;
		}
		
		// When mPoseIndex is not -1, draw the scene at that pose
		// TODO - Multiple pose support
		mImporter->Destroy();
		
		return drawable;
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
	
	
#pragma mark Cinder Structure Creation Methods
	
	// ********************************************************************************
	// Cinder Structure Methods
	// ********************************************************************************	
	
	// TODO - Basically, being recursive is probably not the best thing to do. Im thinking
	// create what we need in memory and ditch the rest. We need to get the vertices and 
	// what not out in arrays anyway so we can use VBOs and shaders for skinning and for
	// bullet physics
	
	void S9FbxLoader::setupForCinder(shared_ptr<FbxDrawable>  pDrawable) {
		KFbxXMatrix lDummyGlobalPosition;
		
		int i, lCount = mScene->GetRootNode()->GetChildCount();
		
		for (i = 0; i < lCount; i++){
			setupForCinderRecursive(pDrawable, mScene->GetRootNode()->GetChild(i), lDummyGlobalPosition);
		}
	}
	
	
	void S9FbxLoader::setupForCinderRecursive(shared_ptr<FbxDrawable>  pDrawable, KFbxNode* pNode, KFbxXMatrix& pParentGlobalPosition) {
		
		KFbxXMatrix globalPosition = GetGlobalPosition(pNode, mCurrentTime, &pParentGlobalPosition);
		KFbxXMatrix geometryOffset = GetGeometry(pNode);
		KFbxXMatrix globalOffPosition = globalPosition * geometryOffset;	
		
		// Pull out the Mesh and create some triangles
		KFbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();
		
		if (lNodeAttribute) {
			
			if (lNodeAttribute->GetAttributeType() == KFbxNodeAttribute::eMESH) {
				
				// Create a new Cinder Mesh
				shared_ptr<FbxMesh> pMesh(new FbxMesh());
				pDrawable->meshes.push_back(pMesh);
				pMesh->applyMatrices = isApplyMatrices();
				
				pMesh->mMin.x = pMesh->mMin.y = pMesh->mMin.z = 10000000.0;
				pMesh->mMax.x = pMesh->mMax.y = pMesh->mMax.z = -10000000.0;
				
				setupForCinderMesh(pDrawable, pNode,globalPosition, pMesh);
				
				int lSkinCount = ((KFbxMesh*)lNodeAttribute)->GetDeformerCount(KFbxDeformer::eSKIN);
				if (lSkinCount > 0) {
					setupForCinderDeformations(pDrawable, globalPosition,(KFbxMesh*)lNodeAttribute,pMesh);
					pMesh->mDeform = true;
				}
				else {
					pMesh->mDeform = false;
				}
				
				
			} else if (lNodeAttribute->GetAttributeType() == KFbxNodeAttribute::eSKELETON){
				setupForCinderSkeleton(pDrawable, pNode,globalPosition);
			}
		}
		
		
		int i, lCount = pNode->GetChildCount();
		for (i = 0; i < lCount; i++){
			setupForCinderRecursive(pDrawable, pNode->GetChild(i), globalPosition);
		}
	}
	
	
	void S9FbxLoader::setupForCinderMesh(shared_ptr<FbxDrawable>  pDrawable, KFbxNode* pNode, KFbxXMatrix& pGlobalPosition, shared_ptr<FbxMesh> pMesh){
		
		// Setup a Mesh - ASSUMING TRIANGLES ONLY! (for now!)
		
		KFbxMesh* lMesh = (KFbxMesh*) pNode->GetNodeAttribute();
		pMesh->offset = toCinderMatrix(pGlobalPosition); 
		
		int vertexCount = lMesh->GetControlPointsCount();
		
		// No vertex to draw.
		if (vertexCount != 0) {
			
			int polygonCount = lMesh->GetPolygonCount();
			int indiciesCount = lMesh->GetPolygonVertexCount();
			
			// Setup the Mesh Containers - Populate so we can assign
			
			pMesh->indicies.reserve(indiciesCount);
			pMesh->vertices.reserve(vertexCount);
			pMesh->texcoords.reserve(vertexCount);
			pMesh->normals.reserve(vertexCount);
			
			pMesh->numtris = polygonCount;
			pMesh->numverts = vertexCount;
			
			// Indicies
			int* pindices = lMesh->GetPolygonVertices();
			for (int i =0; i < indiciesCount; ++i){
				pMesh->indicies.push_back(pindices[i]);
			}
			
			
			// Vertices (and a copy of skinned vertices and matrices as well)
			
			bool sv = lMesh->GetDeformerCount(KFbxDeformer::eSKIN) > 0;
			if (sv) {
				pMesh->skinvertices.reserve(vertexCount);
				pMesh->skinmatrices.reserve(vertexCount);
				pMesh->skinnormals.reserve(vertexCount);
			}
			
			KFbxVector4* pvertices = lMesh->GetControlPoints();
			
			pMesh->floats = new float[vertexCount * 3];
			
			for (int i =0; i < vertexCount; ++i){
				
				Vec3d vert = Vec3d(pvertices[i][0],pvertices[i][1],pvertices[i][2]);
				
				// Set min and max values
				if ( vert.x < pMesh->mMin.x) pMesh->mMin.x = vert.x;
				if ( vert.y < pMesh->mMin.y) pMesh->mMin.y = vert.y;
				if ( vert.z < pMesh->mMin.z) pMesh->mMin.z = vert.z;
				
				if ( vert.x > pMesh->mMax.x) pMesh->mMax.x = vert.x;
				if ( vert.y > pMesh->mMax.y) pMesh->mMax.y = vert.y;
				if ( vert.z > pMesh->mMax.z) pMesh->mMax.z = vert.z;
				
				
				if (mApplyMatrices)
					vert = pMesh->offset * vert;
				
				pMesh->vertices.push_back( vert ) ;
				
				
				
				if (sv) {
					pMesh->skinvertices.push_back( vert ) ;
					Matrix44d m(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0); // If we arent using additive or total in FBX Matrices
					pMesh->skinmatrices.push_back(m);
					pMesh->skinweights.push_back(0.0);
				}
			}
			
			
			// Normals
			
			KFbxLayerElementArrayTemplate<KFbxVector4> *lNormals = NULL;
			bool normalStatus = lMesh->GetNormals(&lNormals);
			if (normalStatus) {
				for (int i =0; i < vertexCount; ++i){
					
					Vec3d norm (lNormals->GetAt(i)[0],lNormals->GetAt(i)[1],lNormals->GetAt(i)[2]);
					
					if (mApplyMatrices)
						norm = pMesh->offset * norm;
					
					pMesh->normals.push_back( norm ) ;
					if (sv) {
						pMesh->skinnormals.push_back(norm);
					}
				}
			}
			
			
			// Textures
			
			KFbxLayerElement::EMappingMode lMappingMode = KFbxLayerElement::eNONE;
			KFbxLayerElementArrayTemplate<KFbxVector2>* lUVArray = NULL;
			lMesh->GetTextureUV(&lUVArray, KFbxLayerElement::eDIFFUSE_TEXTURES);
			
			for (int i =0; i < vertexCount; ++i){
				pMesh->texcoords.push_back( Vec2f(0,0) ) ;
			}
			
			
			for (int i = 0; i < polygonCount; ++i) {
				
				// Materials - Per Triangle
				
				for (int l = 0; l < lMesh->GetLayerCount(); l++) {
					KFbxLayerElementMaterial* lLayerMaterial = lMesh->GetLayer(l)->GetMaterials();
					if (lLayerMaterial) {
						pNode->GetMaterial(lLayerMaterial->GetIndexArray().GetAt(l));
						pMesh->matindicies.push_back(lLayerMaterial->GetIndexArray().GetAt(i));
					}
				}
				
				for (int j = 0; j < 3; j++){
					
					int lCurrentUVIndex;
					
					if(lMesh->GetLayer(0) && lMesh->GetLayer(0)->GetUVs())
						lMappingMode = lMesh->GetLayer(0)->GetUVs()->GetMappingMode();
					
					if (lMappingMode == KFbxLayerElement::eBY_POLYGON_VERTEX)
						lCurrentUVIndex = lMesh->GetTextureUVIndex(i, j);
					else 
						lCurrentUVIndex = lMesh->GetPolygonVertex(i, j);
					
					pMesh->texcoords[ lMesh->GetPolygonVertex(i,j) ].x = (float)lUVArray->GetAt(lCurrentUVIndex)[0];
					pMesh->texcoords[ lMesh->GetPolygonVertex(i,j) ].y = 1.0 - (float)lUVArray->GetAt(lCurrentUVIndex)[1];
					
				}
			}
		}
	}
	
	void S9FbxLoader::setupForCinderSkeleton(shared_ptr<FbxDrawable>  pDrawable, KFbxNode* pNode, KFbxXMatrix& pParentGlobalPosition) {
		// TODO - Not implemented yet but I'm not far off!
		
	}
	
	
	void S9FbxLoader::setupForCinderDeformations(shared_ptr<FbxDrawable> pDrawable, KFbxXMatrix& pGlobalPosition,  KFbxMesh* pMesh, shared_ptr<FbxMesh> pCinderMesh) {
		
		int lSkinCount = pMesh->GetDeformerCount(KFbxDeformer::eSKIN);
		
		if (lSkinCount > 0) {
			
			KFbxCluster::ELinkMode lClusterMode = ((KFbxSkin*)pMesh->GetDeformer(0, KFbxDeformer::eSKIN))->GetCluster(0)->GetLinkMode();
			
			
			map< shared_ptr<FbxRotation>, KFbxNode*> cinderToFBX;
			
			// TODO - Just use the first skin for now!	
			
			int numClusters = ((KFbxSkin *) pMesh->GetDeformer(0, KFbxDeformer::eSKIN))->GetClusterCount();
			
			// Clusters = Matrices and indicies into the Vertex Array
			
			for (int i = 0; i < numClusters; ++i){
				shared_ptr<FbxCluster> cinderCluster(new FbxCluster);
				pCinderMesh->clusters.push_back(cinderCluster);
				
				KFbxCluster* fbxCluster = ((KFbxSkin *) pMesh->GetDeformer(0, KFbxDeformer::eSKIN))->GetCluster(i);
				if (!fbxCluster->GetLink()) // this is actual bone - GetLink
					continue;
				
				cinderCluster->mode = lClusterMode;
				
				// Matrices
				
				KFbxXMatrix lReferenceGlobalInitPosition;
				KFbxXMatrix lReferenceGlobalCurrentPosition;
				KFbxXMatrix lClusterGlobalInitPosition;
				KFbxXMatrix lClusterGlobalCurrentPosition;
				KFbxXMatrix lReferenceGeometry;
				KFbxXMatrix lClusterGeometry;
				KFbxXMatrix lVertexTransformMatrix;
				
				KFbxXMatrix lClusterRelativeInitPosition;
				KFbxXMatrix lClusterRelativeCurrentPositionInverse;				
				
				if (lClusterMode == KFbxLink::eADDITIVE && fbxCluster->GetAssociateModel()) {
					fbxCluster->GetTransformAssociateModelMatrix(lReferenceGlobalInitPosition);
					lReferenceGlobalCurrentPosition = GetGlobalPosition(fbxCluster->GetAssociateModel(), mCurrentTime);
					lReferenceGeometry = GetGeometry(fbxCluster->GetAssociateModel());
					lReferenceGlobalCurrentPosition *= lReferenceGeometry;
					
				}
				else {
					fbxCluster->GetTransformMatrix(lReferenceGlobalInitPosition);
					lReferenceGlobalCurrentPosition = pGlobalPosition;
					lReferenceGeometry = GetGeometry(pMesh->GetNode());				
					lReferenceGlobalInitPosition *= lReferenceGeometry;
				}
				
				fbxCluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);
				lClusterGlobalCurrentPosition = GetGlobalPosition(fbxCluster->GetLink(), mCurrentTime);
				
				// Relate this cluster to a new cinder bone
				
				shared_ptr<FbxRotation> b (new FbxRotation);
				cinderCluster->bone = b;
				pCinderMesh->bones.push_back(b);
				b->targeted = false;
				
				cinderToFBX[b] = fbxCluster->GetLink();
				
				
				// Apply the Matrices
				
				Matrix44d m = toCinderMatrix(lClusterGlobalCurrentPosition);
				b->baseMatrix = m;
				b->realMatrix = shared_ptr<Matrix44d>( new Matrix44d(m) );
				
				
				Matrix44d pr = toCinderMatrix(lReferenceGlobalCurrentPosition.Inverse());
				Matrix44d ps = toCinderMatrix(lClusterGlobalInitPosition.Inverse() * lReferenceGlobalInitPosition);
				cinderCluster->transform = b->realMatrix;
				cinderCluster->pretransform = pr;
				cinderCluster->posttransform = ps;
				
				/*	if (mApplyMatrices) {
				 b->baseMatrix = pCinderMesh->offset * b->baseMatrix * pCinderMesh->offset.inverted();
				 b->realMatrix = pCinderMesh->offset * b->realMatrix * pCinderMesh->offset.inverted();
				 
				 cinderCluster->pretransform = pCinderMesh->offset * cinderCluster->pretransform * pCinderMesh->offset.inverted();
				 cinderCluster->posttransform = pCinderMesh->offset *  cinderCluster->posttransform * pCinderMesh->offset.inverted();
				 }*/
				
				
				// Setup the Indices and weights - it SHOULD be indices and weights for the matrices we currently have
				
				int numIndicies = fbxCluster->GetControlPointIndicesCount();
				
				
				Vec3d centre (0,0,0);
				
				cinderCluster->mMax = Vec3d(-1e10,-1e10,-1e10);
				cinderCluster->mMin = Vec3d(1e10,1e10,1e10);
				
				
				for (int j =0; j < numIndicies; ++j){
					cinderCluster->indicies.push_back(fbxCluster->GetControlPointIndices()[j]);
					cinderCluster->weights.push_back(fbxCluster->GetControlPointWeights()[j]);
					
					Vec3d vert = pCinderMesh->vertices[fbxCluster->GetControlPointIndices()[j]];
					
					cinderCluster->mMax.x = vert.x > cinderCluster->mMax.x ? vert.x : cinderCluster->mMax.x;
					cinderCluster->mMax.y = vert.y > cinderCluster->mMax.y ? vert.y : cinderCluster->mMax.y;
					cinderCluster->mMax.z = vert.z > cinderCluster->mMax.z ? vert.z : cinderCluster->mMax.z;
					
					cinderCluster->mMin.x = vert.x < cinderCluster->mMin.x ? vert.x : cinderCluster->mMin.x;
					cinderCluster->mMin.y = vert.y < cinderCluster->mMin.y ? vert.y : cinderCluster->mMin.y;
					cinderCluster->mMin.z = vert.z < cinderCluster->mMin.z ? vert.z : cinderCluster->mMin.z;
					
					centre += vert;
					
				}
				
				if (centre != Vec3d::zero()){
					cinderCluster->mCentre = centre * 1.0 / static_cast<float>(numIndicies);
				}
				else {
					cinderCluster->mCentre = centre;
				}
				
				
				
			}
			
			// Now, we need to setup the Bone hierarchy so we can rotate and pass down the transforms
			
			for (map< shared_ptr<FbxRotation>, KFbxNode*>::iterator it = cinderToFBX.begin(); it != cinderToFBX.end(); it++){
				shared_ptr<FbxRotation> f = (*it).first;
				KFbxNode* n = (*it).second;
				KFbxNode* p = n->GetParent();
				
				for (vector< shared_ptr<FbxRotation> >::iterator ij = pCinderMesh->bones.begin(); ij != pCinderMesh->bones.end(); ij++) {
					if (cinderToFBX[(*ij)] == p){
						f->parent = *ij;
						break;
					}
				}
			}
		}
		
	}
	
	
	// ********************************************************************************
	// MISC Methods
	// ********************************************************************************
	
#pragma mark misc function
	
	
	ci::Matrix44d S9FbxLoader::toCinderMatrix(KFbxXMatrix m) {
		
		ci::Matrix44d r;
		
		double * dst = (double*) &r;
		double * src = (double*)&m;
		
		for(int i = 0; i < 16; i++){
			*dst = static_cast<double>( *src );
			//*dst = (float) *src;
			dst++;
			src++;
		}
		
		return r;
	}
	
	KFbxXMatrix S9FbxLoader::toFBXMatrix(ci::Matrix44d m) {
		
		KFbxXMatrix r;
		double * dst = (double*)&r;
		double * src = (double*)&m;
		
		for(int i = 0; i < 16; i++){
			*dst = static_cast<double>( *src);
			dst++;
			src++;
		}
		return r;
	}
	
	
	// ********************************************************************************
	// Texture Methods
	// ********************************************************************************
	
#pragma mark Texture and Material Methods
	
	void S9FbxLoader::loadSupportedMaterials(shared_ptr<FbxDrawable>  pDrawable) {
		mTextureArray.Clear();
		loadSupportedMaterialsRecursive(mScene->GetRootNode(), pDrawable);
	}
	
	void S9FbxLoader::loadSupportedMaterialsRecursive(KFbxNode* pNode, shared_ptr<FbxDrawable>  pDrawable){
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
							shared_ptr<FbxMaterial> material (new FbxMaterial);
							material->isTextured = false;
							
							lProperty = lMaterial->FindProperty(KFbxSurfaceMaterial::sDiffuse);
							if(lProperty.IsValid()){
								lNbTex = lProperty.GetSrcObjectCount(KFbxTexture::ClassId);
								for (lTextureIndex = 0; lTextureIndex < lNbTex; lTextureIndex++){
									lTexture = KFbxCast <KFbxTexture> (lProperty.GetSrcObject(KFbxTexture::ClassId, lTextureIndex)); 
									if(lTexture){
										material->tex = loadTexture(lTexture, pDrawable);
										material->isTextured = true;
									}
								}
							}
							
							// Diffuse Colour
							
							if (lMaterial->GetClassId().Is(KFbxSurfaceLambert::ClassId)) {
								KFbxPropertyDouble3 lKFbxDouble3 =((KFbxSurfaceLambert *)lMaterial)->GetDiffuseColor();
								material->colour = Vec3d(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
							}
							else if (lMaterial->GetClassId().Is(KFbxSurfacePhong::ClassId)){
								KFbxPropertyDouble3 lKFbxDouble3 =((KFbxSurfacePhong *) lMaterial)->GetDiffuseColor();
								material->colour = Vec3d(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
							}
							
							pDrawable->materials.push_back(material);	// Hopefully this index matches
							
						}
					}
				} 
			}
			
			lCount = pNode->GetChildCount();
			
			for (i = 0; i < lCount; i++){
				loadSupportedMaterialsRecursive(pNode->GetChild(i), pDrawable);
			}
		}
	}
	
	gl::Texture S9FbxLoader::loadTexture(KFbxTexture* pTexture, shared_ptr<FbxDrawable>  pDrawable) {
		std::string textureFileName = std::string(pTexture->GetFileName());
		
		int lCount = mTextureArray.GetCount();
		
		map<std::string,gl::Texture>::iterator it;
		it= pDrawable->mMapToTex.find(textureFileName);
		if (it != pDrawable->mMapToTex.end())
			return (*it).second;
		
		// TODO - Fail gracefully (try catch?) if texture not found and disallow texture rendering
		
		app::console() << "Loading texture: " << mPath << "/" << textureFileName << endl;
		gl::Texture texture = loadImage(mPath + "/" + textureFileName); 
		mTextureArray.Add(texture);
		pDrawable->mMapToTex[textureFileName] = texture;
		return texture;
	}
	
	
} // End Namespace S9
