//
// Bui Tuong Phong shading model (per-pixel) 
// 
// by 
// Massimiliano Corsini
// Visual Computing Lab (2006)
// 
// Modified by Ben Blundell to utilise textures and base colours

varying vec3 normal;
varying vec3 vpos;

void main()
{	
	// vertex normal
	normal = normalize(gl_NormalMatrix * gl_Normal);
	
	// vertex position
	vpos = vec3(gl_ModelViewMatrix * gl_Vertex);
	
	gl_FrontColor = gl_Color;
	
	// Texture unit 0
	gl_TexCoord[0] =  gl_MultiTexCoord0;
	
	// vertex position
	gl_Position = ftransform();
	
	
}
