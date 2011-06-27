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
 * @brief	A set of classes that draw fbxdrawables.
 *			
 *
 * @file	S9FbxDrawer.cpp
 * @author	Benjamin Blundell <oni@section9.co.uk>
 * @date	27/06/2011
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


#include "S9FbxDrawer.h"


using namespace std;

namespace S9 {
	
	
#pragma mark S9FbxDrawer Class
	
	void S9FbxDrawer::draw(shared_ptr<FbxDrawable> drawable) {
		
		// Draw Normally
		
		for (vector< shared_ptr<FbxMesh> >::iterator it = drawable->meshes.begin(); it != drawable->meshes.end(); it ++){
			
			shared_ptr<FbxMesh> pMesh = *it;
			
			
			glPushMatrix();
			glMultMatrixf(pMesh->offset);
			
			
			if (pMesh->mDeform) applyRotations(pMesh);
			
			if (pMesh->indicies.size() >0 ){
				int matid = -1;
				int *ip = (int*)&pMesh->indicies.at(0);
				
				for (int i =0; i < pMesh->numtris; i ++){
					if (pMesh->matindicies.size() > 0){
						matid = pMesh->matindicies[i];
						
						if (matid >= 0) {
							if (drawable->materials[matid]->isTextured) {
								drawable->materials[matid]->tex.bind();
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
							}
							
							float r = drawable->materials[matid]->colour.x;
							float g = drawable->materials[matid]->colour.y;
							float b = drawable->materials[matid]->colour.z;
							
							glColor3f(r,g,b);
							
							// Full Materials For later! :D
							//	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE, pDancer->mDancer.lMaterials[matid].glMat);
							
						}
					}
					glBegin(GL_TRIANGLES);
					
					int i0 = *ip; ++ip; int i1 = *ip; ++ip; int i2 = *ip; ++ip;
					
					if (i0 < pMesh->numverts && i1 < pMesh->numverts && i2 < pMesh->numverts) {
						
						Vec3d v0 = pMesh->vertices[i0];	Vec3d v1 = pMesh->vertices[i1];	Vec3d v2 = pMesh->vertices[i2];
						if (pMesh->mDeform){ v0 = pMesh->skinvertices[i0];	v1 = pMesh->skinvertices[i1]; v2 = pMesh->skinvertices[i2]; }
						
						///\todo how are normals affected during the mesh warp process? -  do we need skin normals too?
						
						Vec3d n0 = pMesh->normals[i0];	Vec3d n1 = pMesh->normals[i1];	Vec3d n2 = pMesh->normals[i2];
						if (pMesh->mDeform){ n0 = pMesh->skinnormals[i0]; n1 = pMesh->skinnormals[i1]; n2 = pMesh->skinnormals[i2]; }
						
						Vec2d t0 = pMesh->texcoords[i0]; Vec2d t1 = pMesh->texcoords[i1]; Vec2d t2 = pMesh->texcoords[i2];
						
						glNormal3f(n0.x,n0.y,n0.z); glTexCoord2d(t0.x, t0.y); glVertex3d(v0.x, v0.y, v0.z);
						glNormal3f(n1.x,n1.y,n1.z); glTexCoord2d(t1.x, t1.y); glVertex3d(v1.x, v1.y, v1.z);
						glNormal3f(n2.x,n2.y,n2.z); glTexCoord2d(t2.x, t2.y); glVertex3d(v2.x, v2.y, v2.z);
						glEnd();
						
						if (matid >= 0) {
							if (drawable->materials[matid]->isTextured)
								drawable->materials[matid]->tex.unbind();
						}
					}
					
				}
			
				glPopMatrix();
				
			}
		}
		
	}
	
