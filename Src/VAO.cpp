
#include "SpecViz.h"


VAO::VAO(VertexBuffer* withVerts, IndexBuffer* withIndices) {
	glGenVertexArrays(1, &id);
	glBindVertexArray(id);

	glBindBuffer(GL_ARRAY_BUFFER, withVerts->GetId());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, withIndices->GetId());
	GLCHECK();
}

void VAO::Bind() {
	glBindVertexArray(id);
}

void VAO::EnableArrays(int32_t count) {
	// attribute order is always:
	//   1. Position
	//   2. UV's
	//   3. color
	//   4. Vertex normal

	int32_t strideSize;
	switch (count) {
		case 1: strideSize = sizeof(float) * 3; break;
		case 2: strideSize = sizeof(float) * 5; break;
		case 3: strideSize = sizeof(float) * 9; break; 
		case 4: strideSize = sizeof(float) * 12; break;
		default:
			assert(false);
			break;
	}

	uint8_t* curOffset = NULL;
	for (int32_t i = 0; i < count; i++) {
		glEnableVertexAttribArray(i);

		int32_t numFloats = 0;
		switch (i) {	
			case 0:
			case 3:
				numFloats = 3;
				break;
			case 1:
				numFloats = 2;
				break;
			case 2:
				numFloats = 4;
				break;
			default:
				assert(false);
				break;
		}
		glVertexAttribPointer(i, numFloats, GL_FLOAT, GL_FALSE, strideSize, curOffset);
		curOffset += sizeof(float) * numFloats;
	}
}

void VAO::Unbind() {
	glBindVertexArray(0);
}