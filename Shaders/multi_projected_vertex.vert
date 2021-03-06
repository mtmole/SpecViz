#version 140
 
in  vec3 in_Position;
in  vec2 in_UV;
in  vec4 in_Color;
in  vec3 in_Normal;

out vec4 ex_Color;
out vec3 ex_UV[NUM_SAMPLERS];
out vec3 ex_Normal;
out vec3 ex_EyeDirection;

uniform mat4 objMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

uniform mat4 texMatrix[NUM_SAMPLERS];

uniform vec3 eyePosition;
 
void main(void)
{
	// determine scene position from data position and object matrix
	vec4 scenePos = objMatrix * vec4(in_Position, 1.0);

	// viewport position is scene position multiplied by view and projection matrices
	gl_Position = projMatrix * (viewMatrix * scenePos);
	ex_Color = in_Color;

	// object normal is the vertex normal roated by object matrix to get normal in scene space
	ex_Normal = (objMatrix * vec4(in_Normal, 0.0)).xyz;
	
	vec4 uv;
	
	// UV is a deprojected vector based on the view projection matrix provided with the texture representing texture local mesh space
	for (int i = 0; i < NUM_SAMPLERS; i++) {
		uv = texMatrix[i] * vec4(in_Position, 1.0);
		uv.x = uv.x / uv.w;
		uv.y /= uv.w;
		ex_UV[i].xy = uv.xy * 0.5 + 0.5;

		// projected Z value is the face vector amount of the vertex normal in scene space towards the original projection point for that texture
		vec4 texNormal = texMatrix[i] * vec4(in_Normal, 0.0);
		ex_UV[i].z = abs(normalize(texNormal.xyz).z);
	}
	
	// eye direction determined by taking scene space position and finding normalized difference with eye position
	ex_EyeDirection = normalize(eyePosition - scenePos.xyz / scenePos.w);
}