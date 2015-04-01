
#define GLCHECK() { assert(glGetError() == 0); }

// root gl shader class for convenience. Reads shader from a file by default
class Shader {
protected:
	GLuint shaderId;
	const char* code;

public:
	Shader(const char* filePath, const char* predefine);
	void Compile();

	GLuint GetId() const {
		return shaderId;
	}
};

// encapsulates a gl pixel shader
class PixelShader : public Shader {
protected:
public:
	PixelShader(const char* path);
	PixelShader(const char* path, const char* predefine);
};

// encapsulates a gl vertex shader
class VertexShader : public Shader {
protected:
public:
	VertexShader(const char* path);
	VertexShader(const char* path, const char* predefine);
};

// encapsulates a gl shader program
class ShaderProgram {
protected:
	GLuint programId;

public:
	ShaderProgram(PixelShader* pShader, VertexShader* vShader);
	void Bind();
	GLint GetUniform(const char* name);
};

// Encapsulates a gl vertex array buffer
class VertexBuffer {
	GLuint buffer;

public:
	VertexBuffer(void* data, uint32_t dataSize);
	virtual ~VertexBuffer();

	GLuint GetId() const {
		return buffer;
	}
};

// Encapsulates a gl element array buffer
class IndexBuffer {
	GLuint buffer;
	GLenum type;			// type used to draw this index buffer (GL_TRIANGLES, the like)
	uint32_t count;

public:
	IndexBuffer(void* data, uint32_t dataSize, GLenum drawType);
	virtual ~IndexBuffer();

	GLuint GetId() const {
		return buffer;
	}

	GLenum GetType() const {
		return type;
	}

	uint32_t GetCount() const {
		return count;
	}
};

// Encapsulates a vertex array object
class VAO {
protected:
	GLuint id;		// vao id
public:
	VAO(VertexBuffer* withVerts, IndexBuffer* withIndices);

	void Bind();

	void EnableArrays(int32_t count);

	static void Unbind();
};

// encapsulates a gl texture
class Texture { 
protected:
	GLuint textureId;
	GLuint width;
	GLuint height;
	GLenum format;
	
	GLenum GetInternal(GLenum format);
	GLenum GetFormat(GLenum format);
	GLenum GetDataType(GLenum format);

public:
	Texture(void* data, uint32_t withWidth, uint32_t withHeight, GLenum withFormat);

	float GetAspect() const {
		return (float) width / height;
	}

	uint32_t GetWidth() const {
		return width;
	}
	
	uint32_t GetHeight() const {
		return height;
	}

	void Bind(uint32_t samplerId);
	static Texture* CreateFromFile(const char* filePath, GLenum desiredFormat);
	static Texture* CreateFromFileCombined(const char* rgbPath, const char* alphaPath);
	static void SavePNG(const char* filename, uint32_t width, uint32_t height, uint8_t* colors,
		uint32_t destWidth = 0, uint32_t destHeight = 0);
};