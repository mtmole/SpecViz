
#include "PlyModel.h"
#include <fstream>
#include <vector>

using namespace std;

// struct represents a vertex used for PlyModels in both the loading process and how vertices stored in data for GPU
struct PlyVertex {
	glm::vec3 position;
	glm::vec2 uv;
	glm::vec4 color;
	glm::vec3 normal;

	PlyVertex() {
		position = glm::vec3(0,0,0);
		uv = glm::vec2(0.5,0.5);
		color = glm::vec4(1,1,1,1);
		normal = glm::vec3(0,0,0);
	}
};

// verious PLY property types that represent various  OpenGL vertex attributes or their components
enum PlyPropertyType {
	PPT_X,
	PPT_Y,
	PPT_Z,
	PPT_U,
	PPT_V,
	PPT_R,
	PPT_G,
	PPT_B,
	PPT_NX,
	PPT_NY,
	PPT_NZ,
	PPT_Intensity,
	PPT_Indices,
	PPT_Unknown
};

// given a property name returns the type as determined by empirical evidence
PlyPropertyType GetPropType(const char* withName) {
	if (strlen(withName) == 1) {
		switch (withName[0]) {
			case 'x': return PPT_X;
			case 'y': return PPT_Y;
			case 'z': return PPT_Z;
			case 'u': return PPT_U;
			case 'v': return PPT_V;
			case 'r': return PPT_R;
			case 'g': return PPT_G;
			case 'b': return PPT_B;
		}

		return PPT_Unknown;
	}
	
	if (!strcmp(withName, "red")) 
		return PPT_R;

	if (!strcmp(withName, "green")) 
		return PPT_G;

	if (!strcmp(withName, "blue")) 
		return PPT_B;

	if (!strcmp(withName, "nx")) 
		return PPT_NX;

	if (!strcmp(withName, "ny")) 
		return PPT_NY;

	if (!strcmp(withName, "nz")) 
		return PPT_NZ;

	if (!strcmp(withName, "intensity")) 
		return PPT_Intensity;

	if (!strcmp(withName, "vertex_indices"))
		return PPT_Indices;

	return PPT_Unknown;
}

// various supported ply data formats
enum PlyPropertyFormat {
	PPF_Float,
	PPF_Int,
	PPF_Uchar,
	PPF_List,
	PPF_Unknown
};

// given the PLY model provided property format, returns our enum equivalent
PlyPropertyFormat GetPropFormat(const char* withName) {
	if (!strcmp(withName, "float") || !strcmp(withName, "float32"))
		return PPF_Float;
	if (!strcmp(withName, "uint8") || !strcmp(withName, "uint16") || !strcmp(withName, "uint32") ||
		!strcmp(withName, "int8") || !strcmp(withName, "int16") || !strcmp(withName, "int32"))
		return PPF_Int;
	if (!strcmp(withName, "uchar"))
		return PPF_Uchar;
	if (!strcmp(withName, "list"))
		return PPF_List;

	return PPF_Unknown;
}

struct PlyProperty {
	PlyPropertyType type;
	PlyPropertyFormat format;
};

// swaps a value for BIG_ENDIAN <-> LITTLE_ENDIAN conversion purposes
template<class T> 
T byte_swap(T& withValue) {
	assert(sizeof(T) == 4);
	uint32_t ret;
	ret =
		((*((uint32_t*)&withValue) & 0xFF000000) >> 24) |
		((*((uint32_t*)&withValue) & 0x00FF0000) >> 8) |
		((*((uint32_t*)&withValue) & 0x0000FF00) << 8) |
		((*((uint32_t*)&withValue) & 0x000000FF) << 24);
	return *((T*)&ret);
}

struct PlyElement {
	std::vector<PlyProperty> properties;
	const char* name; 
	uint32_t count;

	virtual void prepare() {
	}

	void read_ascii(fstream& file) {
		prepare();

		// count elements:
		for (uint32_t i = 0; i < count; i++) {
			// read each property:
			for (uint32_t p = 0; p < properties.size(); p++) {
				read_prop_ascii(i, file, properties[p]);
			}
		}
	}

	void read_prop_ascii(uint32_t index, fstream& file, const PlyProperty& prop) {
		switch (prop.format) {
			case PPF_Float:
			{
				float into = 0.0f;
				file >> into;
				read_prop_float(index, prop.type, into);
				return;
			}
			case PPF_Int:
			{
				int32_t into = 0;
				file >> into;
				read_prop_int(index, prop.type, into);
				return;
			}
			case PPF_List:
			{
				int32_t count = 0;
				file >> count;
				read_prop_list_ascii(index, prop.type, count, file);
				return;
			}
			default:
			{
				// unknown, just read and move on
				char buffer[512];
				file >> buffer;
			}
		}
	}
	
