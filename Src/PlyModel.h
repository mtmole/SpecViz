#pragma once

#include "SpecViz.h"

// representation of a PLY Model used for the viewer
class PlyModel {
protected:
	// vertex array object is the only thing that needs to be bound when drawing the model
	VAO* vao;

	// index and vertex buffers representing the model data
	IndexBuffer* iBuffer;
	VertexBuffer* vBuffer;

	// bounds for the mesh (used to determine default camera placement, etc)
	glm::vec3 boundMin, boundMax;

public:
	// create a ply model from the given PLY file path
	PlyModel(const char* filename);

	// returns the AABB size of the model
	glm::vec3 GetScale() const {
		return boundMax - boundMin;
	}

	// returns the AABB center of the model
	glm::vec3 GetCenter() const {
		return (boundMax + boundMin) * 0.5f;
	}

	// returns the vertex array object used for rendering the model
	VAO* GetVAO() const {
		return vao;
	}

	// returns the index buffer used for the model data
	IndexBuffer* GetIndexBuffer() const {
		return iBuffer;
	}

	// returns the vertex buffer used for the model data
	VertexBuffer* GetVertexBuffer() const {
		return vBuffer;
	}

	// render the model in OpenGL using the current program and texture settings
	void Render();
};