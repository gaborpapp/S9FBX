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
 * @file	S9FbxDrawer.h
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



#pragma once

#include "cinder/app/AppBasic.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Matrix.h"
#include "cinder/Font.h"
#include "cinder/Utilities.h"
#include "cinder/gl/Vbo.h"
#include "cinder/Sphere.h"

#include "S9FbxLoader.h"

using namespace cinder;
using namespace ci;

namespace S9 {
	
	class S9FbxDrawer {
		
		/**
		 * This class draws an FBX Drawable and also controls the hierarchy of bones and performs skinning
		 */	
		
	public:
		
		S9FbxDrawer() {};
		
		void	draw(std::shared_ptr<FbxDrawable> drawable);
		void	drawNormals(std::shared_ptr<FbxDrawable> drawable);
		void	rotateBone(std::shared_ptr<FbxMesh> pMesh, int boneid, ci::Matrix44d &mat); 
		void	resetRotations(std::shared_ptr<FbxMesh> pMesh);
		void	drawMeshExtents(std::shared_ptr<FbxMesh> pMesh);
		void	drawClusters(std::shared_ptr<FbxDrawable> pDrawable);
		
		/// Helper Functions for clusters and similar
		
		std::shared_ptr<FbxCluster> getCluster(std::shared_ptr<FbxDrawable> drawable, int boneid);
		
	protected:
		
		void rotateBoneRecursive(std::shared_ptr<FbxRotation> pRot, std::shared_ptr<Matrix44d> pmat, Matrix44d rmat, Matrix44d mat, std::shared_ptr<FbxMesh> pMesh);
		void applyRotations(std::shared_ptr<FbxMesh> pMesh);
		
	};
	
	
}
