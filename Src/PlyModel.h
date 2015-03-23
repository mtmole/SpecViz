#pragma once

#include "SpecViz.h"

class PlyModel {
protected:
	VAO* vao;
	IndexBuffer* iBuffer;
	VertexBuffer* vBuffer;
	glm::vec3 boundMin, boundMax;

public:
	PlyModel(const char* filename);

	glm::vec3 GetScale() const {
		return boundMax - boundMin;
	}

	glm::vec3 GetCenter() const {
		return (boundMax + boundMin) * 0.5f;
	}

	VAO* GetVAO() const {
		return vao;
	}

	IndexBuffer* GetIndexBuffer() const {
		return iBuffer;
	}

	VertexBuffer* GetVertexBuffer() const {
		return vBuffer;
	}

	void Render();
};