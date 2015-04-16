#version 140
 
in  vec3 in_Position;
in  vec2 in_UV;
in  vec4 in_Color;
in  vec3 in_Normal;

out vec4 ex_Color;
out vec2 ex_UV;
out vec3 ex_Normal;
out vec3 ex_EyeDirection;

uniform mat4 objMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform vec3 eyePosition;
 
void main(void)
{
	// determine scene position from data position and object matrix
	vec4 scenePos = objMatrix * vec4(in_Position, 1.0);

	// viewport position is scene position multiplied by view and projection matrices
	gl_Position = projMatrix * (viewMatrix * scenePos);

	// object normal is the vertex normal roated by object matrix to get normal in scene space
	ex_Normal = (objMatrix * vec4(in_Normal, 0.0)).xyz;

	// eye direction determined by taking scene space position and finding normalized difference with eye position
	ex_EyeDirection = normalize(eyePosition - scenePos.xyz / scenePos.w);

	// pass throughs
	ex_Color = in_Color;
	ex_UV = in_UV;
}