	void read_binary(fstream& file, bool bigEndian) {
		prepare();

		// count elements:
		for (uint32_t i = 0; i < count; i++) {
			// read each property:
			for (uint32_t p = 0; p < properties.size(); p++) {
				read_prop_binary(i, file, properties[p], bigEndian);
			}
		}
	}

	void read_prop_binary(uint32_t index, fstream& file, const PlyProperty& prop, bool bigEndian) {
		switch (prop.format) {
			case PPF_Float:
			{
				uint32_t offset = (uint32_t) file.tellg();
				float into = 0.0f;
				file.read((char*) &into, sizeof(float));
				if (bigEndian) {
					into = byte_swap<float>(into);
				}
				read_prop_float(index, prop.type, into);
				return;
			}
			case PPF_Int:
			{
				int32_t into = 0;
				file.read((char*) &into, sizeof(int32_t));
				if (bigEndian) {
					into = byte_swap<int32_t>(into);
				}
				read_prop_int(index, prop.type, into);
				return;
			}
			case PPF_List:
			{
				uint8_t count = file.get();
				read_prop_list_binary(index, prop.type, count, file, bigEndian);
				return;
			}
			case PPF_Uchar:
			{
				uint8_t into = 0;
				file.read((char*) &into, sizeof(into));
				read_prop_float(index, prop.type, (float) into / 255.0f);
				return;
			}
			default:
			{
				// unknown, just read and move on
				char buffer[512];
				file >> buffer;
			}
		}
	}

	virtual void read_prop_float(uint32_t index, PlyPropertyType type, float value) {
		// default does nothing
	}

	virtual void read_prop_int(uint32_t index, PlyPropertyType type, int32_t value) {
		// default does nothing
	}

	virtual void read_prop_list_ascii(uint32_t index, PlyPropertyType type, int32_t count, fstream& file) {
		// default just reads in the list values and does nothing
		char buffer[512];
		for (int32_t i = 0; i < count; i++) {
			file >> buffer;
		}
	}
	
	virtual void read_prop_list_binary(uint32_t index, PlyPropertyType type, int32_t count, fstream& file, bool bigEndian) {
		// default just reads in the list values and does nothing
		char buffer[512];
		for (int32_t i = 0; i < count; i++) {
			file.read(buffer, sizeof(int32_t));
		}
	}

	bool has_type(PlyPropertyType type) {
		for (uint32_t i = 0; i < properties.size(); i++) {
			if (properties[i ].type == type)
				return true;
		}

		return false;
	}

	virtual ~PlyElement() {
		if (name) {
			free((void*) name);
			name = NULL;
		}
	}
};

struct VertexPlyElement : public PlyElement {
	std::vector<PlyVertex> vertices;

	void prepare() {
		for (uint32_t i = 0; i < count; i++) {
			vertices.push_back(PlyVertex());
		}
	}

	virtual void read_prop_float(uint32_t index, PlyPropertyType type, float value) {
		switch (type) {	
			case PPT_X: vertices[index].position.x = value; return;
			case PPT_Y: vertices[index].position.y = value; return;
			case PPT_Z: vertices[index].position.z = value; return;
			case PPT_U: vertices[index].uv.x = value; return;
			case PPT_V: vertices[index].uv.y = value; return;
			case PPT_R: vertices[index].color.r = value; return;
			case PPT_G: vertices[index].color.g = value; return;
			case PPT_B: vertices[index].color.b = value; return;
			case PPT_NX: vertices[index].normal.x = value; return;
			case PPT_NY: vertices[index].normal.y = value; return;
			case PPT_NZ: vertices[index].normal.z = value; return;
			default:
				// we don't handle other types of data
				return;
		}
	}

	VertexBuffer* CreateVertexBuffer() {
		return new VertexBuffer(&vertices[0], sizeof(PlyVertex) * vertices.size());
	}

	void calc_bounds(glm::vec3& intoMin, glm::vec3& intoMax) {
		intoMin = vertices[0].position;
		intoMax = vertices[0].position;
		for (uint32_t i = 0; i < vertices.size(); i++) {
			if (intoMin.x > vertices[i].position.x) intoMin.x = vertices[i].position.x;
			if (intoMin.y > vertices[i].position.y) intoMin.y = vertices[i].position.y;
			if (intoMin.z > vertices[i].position.z) intoMin.z = vertices[i].position.z;
			if (intoMax.x < vertices[i].position.x) intoMax.x = vertices[i].position.x;
			if (intoMax.y < vertices[i].position.y) intoMax.y = vertices[i].position.y;
			if (intoMax.z < vertices[i].position.z) intoMax.z = vertices[i].position.z;
		}
	}