	void S9FbxDrawer::drawNormals(shared_ptr<FbxDrawable> drawable) {
		
		// Draw Normally
		
		for (vector< shared_ptr<FbxMesh> >::iterator it = drawable->meshes.begin(); it != drawable->meshes.end(); it ++){
			
			shared_ptr<FbxMesh> pMesh = *it;
			
			glPushMatrix();
			glMultMatrixf(pMesh->offset);
			
			if (pMesh->mDeform) applyRotations(pMesh);
			
			if (pMesh->indicies.size() >0 ){
				int matid = -1;
				int *ip = (int*)&pMesh->indicies.at(0);
				
				for (int i =0; i < pMesh->numtris; i ++){
		
					glBegin(GL_LINES);
					
					glColor3f(1.0, 0, 0);
					
					int i0 = *ip; ++ip; int i1 = *ip; ++ip; int i2 = *ip; ++ip;
					
					Vec3d v0 = pMesh->vertices[i0];	Vec3d v1 = pMesh->vertices[i1];	Vec3d v2 = pMesh->vertices[i2];
					if (pMesh->mDeform){ v0 = pMesh->skinvertices[i0];	v1 = pMesh->skinvertices[i1]; v2 = pMesh->skinvertices[i2]; }
					
					Vec3d n0 = pMesh->normals[i0];	Vec3d n1 = pMesh->normals[i1];	Vec3d n2 = pMesh->normals[i2];
					if (pMesh->mDeform){ n0 = pMesh->skinnormals[i0]; n1 = pMesh->skinnormals[i1]; n2 = pMesh->skinnormals[i2]; }
					
					n0 += v0;
					n1 += v1;
					n2 += v2;
					
					glVertex3f(v0.x, v0.y, v0.z); glVertex3f(n0.x, n0.y, n0.z);
					glVertex3f(v1.x, v1.y, v1.z); glVertex3f(n1.x, n1.y, n1.z);
					glVertex3f(v2.x, v2.y, v2.z); glVertex3f(n2.x, n2.y, n2.z);
					glEnd();
					
					
				}
				glPopMatrix();
			
			}
		}
		
	}
	
