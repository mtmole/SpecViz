
// This file contains object oriented graphics declarations using OpenGL

// macro that throws an assertion if OpenGL encounters an error
#define GLCHECK() { assert(glGetError() == 0); }

// root gl shader class for convenience. Reads shader from a file by default
class Shader {
protected:
	GLuint shaderId;			// shader identifier via OpenGL
	const char* code;			// locally allocated string of the shader code

public:
	// creates an instances of this shader from the provided file path with the given predefined macros
	Shader(const char* filePath, const char* predefine);	

	// compiles the created shader instance from its code
	void Compile();

	// returns the OpenGL shader ID for this shader
	GLuint GetId() const {
		return shaderId;
	}
};

// encapsulates a gl pixel shader
class PixelShader : public Shader {
protected:
public:
	// creates a pixel shader given the file path to the shader code
	PixelShader(const char* path);

	// creates a pixel shader given the path and predefined macros
	PixelShader(const char* path, const char* predefine);
};

// encapsulates a gl vertex shader
class VertexShader : public Shader {
protected:
public:
	// creates a vertex shader given the file path to the shader code
	VertexShader(const char* path);

	// creates a vertex shader given the path and predefined macros
	VertexShader(const char* path, const char* predefine);
};

// encapsulates a gl shader program
class ShaderProgram {
protected:
	GLuint programId;	// shader program ID as represented in OpenGL

public:
	// creates and links a ShaderProgram given the two compiled pixel and vertex instances
	ShaderProgram(PixelShader* pShader, VertexShader* vShader);

	// binds the shader program for drawining
	void Bind();

	// gets the uniform index for the uniform with the given name
	GLint GetUniform(const char* name);
};

// Encapsulates a gl vertex array buffer
class VertexBuffer {
	GLuint buffer;		// the buffer according to OpenGL

public:
	// creates a vertex buffer given the direct data and amount of it
	VertexBuffer(void* data, uint32_t dataSize);

	// destructor
	virtual ~VertexBuffer();

	GLuint GetId() const {
		return buffer;
	}
};

// Encapsulates a gl element array buffer
class IndexBuffer {
	GLuint buffer;			// the buffer id accordin to OpenGL
	GLenum type;			// type used to draw this index buffer (GL_TRIANGLES, the like)
	uint32_t count;			// the number of indices in the buffer

public:
	// creates an index buffer given the data and size, as well as the draw type (GL_TRIANGLES, etc)
	IndexBuffer(void* data, uint32_t dataSize, GLenum drawType);
	virtual ~IndexBuffer();

	// returns the index buffer id according to OpenGL
	GLuint GetId() const {
		return buffer;
	}

	// returns the index draw type of the index buffer (GL_TRIANGLES, etc)
	GLenum GetType() const {
		return type;
	}

	// returns the number of indices in the index buffer
	uint32_t GetCount() const {
		return count;
	}
};

// Encapsulates a vertex array object
class VAO {
protected:
	GLuint id;		// vao id according to OpenGL
public:
	// creates a VAO binding given the vertex buffer and index buffer
	VAO(VertexBuffer* withVerts, IndexBuffer* withIndices);

	// binds the VAO For drawing
	void Bind();

	// enables arrays using the VAO with the given number of attributes
	void EnableArrays(int32_t count);

	// unbinds any VAO (should be used prior to deletion)
	static void Unbind();
};

// encapsulates a gl texture
class Texture { 
protected:
	GLuint textureId;			// texture ID according to OpenGL
	GLuint width;				// texture width in pixels
	GLuint height;				// texture height in pixels
	GLenum format;				// texture format (GL_RGBA8, etc)
	
	// helper functions for generating texture in OpenGL with this texture object
	GLenum GetInternal(GLenum format);
	GLenum GetFormat(GLenum format);
	GLenum GetDataType(GLenum format);

public:
	// creates a texture with the given texture data and format
	Texture(void* data, uint32_t withWidth, uint32_t withHeight, GLenum withFormat);

	// returns the aspect ratio of the texture as a floating point
	float GetAspect() const {
		return (float) width / height;
	}

	// returns the texture width in pixels
	uint32_t GetWidth() const {
		return width;
	}
	
	// returns the texture height in pixels
	uint32_t GetHeight() const {
		return height;
	}
	
	// binds the texture to the given sampler ID in OpenGL
	void Bind(uint32_t samplerId);
	
	// using FreeImage, create a texture image fro the given image path with the desired format
	static Texture* CreateFromFile(const char* filePath, GLenum desiredFormat);

	// using FreeImage, create an RGBA8 texture using the image wirh rgbPath for the color channels, and the
	// image with alphaPath as the alpha channel (a combined image)
	static Texture* CreateFromFileCombined(const char* rgbPath, const char* alphaPath);

	// save the given image data as a PNG using FreeImage with the optional target size (otherwise it will save with the same source size)
	static void SavePNG(const char* filename, uint32_t width, uint32_t height, uint8_t* colors,
		uint32_t destWidth = 0, uint32_t destHeight = 0);
};