	glm::vec3 calc_avg() {
		glm::vec3 ret(0,0,0);
		glm::vec3 running(0,0,0);
		float samples = 0.0f;
		for (uint32_t i = 0; i < vertices.size(); i++) {
			running += vertices[i].position;
			if ((i+1) % 1024 == 0) {
				ret += running / 1024.0f;
				samples += 1.0f;
				running = glm::vec3(0,0,0);
			}
		}
		ret += running / 1024.0f;
		samples += (vertices.size() % 1024) / 1024.0f;
		ret /= samples;
		return ret;
	}

	void offset(glm::vec3 byAmount) {
		for (uint32_t i = 0; i < vertices.size(); i++) {
			vertices[i].position += byAmount;
		}
	}

	void construct_normals(const std::vector<uint32_t>& withFaces) {
		// set all normals to 0:
		for (uint32_t i = 0; i < vertices.size(); i++) {
			vertices[i].normal = glm::vec3(0,0,0);
		}

		// add normals of all faces to their used vertices
		for (uint32_t index = 0; index < withFaces.size(); index += 3) {
			glm::vec3 faceNormal =
				glm::normalize(
				glm::cross(
				    vertices[withFaces[index]].position - vertices[withFaces[index+1]].position,
				    vertices[withFaces[index]].position - vertices[withFaces[index+2]].position)
					);
			
			vertices[withFaces[index]].normal += faceNormal;
			vertices[withFaces[index+1]].normal += faceNormal;
			vertices[withFaces[index+2]].normal += faceNormal;
		}

		// normalize all results:
		for (uint32_t i = 0; i < vertices.size(); i++) {
			vertices[i].normal = glm::normalize(vertices[i].normal);
		}
	}
};

struct FacePlyElement : public PlyElement {
	std::vector<uint32_t> indices;

	uint32_t get_highest() {
		uint32_t ret = 0;
		for (uint32_t i = 0; i < indices.size(); i++) {
			if (indices[i] > ret) {
				ret = indices[i];
			}
		}
		return ret;
	}
	
	virtual void read_prop_list_ascii(uint32_t index, PlyPropertyType type, int32_t count, fstream& file) {
		if (type == PPT_Indices) {
			if (count == 3) {
				// triangle:
				uint32_t i[3];
				file >> i[0] >> i[1] >> i[2];
				indices.push_back(i[0]);
				indices.push_back(i[1]);
				indices.push_back(i[2]);
				return;
			} 
			if (count == 4) {
				// quad
				uint32_t i[4];
				file >> i[0] >> i[1] >> i[2] >> i[3];
				indices.push_back(i[0]);	// quad = two triangles
				indices.push_back(i[1]);
				indices.push_back(i[2]);
				indices.push_back(i[3]);		
				indices.push_back(i[1]);
				indices.push_back(i[2]);
				return;
			}
		}
		// otherwise currently unsupported:
		PlyElement::read_prop_list_ascii(index, type, count, file);
	}

	virtual void read_prop_list_binary(uint32_t index, PlyPropertyType type, int32_t count, fstream& file, bool bigEndian) {
		if (type == PPT_Indices) {
			if (count == 3) {
				// triangle:
				uint32_t i[3];
				file.read((char*) &i[0], sizeof(uint32_t));
				file.read((char*) &i[1], sizeof(uint32_t));
				file.read((char*) &i[2], sizeof(uint32_t));
				if (bigEndian) {
					i[0] = byte_swap<uint32_t>(i[0]);
					i[1] = byte_swap<uint32_t>(i[1]);
					i[2] = byte_swap<uint32_t>(i[2]);
				}
				indices.push_back(i[0]);
				indices.push_back(i[1]);
				indices.push_back(i[2]);
				return;
			} 
			if (count == 4) {
				// quad
				uint32_t i[4];
				file.read((char*) &i[0], sizeof(uint32_t));
				file.read((char*) &i[1], sizeof(uint32_t));
				file.read((char*) &i[2], sizeof(uint32_t));
				file.read((char*) &i[3], sizeof(uint32_t));
				if (bigEndian) {
					i[0] = byte_swap<uint32_t>(i[0]);
					i[1] = byte_swap<uint32_t>(i[1]);
					i[2] = byte_swap<uint32_t>(i[2]);
					i[3] = byte_swap<uint32_t>(i[3]);
				}
				indices.push_back(i[0]);	// quad = two triangles
				indices.push_back(i[1]);
				indices.push_back(i[2]);
				indices.push_back(i[3]);		
				indices.push_back(i[1]);
				indices.push_back(i[2]);
				return;
			}
		}
		// otherwise currently unsupported:
		PlyElement::read_prop_list_binary(index, type, count, file, bigEndian);
	}

	IndexBuffer* CreateIndexBuffer() {
		return new IndexBuffer(&indices[0], 4 * indices.size(), GL_TRIANGLES);
	}
};

