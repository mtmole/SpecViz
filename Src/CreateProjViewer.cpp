
#include "SpecViz.h"
#include "PlyModel.h"

#include <fstream>

// CreateProjected is the viewer currently used for creating projection matrix with a semi-transparent mesh
// and the indicated image

class CreateProjected : public Viewer {
public:
	// shaders and programs used for rendering projected mesh and background texture
	PixelShader* modelPShader;
	VertexShader* modelVShader;
	ShaderProgram* modelProgram;

	PixelShader* texturePShader;
	VertexShader* textureVShader;
	ShaderProgram* textureProgram;

	// texture actually being projected on to
	Texture* ontoTexture;

	// projection matrix creation variables
	bool rotate;
	glm::vec3 rotation;
	glm::vec3 center;
	glm::mat4 objMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 projMatrix;
	glm::vec3 lightDirection;

	// light and camera settings
	float lightPitch;
	float lightYaw;
	float cameraDistance;
	float baseCameraDistance;
	float fieldOfView;

	// model of mesh being projected
	PlyModel* model;

	// these VAOs and Buffers are for background image ( to remain true to aspect )
	VAO* vao;
	IndexBuffer* iBuffer;
	VertexBuffer* vBuffer;

	// texture and model path names
	const char* texName;
	const char* modelName;

	CreateProjected(const char* texture, const char* model);
	void MainLoop(float deltaTime);
	void NotifyKeyPress(const char* name);
	void NotifyMouseWheel(float amt, bool controlHeld);
	void NotifyMouseDrag(float x, float y, uint32_t button, bool controlHeld);
	void Save(const char* toFile);
	virtual ~CreateProjected();
};

Viewer* CreateCreateProjViewer(const char* texture, const char* model) {
	CreateProjected* newViewer = new CreateProjected(texture, model);
	return newViewer;
}

CreateProjected::CreateProjected(const char* textureFile, const char* modelFile) {
	GLCHECK();

	// set up defaults for transform
	rotate = false;
	rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	center = glm::vec3(0.0f, 0.0f, 0.0f);

	lightPitch = 1.0f;
	lightYaw = 0.0f;
	model = NULL;

	int32_t major = 0;
	int32_t minor = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	printf("GL %d.%d", major, minor);

	// the model is rendered with a simple light and lit vertices but no texturing
	modelPShader = new PixelShader("Shaders/simple_light.pix");
	modelVShader = new VertexShader("Shaders/lit_vertex.vert");
	modelProgram = new ShaderProgram(modelPShader, modelVShader);

	// pass through shader for the background image
	texturePShader = new PixelShader("Shaders/simple_tex.pix");
	textureVShader = new VertexShader("Shaders/passthrough.vert");
	textureProgram = new ShaderProgram(texturePShader, textureVShader);

	// load up the texture from the given file name as full color
	ontoTexture = Texture::CreateFromFile(textureFile, GL_RGBA8);

	// load up the model to project onto from the given file name as a PLY model
	model = new PlyModel(modelFile);
	
	// this is currently hard coded and has to be adjusted based on the image source
	// TODO : grab field of view from image file info itself when available
	//        I've noticed that the file metadata for this does not follow a standard format
	//        however and varies by device source
	fieldOfView = 20.0f;

	// starting camera distance is a function of the model size and image field of view
	baseCameraDistance = glm::length(model->GetScale()) / 1.404f * 90.0f / fieldOfView;
	cameraDistance = baseCameraDistance;
	
	// create a basic quad vertex and index buffer for rendering the background image
	float aspect = ontoTexture->GetAspect();
	struct vPos {
		float pos[3];
		float uv[2];
	};
	vPos vData[] = {
		-1.0,-1,0, 0,0,
		 1.0,-1,0, 1,0,
		-1.0, 1,0, 0,1,
		 1.0, 1,0, 1,1,
	};
	vBuffer = new VertexBuffer(vData, sizeof(vData));

	uint32_t iData[] = {
		0,1,2,2,1,3
	};
	iBuffer = new IndexBuffer(iData, sizeof(iData), GL_TRIANGLES);

	vao = new VAO(vBuffer, iBuffer);
	vao->EnableArrays(2);

	texName = _strdup(textureFile);
	modelName = _strdup(modelFile);
}

