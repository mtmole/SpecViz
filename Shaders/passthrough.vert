#version 140
 
in  vec3 in_Position;
in  vec2 in_UV;

out vec2 ex_UV;
out vec3 ex_EyeDirection;

uniform vec3 eyePosition;
uniform vec2 scale;
 
void main(void)
{
	// positions and UVS determined by program (with exception of scaling), simple pass through to pixel shader
	gl_Position = vec4(in_Position.xy * scale, in_Position.z, 1.0);
	ex_UV = in_UV;
	ex_EyeDirection = normalize(eyePosition - in_Position);
}