#include "SpecViz.h"

VertexBuffer::VertexBuffer(void* data, uint32_t dataSize) {
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, dataSize, data, GL_STATIC_DRAW);
	GLCHECK();
}

VertexBuffer::~VertexBuffer() {
	glDeleteBuffers(1, &buffer);
	buffer = 0;
}

IndexBuffer::IndexBuffer(void* data, uint32_t dataSize, GLenum drawType) : type(drawType) {
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, dataSize, data, GL_STATIC_DRAW);
	GLCHECK();

	count = dataSize / sizeof(int);
}

IndexBuffer::~IndexBuffer() {
	glDeleteBuffers(1, &buffer);
	buffer = 0;
}