void CreateProjected::MainLoop(float deltaTime) {
	GLCHECK();

	// set up z write/read
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	GLCHECK();

	// clear frame buffer
	glClearColor(0,0,0,1);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLCHECK();

	// render texture:
	
	// bind program
	textureProgram->Bind();
	GLCHECK();

	// maintain texture aspect by computing its necessary scale as a function of the viewport aspect
	glm::vec2 scale(1.0f, 1.0f);
	float texA = ontoTexture->GetAspect();
	float viewA = GetAspectRatio();
	if (texA > viewA) {
		scale.y = viewA / texA;
	} else {
		scale.x = texA / viewA;
	}

	GLCHECK();

	// set the computed scale and point the colorMap to sampler 0
	glUniform2fv(textureProgram->GetUniform("scale"), 1, &scale.x);
	glUniform1i(textureProgram->GetUniform("colorMap"), 0);
	GLCHECK();

	// bind texture to sampler 0
	ontoTexture->Bind(0);

	// draw our quad
	vao->Bind();
	glDrawElements(iBuffer->GetType(), iBuffer->GetCount(), GL_UNSIGNED_INT, (void*) 0);
	GLCHECK();
	

	// render model
	glDepthMask(GL_TRUE);
	modelProgram->Bind();
	GLCHECK();
	
	projMatrix = glm::infinitePerspective(fieldOfView * glm::pi<float>() / 180.0f, GetAspectRatio(), 0.01f);

	// create our view matrix based on the current rotation, offset, and camera distance
	glm::vec3 eyePosition = glm::vec3(cameraDistance, 0.0f, 0.0f);

	objMatrix = glm::mat4x4();
	objMatrix = glm::rotate(objMatrix, rotation.x, glm::vec3(0.0f, 1.0f, 0.0f));
	objMatrix = glm::rotate(objMatrix, rotation.y, glm::vec3(objMatrix[0][2], objMatrix[1][2], objMatrix[2][2]));
	objMatrix = glm::rotate(objMatrix, rotation.z, glm::vec3(objMatrix[0][0], objMatrix[1][0], objMatrix[2][0]));
	viewMatrix = glm::lookAt(
		eyePosition + center,
		glm::vec3(0,0,0) + center,
		glm::vec3(0,1,0)
	);

	float pitchVar = 1.0f - sin(lightPitch);
	lightDirection = glm::normalize(glm::vec3(cos(lightYaw) * pitchVar, sin(lightPitch), sin(lightYaw) * pitchVar));
	
	// we are rendering with alpha transparency so enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLCHECK();
	
	// set computed program matrices and alpha amount
	glUniformMatrix4fv(modelProgram->GetUniform("objMatrix"), 1, GL_FALSE, &objMatrix[0][0]);
	glUniformMatrix4fv(modelProgram->GetUniform("projMatrix"), 1, GL_FALSE, &projMatrix[0][0]);
	glUniformMatrix4fv(modelProgram->GetUniform("viewMatrix"), 1, GL_FALSE, &viewMatrix[0][0]);
	glUniform3fv(modelProgram->GetUniform("lightDirection"), 1, &lightDirection.x);
	glUniform3fv(modelProgram->GetUniform("eyePosition"), 1, &eyePosition.x);
	glUniform1f(modelProgram->GetUniform("alpha"), 0.5f);
	GLCHECK();

	// draw our model
	model->Render();
	GLCHECK();
}

void CreateProjected::NotifyKeyPress(const char* name) {
	// deprecated (model used to auto rotate before we have manual control)
	if (!strcmp(name, "space")) {
		rotate = !rotate;
		return;
	}
}

void CreateProjected::NotifyMouseWheel(float amt, bool controlHeld) {
	// mouse wheel is used to determine camera distance (how the model is scaled in the perspective view)
	cameraDistance += baseCameraDistance * -0.005f * amt * (controlHeld ? 0.1f : 1.0f);
}

void CreateProjected::NotifyMouseDrag(float x, float y, uint32_t button, bool controlHeld) {
	const float p = glm::pi<float>();

	// when control is held adjustments are much more fine tuned
	float controlScale = controlHeld ? 0.01f : 1.0f;

	if (button == 2) {
		// right mouse click and up/down will rotate along the z axis
		rotation.z += y * 0.01f * controlScale;
		if (rotation.z > p * 2.0f)
			rotation.z -= p * 2.0f;
		if (rotation.z < p * -2.0f)
			rotation.z += p * 2.0f;
		//fieldOfView += x * 0.01f * controlScale;
	} else if (button == 1) {
		// middle click will offset (math here uses the current view direction to determine up / down )
		center.x += viewMatrix[0].x * x / 1000.0f * cameraDistance * controlScale;
		center.y += viewMatrix[0].y * x / 1000.0f * cameraDistance * controlScale;
		center.z += viewMatrix[0].z * x / 1000.0f * cameraDistance * controlScale;
		center.x += viewMatrix[1].x * y / 1000.0f * cameraDistance * controlScale;
		center.y += viewMatrix[1].y * y / 1000.0f * cameraDistance * controlScale;
		center.z += viewMatrix[1].z * y / 1000.0f * cameraDistance * controlScale;
	} else if (button == 0) {
		// left click will rotate about x and y (pitch and yaw respectively)
		rotation.x += x * 0.01f * controlScale;
		rotation.y -= y * 0.01f * controlScale;
		if (rotation.x > p * 2.0f)
			rotation.x -= p * 2.0f;
		if (rotation.x < p * -2.0f)
			rotation.x += p * 2.0f;
		if (rotation.y > p * 2.0f)
			rotation.y -= p * 2.0f;
		if (rotation.y < p * -2.0f)
			rotation.y += p * 2.0f;
	}
}

void CreateProjected::Save(const char* toFile) {
	// a "projection file" is the texture and model file paths used for projection, 
	// as well as the final view matrix (with the scale aspect correction from the background image applied)
	std::fstream file(toFile, std::ofstream::out);

	file << texName << "\n";
	file << modelName << "\n";

	// apply our scale correction
	glm::vec2 scale(1.0f, 1.0f);
	float texA = ontoTexture->GetAspect();
	float viewA = GetAspectRatio();
	if (texA > viewA) {
		scale.y = viewA / texA;
	} else {
		scale.x = texA / viewA;
	}
	projMatrix[0] /= scale.x;
	projMatrix[1] /= scale.y;
	
	// create final matrix:
	glm::mat4x4 finalMat = projMatrix * (viewMatrix * objMatrix);

	float* ref = &finalMat[0].x;
	for (uint32_t i = 0; i < 16; i++) {
		file << *ref << "\n";
		ref++;
	}

	file.close();
}

CreateProjected::~CreateProjected() {
	// clean up
	delete modelProgram;
	GLCHECK();
	delete modelPShader;
	GLCHECK();
	delete modelVShader;
	GLCHECK();
	delete textureProgram;
	GLCHECK();
	delete texturePShader;
	GLCHECK();
	delete textureVShader;
	GLCHECK();
	delete model;
	GLCHECK();

	free((void*) texName);
	free((void*) modelName);
	
	glClearColor(1,1,1,1);
	glClear(GL_COLOR_BUFFER_BIT);
}
