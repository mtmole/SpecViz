#include "SpecViz.h"

Shader::Shader(const char* filePath, const char* predefine) : shaderId(0) {
	Log("Opening shader at '%s'", filePath);

	if (predefine == NULL) {
		predefine = "";
	}
	uint32_t predefLength = strlen(predefine);

	FILE* f = NULL;
	fopen_s(&f, filePath, "rb");
	assert(f);
		
	// get the size
	fseek(f, 0, SEEK_END);
	uint32_t fileSize = ftell(f);
	code = (const char*) malloc(fileSize+predefLength+1);
	memset((void*) code, 0, fileSize+predefLength+1);
	strcpy((char*) code, predefine);

	fseek(f, 0, SEEK_SET);
	fread((void*) (code+predefLength), 1, fileSize, f);
	fclose(f);
}

void Shader::Compile() {
	glShaderSource(shaderId, 1, &code, NULL); 
	glCompileShader(shaderId);
	GLCHECK();

	GLint ret;
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &ret);

	if (ret == 0) {
		char log[1024];
		int32_t iLength = 0;
		glGetShaderInfoLog(shaderId, 1024, &iLength, log);
		Log(log);
	}

	assert(ret == GL_TRUE);
}

PixelShader::PixelShader(const char* path) : Shader(path, NULL) {
	shaderId = glCreateShader(GL_FRAGMENT_SHADER);

	Compile();
}


PixelShader::PixelShader(const char* path, const char* predefine) : Shader(path, predefine) {
	shaderId = glCreateShader(GL_FRAGMENT_SHADER);

	Compile();
}

VertexShader::VertexShader(const char* path) : Shader(path, NULL) {
	shaderId = glCreateShader(GL_VERTEX_SHADER);

	Compile();
}

VertexShader::VertexShader(const char* path, const char* predefine) : Shader(path, predefine) {
	shaderId = glCreateShader(GL_VERTEX_SHADER);

	Compile();
}

ShaderProgram::ShaderProgram(PixelShader* pShader, VertexShader* vShader) {
	programId = glCreateProgram();
	glAttachShader(programId, pShader->GetId());
	glAttachShader(programId, vShader->GetId());
	GLCHECK();

	glBindAttribLocation(programId, 0, "in_Position");
	glBindAttribLocation(programId, 1, "in_UV");
	glBindAttribLocation(programId, 2, "in_Color");
	glBindAttribLocation(programId, 3, "in_Normal");
	GLCHECK();

	glLinkProgram(programId);
	GLCHECK();

	GLint ret;
	glGetProgramiv(programId, GL_LINK_STATUS, &ret);
	assert(ret == GL_TRUE);
}

void ShaderProgram::Bind() {
	glUseProgram(programId);
}

GLint ShaderProgram::GetUniform(const char* name) {
	return glGetUniformLocation(programId, name);
}