	///\todo - This should really be a shader operation - it would be faster and no need to copy the vertices
	
	
	void S9FbxDrawer::applyRotations(shared_ptr<FbxMesh> pMesh) {
		
		//int clusterMode = pMesh->clusters[0]->mode;
		
		// set skin matrices to ALL ZEROS if not additive (which it is likely to be but we'll need to change that)
		for (vector<Matrix44d>::iterator it = pMesh->skinmatrices.begin(); it != pMesh->skinmatrices.end(); it++){
			(*it) = Matrix44d(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
		}
		
		// set skin weights to zero
		
		for (vector<float>::iterator it = pMesh->skinweights.begin(); it != pMesh->skinweights.end(); it++){
			*it = 0.0;
		}
		
		
		// Compute the skin matrices (a matrix for each vertex) - This is the shader bit I'd have thought
		for (int i = 0; i < pMesh->clusters.size(); ++i){
			shared_ptr<FbxCluster> cinderCluster = pMesh->clusters[i];
			
			Matrix44d clusterMatrix;
			clusterMatrix = cinderCluster->pretransform * (*cinderCluster->transform) * cinderCluster->posttransform;				
			
			// Modify this cluster's matrices by the current indicies weight then add this up for all matrices
			// affecting that vertex (as multiple clusters can influence one vertex)
			
			Vec3f centre(0,0,0);
			
			for (int j=0; j < cinderCluster->indicies.size(); ++j) {
				
				int idx = cinderCluster->indicies[j];
				float w = cinderCluster->weights[j];
				
				if (w == 0.0)
					continue;
				
				Matrix44d influenceMatrix = clusterMatrix * w;
				
				pMesh->skinmatrices[idx] = pMesh->skinmatrices[idx] + influenceMatrix;
				pMesh->skinweights[idx] += w;
				
			}
			
			//	cinderCluster->mCentre = centre / cinderCluster->indicies.size();
		}
		
		pMesh->skinvertices = pMesh->vertices;
		
		
		for (int i =0; i < pMesh->skinvertices.size(); ++i){
			float w = pMesh->skinweights[i];
			if (w != 0.0){
				pMesh->skinvertices[i] = pMesh->skinmatrices[i] * pMesh->vertices[i];
				pMesh->skinnormals[i] = pMesh->skinmatrices[i].transformVec(pMesh->normals[i]);
			//	pMesh->skinnormals[i] = pMesh->normals[i];
			}
		}
		
		
		///\todo - There is definitely a performance hit here. Can we perhaps do some things with that?
		
		
		for (int i = 0; i < pMesh->clusters.size(); ++i){
			shared_ptr<FbxCluster> cinderCluster = pMesh->clusters[i];
			
			// Modify this cluster's matrices by the current indicies weight then add this up for all matrices
			// affecting that vertex (as multiple clusters can influence one vertex)
			
			Vec3f centre(0,0,0);
			
			cinderCluster->mMax = Vec3f(-1e10,-1e10,-1e10);
			cinderCluster->mMin = Vec3f(1e10,1e10,1e10);
			
			for (int j=0; j < cinderCluster->indicies.size(); ++j) {
				//Centre points - how best to do this? Dont want to loop again! ><
				
				int idx = cinderCluster->indicies[j];
				
				Vec3f vert = pMesh->skinvertices[idx];
				
				cinderCluster->mMax.x = vert.x > cinderCluster->mMax.x ? vert.x : cinderCluster->mMax.x;
				cinderCluster->mMax.y = vert.y > cinderCluster->mMax.y ? vert.y : cinderCluster->mMax.y;
				cinderCluster->mMax.z = vert.z > cinderCluster->mMax.z ? vert.z : cinderCluster->mMax.z;
				
				cinderCluster->mMin.x = vert.x < cinderCluster->mMin.x ? vert.x : cinderCluster->mMin.x;
				cinderCluster->mMin.y = vert.y < cinderCluster->mMin.y ? vert.y : cinderCluster->mMin.y;
				cinderCluster->mMin.z = vert.z < cinderCluster->mMin.z ? vert.z : cinderCluster->mMin.z;
				
				centre += vert;
				
			}
			
			if (centre != Vec3f::zero()){
				cinderCluster->mCentre = centre * 1.0 / static_cast<float>(cinderCluster->indicies.size());
			}
			else {
				cinderCluster->mCentre = centre;
			}
		}
	}
	
	
	void S9FbxDrawer::resetRotations(shared_ptr<FbxMesh> pMesh) {
		
		for (vector < shared_ptr<FbxRotation> >::iterator it = pMesh->bones.begin(); it != pMesh->bones.end(); it ++){
			*(*it)->realMatrix = (*it)->baseMatrix;
		}
	}
	
	void S9FbxDrawer::drawMeshExtents(shared_ptr<FbxMesh> pMesh) {
		
		glColor3f(1,0,1);
		glLineWidth(3.0f);
		glPushMatrix();
		glMultMatrixf(pMesh->offset);
		
		glBegin(GL_LINES);
		
		// Top
		glVertex3f(pMesh->mMin.x, pMesh->mMax.y, pMesh->mMin.z); glVertex3f(pMesh->mMax.x, pMesh->mMax.y, pMesh->mMin.z);
		glVertex3f(pMesh->mMax.x, pMesh->mMax.y, pMesh->mMin.z); glVertex3f(pMesh->mMax.x, pMesh->mMax.y, pMesh->mMax.z);
		glVertex3f(pMesh->mMax.x, pMesh->mMax.y, pMesh->mMax.z); glVertex3f(pMesh->mMin.x, pMesh->mMax.y, pMesh->mMax.z);
		glVertex3f(pMesh->mMin.x, pMesh->mMax.y, pMesh->mMax.z); glVertex3f(pMesh->mMin.x, pMesh->mMax.y, pMesh->mMin.z);
		
		// Bottom
		glVertex3f(pMesh->mMin.x, pMesh->mMin.y, pMesh->mMin.z); glVertex3f(pMesh->mMax.x, pMesh->mMin.y, pMesh->mMin.z);
		glVertex3f(pMesh->mMax.x, pMesh->mMin.y, pMesh->mMin.z); glVertex3f(pMesh->mMax.x, pMesh->mMin.y, pMesh->mMax.z);
		glVertex3f(pMesh->mMax.x, pMesh->mMin.y, pMesh->mMax.z); glVertex3f(pMesh->mMin.x, pMesh->mMin.y, pMesh->mMax.z);
		glVertex3f(pMesh->mMin.x, pMesh->mMin.y, pMesh->mMax.z); glVertex3f(pMesh->mMin.x, pMesh->mMin.y, pMesh->mMin.z);
		
		// Front
		glVertex3f(pMesh->mMin.x, pMesh->mMax.y, pMesh->mMax.z); glVertex3f(pMesh->mMin.x, pMesh->mMin.y, pMesh->mMax.z);
		glVertex3f(pMesh->mMax.x, pMesh->mMax.y, pMesh->mMax.z); glVertex3f(pMesh->mMax.x, pMesh->mMin.y, pMesh->mMax.z);
		
		// Back
		glVertex3f(pMesh->mMin.x, pMesh->mMax.y, pMesh->mMin.z); glVertex3f(pMesh->mMin.x, pMesh->mMin.y, pMesh->mMin.z);
		glVertex3f(pMesh->mMax.x, pMesh->mMax.y, pMesh->mMin.z); glVertex3f(pMesh->mMax.x, pMesh->mMin.y, pMesh->mMin.z);
		
		glEnd();
		
	}
	
	void S9FbxDrawer::drawClusters(shared_ptr<FbxDrawable> pDrawable) {
		
		shared_ptr<FbxMesh> pMesh = pDrawable->meshes[0];
		Sphere tsphere;
		tsphere.setCenter(Vec3f(0,0,0));
		
		int idx =0;
		
		for (vector<shared_ptr<FbxCluster> >::iterator it = pMesh->clusters.begin(); it != pMesh->clusters.end(); it++) {
			shared_ptr<FbxCluster> pCluster = *it;
			
			if (pCluster->mCentre != Vec3f::zero()) {
				
				Vec3f dv = pCluster->mMax;
				float d = dv.distance(pCluster->mMin);
				
				tsphere.setRadius(d / 2.0);	// TODO - with the max and min its not great. Bounding box maybe better
				glPushMatrix();
				glTranslatef(pCluster->mCentre.x, pCluster->mCentre.y, pCluster->mCentre.z);
				gl::draw(tsphere);
				glPopMatrix();
			}
			idx++;
		}
	}
	
	
#pragma mark Bone Rotations
	
	void S9FbxDrawer::rotateBone(shared_ptr<FbxMesh> pMesh, int boneid, ci::Matrix44d &mat) {
		if (boneid > -1 && boneid < pMesh->bones.size()) {
			
			shared_ptr<FbxRotation> pBone = pMesh->bones[boneid];
			if (mat != pBone->rotMatrix){
				
				for (vector < shared_ptr<FbxRotation> >::iterator it = pMesh->bones.begin(); it != pMesh->bones.end(); it ++){
					(*it)->targeted = false;
				}
				
				Matrix44d tmp (pBone->rotMatrix);
				
				tmp = tmp.inverted() * mat;
				
				pBone->rotMatrix = mat;
				*pBone->realMatrix = *pBone->realMatrix  * tmp;
				pBone->targeted = true;
				
				
				for (vector < shared_ptr<FbxRotation> >::iterator it = pMesh->bones.begin(); it != pMesh->bones.end(); it ++){
					if ((*it)->parent == pMesh->bones[boneid]){
						rotateBoneRecursive(*it, pMesh->bones[boneid]->realMatrix, tmp, mat, pMesh);
					}
				}
			}
		}
	}
	
	
	
	void S9FbxDrawer::rotateBoneRecursive(shared_ptr<FbxRotation> pRot, shared_ptr<Matrix44d> pmat, Matrix44d rmat, Matrix44d mat, shared_ptr<FbxMesh> pMesh) {
		pRot->targeted = true;
	
		*(pRot->realMatrix) =  (*pmat) * rmat * pmat->inverted() * (*pRot->realMatrix);
		
		for (vector < shared_ptr<FbxRotation> >::iterator it = pMesh->bones.begin(); it != pMesh->bones.end(); it ++){
			if ((*it)->parent == pRot){
				rotateBoneRecursive(*it, pmat, rmat, mat, pMesh);
			}
		}
		
	}
	
	
}