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
uniform mat4 texMatrix;

uniform vec3 eyePosition;
 
void main(void)
{
	vec4 scenePos = objMatrix * vec4(in_Position, 1.0);
	gl_Position = projMatrix * (viewMatrix * scenePos);
	ex_Color = in_Color;
	ex_Normal = (objMatrix * vec4(in_Normal, 0.0)).xyz;

	vec4 uv = texMatrix * vec4(in_Position, 1.0);
	uv.x = uv.x / uv.w;
	uv.y /= uv.w;
	ex_UV = uv.xy * 0.5 + 0.5;

	ex_EyeDirection = normalize(eyePosition - scenePos.xyz / scenePos.w);
}