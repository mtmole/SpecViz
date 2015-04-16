#include "SpecViz.h"

Shader::Shader(const char* filePath, const char* predefine) : shaderId(0) {
	Log("Opening shader at '%s'", filePath);

	// this constructor simply sets up the shader code memory using the given predefines. Compile() then sends it to OpenGL
	if (predefine == NULL) {
		predefine = "";
	}
	uint32_t predefLength = strlen(predefine);

	// open the file
	FILE* f = NULL;
	fopen_s(&f, filePath, "rb");
	assert(f);
		
	// get the size
	fseek(f, 0, SEEK_END);
	uint32_t fileSize = ftell(f);

	// allocate the code, and copy predefine into it first
	code = (const char*) malloc(fileSize+predefLength+1);
	memset((void*) code, 0, fileSize+predefLength+1);
	strcpy((char*) code, predefine);

	// append code with contents of file via fseek
	fseek(f, 0, SEEK_SET);
	fread((void*) (code+predefLength), 1, fileSize, f);
	fclose(f);
}

void Shader::Compile() {
	// compile this shader given its preallocated code
	glShaderSource(shaderId, 1, &code, NULL); 
	glCompileShader(shaderId);
	GLCHECK();

	GLint ret;
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &ret);

	// output shader compiler errors if there were any
	if (ret == 0) {
		char log[1024];
		int32_t iLength = 0;
		glGetShaderInfoLog(shaderId, 1024, &iLength, log);
		Log(log);
	}

	assert(ret == GL_TRUE);
}

PixelShader::PixelShader(const char* path) : Shader(path, NULL) {
	// create a fragment shader and then compile (already set up through constructor)
	shaderId = glCreateShader(GL_FRAGMENT_SHADER);

	Compile();
}


PixelShader::PixelShader(const char* path, const char* predefine) : Shader(path, predefine) {
	// create a fragment shader and then compile (already set up through constructor)
	shaderId = glCreateShader(GL_FRAGMENT_SHADER);

	Compile();
}

VertexShader::VertexShader(const char* path) : Shader(path, NULL) {
	// create a vertex shader and then compile (already set up through constructor)
	shaderId = glCreateShader(GL_VERTEX_SHADER);

	Compile();
}

VertexShader::VertexShader(const char* path, const char* predefine) : Shader(path, predefine) {
	// create a vertex shader and then compile (already set up through constructor)
	shaderId = glCreateShader(GL_VERTEX_SHADER);

	Compile();
}

ShaderProgram::ShaderProgram(PixelShader* pShader, VertexShader* vShader) {
	// create the program object and attach our two provided shaders
	programId = glCreateProgram();
	glAttachShader(programId, pShader->GetId());
	glAttachShader(programId, vShader->GetId());
	GLCHECK();

	// bind the potential attributes we use
	glBindAttribLocation(programId, 0, "in_Position");
	glBindAttribLocation(programId, 1, "in_UV");
	glBindAttribLocation(programId, 2, "in_Color");
	glBindAttribLocation(programId, 3, "in_Normal");
	GLCHECK();

	// link the program together now
	glLinkProgram(programId);
	GLCHECK();

	// ensure the link succeeded (otherwise we are trying to build a program out of incompatible shaders)
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