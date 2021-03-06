
#include "SpecViz.h"
#include "PlyModel.h"

#include <fstream>

// MultiProjViewer is the main viewer that allows for multiple projections on the 3D mesh simultaneously. Projections
// should have been run through using CreateProjViewer, and then ideally had a Depth Field made for them

#define MAX_TEXTURES 16

class MultiProjViewer : public Viewer {
public:
	PixelShader* pShader;
	VertexShader* vShader;
	ShaderProgram* program;

	glm::vec3 rotation;
	glm::vec3 center;
	glm::mat4 objMatrix;
	glm::mat4 viewMatrix;
	glm::vec3 lightDirection;
	
	glm::mat4 projMatrix;

	glm::mat4 texMatrix[MAX_TEXTURES];
	Texture* projTexture[MAX_TEXTURES];
	uint32_t numTextures;

	float lightPitch;
	float lightYaw;
	float cameraDistance;
	float baseCameraDistance;
	float fieldOfView;

	PlyModel* model;

	MultiProjViewer(std::vector<const char*>& filenames);
	void MainLoop(float deltaTime);
	void NotifyKeyPress(const char* name);
	void NotifyMouseWheel(float amt, bool controlHeld);
	void NotifyMouseDrag(float x, float y, uint32_t button, bool controlHeld);
	virtual ~MultiProjViewer();
};

Viewer* CreateMultiProjViewer(std::vector<const char*>& filenames) {
	MultiProjViewer* newViewer = new MultiProjViewer(filenames);
	return newViewer;
}

MultiProjViewer::MultiProjViewer(std::vector<const char*>& filenames) {
	GLCHECK();
	assert(filenames.size() <= MAX_TEXTURES);
	
	// load up the projection files as created from CreateProjViewer
	char textureFile[MAX_TEXTURES][512];
	char modelFile[512];

	for (uint32_t i = 0; i < filenames.size(); i++) {
		const char* filename = filenames[i];
		std::fstream f(filename, std::ifstream::in);

		// texture and model file paths
		f >> textureFile[i];
		f >> modelFile;
	
		// projection matrix for this projection
		float* ref = &texMatrix[i][0].x;
		for (int32_t i = 0; i < 16; i++) {
			f >> *ref;
			ref++;
		}
	}

	// mesh rotation and lighting defaults
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

	// define the multiprojection shaders with the number of samplers we are using for this viewer instance
	char samplerDefine[256];
	sprintf_s(samplerDefine, "#define NUM_SAMPLERS %d\n", filenames.size());

	// compile the vertex and pixel shaders with this parameter
	pShader = new PixelShader("Shaders/multi_textured_light.pix", samplerDefine);
	vShader = new VertexShader("Shaders/multi_projected_vertex.vert", samplerDefine);
	program = new ShaderProgram(pShader, vShader);

	// load the singular model used for this setup
	model = new PlyModel(modelFile);
	
	// create a combined depth field / color texture for each instance. In the case that a depth field
	// texture has not been generated for a given projection, CreateFromFileCombined will fill the alpha
	// channel with white, making only normals the relevant weighting for that projection
	char depthTexture[512];
	for (uint32_t i = 0; i < filenames.size(); i++) {
		strcpy(depthTexture, filenames[i]);
		strcat(depthTexture, ".edgedist.png");
		projTexture[i] = Texture::CreateFromFileCombined(textureFile[i], depthTexture);
	}

	numTextures = filenames.size();
	fieldOfView = 30.0f;
	baseCameraDistance = glm::length(model->GetScale()) / 1.404f * 90.0f / fieldOfView;
	cameraDistance = baseCameraDistance;
}

void MultiProjViewer::MainLoop(float deltaTime) {
	GLCHECK();

	// set up z write/read
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	GLCHECK();

	// clear frame buffer
	glClearColor(0,0,0,1);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLCHECK();

	// bind program
	program->Bind();
	GLCHECK();
	
	// bind textures
	for (uint32_t i = 0; i < numTextures; i++) {
		projTexture[i]->Bind(i);
	}

	projMatrix = glm::infinitePerspective(fieldOfView * glm::pi<float>() / 180.0f, GetAspectRatio(), 0.01f);

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
	
	glDisable(GL_BLEND);

	GLCHECK();
	glUniformMatrix4fv(program->GetUniform("objMatrix"), 1, GL_FALSE, &objMatrix[0][0]);
	glUniformMatrix4fv(program->GetUniform("projMatrix"), 1, GL_FALSE, &projMatrix[0][0]);
	glUniformMatrix4fv(program->GetUniform("viewMatrix"), 1, GL_FALSE, &viewMatrix[0][0]);

	// set the texMatrix array with numTextures size
	glUniformMatrix4fv(program->GetUniform("texMatrix"), numTextures, GL_FALSE, &texMatrix[0][0][0]);
	glUniform3fv(program->GetUniform("lightDirection"), 1, &lightDirection.x);
	glUniform3fv(program->GetUniform("eyePosition"), 1, &eyePosition.x);
	glUniform1f(program->GetUniform("alpha"), 1.0f);

	// bind each sampler to the corresponding texture index
	GLint textureList[MAX_TEXTURES] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	};
	glUniform1iv(program->GetUniform("colorMap"), numTextures, textureList);

	GLCHECK();

	// draw our model
	model->Render();
	GLCHECK();
}

void MultiProjViewer::NotifyKeyPress(const char* name) {
	// has to be overridden since it is an abstract function but does nothing
}

void MultiProjViewer::NotifyMouseWheel(float amt, bool controlHeld) {
	// mouse wheel is used to determine camera distance (how the model is scaled in the perspective view)
	cameraDistance += baseCameraDistance * -0.005f * amt * (controlHeld ? 0.1f : 1.0f);
}

void MultiProjViewer::NotifyMouseDrag(float x, float y, uint32_t button, bool controlHeld) {
	const float p = glm::pi<float>();

	if (button == 2) {
		// right mouse click and up/down will rotate along the z axis
		rotation.z += y * 0.01f;
		if (rotation.z > p * 2.0f)
			rotation.z -= p * 2.0f;
		if (rotation.z < p * -2.0f)
			rotation.z += p * 2.0f;
	} else if (button == 1) {
		// middle click will offset (math here uses the current view direction to determine up / down )
		center.x += viewMatrix[0].x * x / 1000.0f * cameraDistance;
		center.y += viewMatrix[0].y * x / 1000.0f * cameraDistance;
		center.z += viewMatrix[0].z * x / 1000.0f * cameraDistance;
		center.x += viewMatrix[1].x * y / 1000.0f * cameraDistance;
		center.y += viewMatrix[1].y * y / 1000.0f * cameraDistance;
		center.z += viewMatrix[1].z * y / 1000.0f * cameraDistance;
	} else if (button == 0) {
		// left click will rotate about x and y (pitch and yaw respectively)
		rotation.x += x * 0.01f;
		rotation.y -= y * 0.01f;
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

MultiProjViewer::~MultiProjViewer() {
	// clean up 
	delete program;
	GLCHECK();
	delete pShader;
	GLCHECK();
	delete vShader;
	GLCHECK();
	delete model;
	GLCHECK();

	for (uint32_t i = 0; i < numTextures; i++) {
		delete projTexture[i];
	}
	GLCHECK();
	
	glClearColor(1,1,1,1);
	glClear(GL_COLOR_BUFFER_BIT);
}