PlyModel::PlyModel(const char* filename) : vao(NULL), iBuffer(NULL), vBuffer(NULL) {
	fstream file(filename);
	char buffer[512], buffer2[512], buffer3[512];

	// make sure it is an ascii ply file
	file >> buffer;
	if (strcmp(buffer, "ply")) {
		file.close();
		Log("Not a ply file.");
		return;
	}
	file >> buffer >> buffer2 >> buffer3;

	bool isAscii = !strcmp(buffer2, "ascii");
	bool bigEndian = false, isBinary = false;
	if (!strcmp(buffer2, "binary_little_endian")) {
		isBinary = true;
	} else if (!strcmp(buffer2, "binary_big_endian")) {
		bigEndian = true;
		isBinary = true;
	}
	if (strcmp(buffer, "format") || strcmp(buffer3, "1.0") || (!isAscii && !isBinary)) {
		file.close();
		Log("Not an ascii or binary 1.0 formatted ply file.");
		return;
	}

	// read in our elements and their associated properties:
	VertexPlyElement* vertElement = NULL;
	FacePlyElement* faceElement = NULL;
	std::vector<PlyElement*> elements;
	do {
		file >> buffer;
		while (!strcmp(buffer, "element")) {
			uint32_t count;
			file >> buffer;			// element name;
			file >> count;			// element count
			PlyElement* newElement;
			if (!strcmp(buffer, "vertex")) {
				newElement = new VertexPlyElement;
				vertElement = (VertexPlyElement*) newElement;
			} else if (!strcmp(buffer, "face")) {
				newElement = new FacePlyElement;
				faceElement = (FacePlyElement*) newElement;
			} else {
				newElement = new PlyElement;
			};
			newElement->name = _strdup(buffer);
			newElement->count = count;

			// read in properties
			PlyProperty newProp;
			file >> buffer;
			while (!strcmp(buffer, "property")) {
				// format
				file >> buffer;
				newProp.format = GetPropFormat(buffer);

				if (newProp.format == PPF_List) {
					// trash the list types for now, we don't use them:
					file >> buffer >> buffer2;
				}

				// type
				file >> buffer;
				newProp.type = GetPropType(buffer);

				newElement->properties.push_back(newProp);

				file >> buffer;
			}

			elements.push_back(newElement);
		}
	} while (strcmp(buffer, "end_header"));		// read until the end of the header

	// error if there isn't a vertex or face element set up:
	if (!vertElement || !faceElement) {
		Log("Couldn't find both vertices and faces in the ply file for loading!");
		file.close();
		return;
	}

	// allow each element to read itself in:
	if (isAscii) {
		for (uint32_t i = 0; i < elements.size(); i++) {
			elements[i]->read_ascii(file);
		}
	} else {
		assert(isBinary);

		bool carriageReturn = file.peek() == 0x0a;

		// reopen file as binary
		uint32_t offset = (uint32_t) file.tellg();
		file.close();
		file.open(filename, std::ifstream::in | std::ifstream::binary);
		char* textHeader = (char*) malloc(offset+1);
		memset(textHeader, 0, offset+1);
		file.read(textHeader, offset);
		char* endOffset = strstr(textHeader, "end_header") + 9;
		uint32_t offsetHeader = endOffset - textHeader;
		free((void*) textHeader);

		file.seekg(offsetHeader);
		while (file.get() != 0x0a) {}

		for (uint32_t i = 0; i < elements.size(); i++) {
			elements[i]->read_binary(file, bigEndian);
		}

		PlyVertex lastV = vertElement->vertices.back();
		int i = 1;
	}

	// get the highest referenced index
	uint32_t highestRef = faceElement->get_highest();
	while (highestRef+1 < vertElement->vertices.size()) {
		vertElement->vertices.pop_back();
	}

	// calculate resulting model scale:
	vertElement->calc_bounds(boundMin, boundMax);

	// and center the mesh for better viewing:
	vertElement->offset(-vertElement->calc_avg());

	// if the vertex element didn't contain normal information, then compute them:
	if (!vertElement->has_type(PPT_NX)) {
		vertElement->construct_normals(faceElement->indices);
	}

	// construct vertex buffer from vertex element:
	vBuffer = vertElement->CreateVertexBuffer();

	// construct index buffer from index element:
	iBuffer = faceElement->CreateIndexBuffer();

	// construct vertex array object using both buffers and our common "ply" format:
	vao = new VAO(vBuffer, iBuffer);
	vao->EnableArrays(4);
	vao->Unbind();

	// done! delete elements list
	for (uint32_t i = 0; i < elements.size(); i++) {
		delete elements[i];
	}

	file.close();
}

void PlyModel::Render() {
	vao->Bind();
	glDrawElements(iBuffer->GetType(), iBuffer->GetCount(), GL_UNSIGNED_INT, (void*) 0);
}