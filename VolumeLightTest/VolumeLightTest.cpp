
#include "TheGame.h"

//Note we can write to 1 buffer if we're in 1 thread only. 2 for 2
//One Vao bound, and different VBO's for different vertex elements


GLuint Grass_Texture, Brick_Texture, Sky_Texture;
std::vector<Model> v_models;

static glm::vec4 redcolor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
static glm::vec3 camera_distance;
static glm::vec3 camera_origin;
static glm::vec3 right_vector;

static glm::vec3 next_position;
static glm::vec2 newMousepos;
static glm::vec2 oldMousepos;

static glm::vec4 ambient_light;

glm::mat4 mProjection, p1Model,p2Model, mvp, testModel;
glm::mat4 ViewMatrix;

bool Running = false;
static bool window_is_fullscreen = false;

SDL_Event event;
glm::vec2 mouseDelta;
glm::mat4 quadModel = glm::mat4(1.0f);
static glm::vec3 camera_speed;
static glm::vec3 volume_camera_origin = glm::vec3(0.0, 0.0, 1.0);
static glm::vec3 volume_camera_distance;
struct dir_light
{
	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 color;
	glm::mat4 modelMatrix;
};
dir_light DirectionalLight;
dir_light pointLight, pointLight2;

GLuint programID;
GLuint VAO;
GLuint VBO_position, VBO_UV, VBO_color, VBO_normals;
//GLuint LineVBO, DebugSquareVBO;
GLint mvp_location, vpos_location, vcol_location, sampler_location;
GLint model_matrix_uniform_location, eye_position_uniform_location;
GLint model_view_matrix_uniform_location;

GLuint sampler_state = 0;
static void PrintFPS(ace_timer *thetimer)
{
	thetimer->FPS++;
	thetimer->global_newtime = SDL_GetTicks();
	if (thetimer->global_newtime - thetimer->global_oldtime > 1000.0f)
	{
		printf("FPS : %d \n ", thetimer->FPS);
		thetimer->global_oldtime = thetimer->global_newtime;
		thetimer->FPS = 0;
	}
}
static void ForceFramerate(ace_timer *thetimer)
{
	thetimer->newtime = SDL_GetTicks();
	thetimer->MSElapsed = thetimer->newtime - thetimer->oldtime;
	if (thetimer->MSElapsed  < thetimer->DesiredMiliseconds)
	{
		long timetosleep = (long)(thetimer->DesiredMiliseconds - thetimer->MSElapsed);
		SDL_Delay(timetosleep);

		while (thetimer->MSElapsed < thetimer->DesiredMiliseconds)
		{
			thetimer->newtime = SDL_GetTicks();
			thetimer->MSElapsed = thetimer->newtime - thetimer->oldtime;
		}

	}
	PrintFPS(thetimer);
	
	thetimer->oldtime = thetimer->newtime;
}
//loadMaterial(char* tex_path)

bool loadOBJFile(char* path, Model *model, char* tex_path)
{

	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;

	FILE* file = fopen(path, "r");
	if (file == NULL)
	{
		//LOGGING
		printf("cANNOT OPEN FILE");
		return false;
	}
	while (1)
	{
		char header[255];
		//read a word
		int result = fscanf(file, "%s", header);
		if (result == EOF)break;

		//vertices
		//if the first word of the line is "v" then the next 3 %f's are the vertices
		if (strcmp(header, "v") == 0)
		{
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		// UV
		else if (strcmp(header, "vt") == 0)
		{
			glm::vec2 uvs;
			fscanf(file, "%f %f\n", &uvs.x, &uvs.y);
			temp_uvs.push_back(uvs);
		}
		//NORMALS
		else if (strcmp(header, "vn") == 0)
		{
			glm::vec3 norms;
			fscanf(file, "%f %f %f\n", &norms.x, &norms.y, &norms.z);
			temp_normals.push_back(norms);
		}
		//INDICES v/uv/n
		else if (strcmp(header, "f") == 0)
		{
			std::string vert1, vert2, vert3;
			uint32_t vertindex[3], normalindex[3], uvindex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
				&vertindex[0], &uvindex[0], &normalindex[0],
				&vertindex[1], &uvindex[1], &normalindex[1],
				&vertindex[2], &uvindex[2], &normalindex[2]);
			if (matches != 9)
			{
				printf("we cant read the obj. something went wrong in parsing the f's ");
				return false;
			}
			//the indices are the same as the vertex


			model->index.vertexindices.push_back(vertindex[0]);
			model->index.vertexindices.push_back(vertindex[1]);
			model->index.vertexindices.push_back(vertindex[2]);
			model->index.uvindices.push_back(uvindex[0]);
			model->index.uvindices.push_back(uvindex[1]);
			model->index.uvindices.push_back(uvindex[2]);
			model->index.normalindices.push_back(normalindex[0]);
			model->index.normalindices.push_back(normalindex[1]);
			model->index.normalindices.push_back(normalindex[2]);
		}
	}
	//Process everything
	//vertices
	for (int i = 0; i < model->index.vertexindices.size(); i++)
	{
		uint32_t index = model->index.vertexindices[i];
		glm::vec3 vertex = temp_vertices[index - 1];
		//fill the output
		model->vert.position.push_back(vertex);
	}

	//uv

	for (int i = 0; i < model->index.uvindices.size(); i++)
	{
		uint32_t index = model->index.uvindices[i];
		glm::vec2 uvs = temp_uvs[index - 1];
		//fill the output
		model->vert.UV.push_back(uvs);
	}

	//normals
	for (int i = 0; i < model->index.normalindices.size(); i++)
	{
		uint32_t index = model->index.normalindices[i];
		glm::vec3 norms = temp_normals[index - 1];
		//fill the output
		model->vert.Normals.push_back(norms);
	}

	return true;
}
GLuint LoadShaders(const char * vertex_file_path, const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if (VertexShaderStream.is_open()) {
		std::string Line = "";
		while (getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}
	else {
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		getchar();
		return 0;
	}


	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if (FragmentShaderStream.is_open()) {
		std::string Line = "";
		while (getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;


	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}



	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}



	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}


	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}


GLuint CreateTexture(int width, int height, bool depth = false)
{
	GLuint TextureID;
	glGenTextures(1, &TextureID);
	glBindTexture(GL_TEXTURE_2D, TextureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, (!depth ? GL_RGBA8 : GL_DEPTH_COMPONENT), width, height, 0, depth ? GL_DEPTH_COMPONENT : GL_RGBA, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);
	return TextureID;
}

GLuint LoadTextures(char *file_name)
{
	int ImageX,
		ImageY,
		ImageComponents;

	GLubyte *ImageData = stbi_load(file_name, &ImageX, &ImageY, &ImageComponents, 4);
	//Get the Texture
	GLuint Texture;
	glGenTextures(1, &Texture);
	glBindTexture(GL_TEXTURE_2D, Texture);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, ImageX, ImageY, 0, GL_RGBA, GL_UNSIGNED_BYTE, ImageData);
	
	//for not using border colors GL_CLAMP uses border colors
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(ImageData);
	
	return Texture;
}
void BindTexture(GLenum TextureUnit, GLuint Texture, GLuint *Sampler)
{
	glActiveTexture(GL_TEXTURE0 +TextureUnit);
	glBindTexture(GL_TEXTURE_2D, Texture);
	glBindSampler(TextureUnit, *Sampler);
}
void releaseTextures(GLuint *Texture, GLuint *Sampler)
{
	glDeleteTextures(1, Texture);
	glDeleteSamplers(1, Sampler);

}
static void SetupQuad(Model *Entity)
{
	//Models are meant for different VBO's


	//they are like this so the square can rotate around its center
	glm::vec3 vertices[4] =
	{
		glm::vec3(-1.0f,-1.0f, 0.0f),
		glm::vec3(1.0f, -1.0f, 0.0f),
		glm::vec3(-1.0f, 1.0f, 0.0f),
		glm::vec3(1.0f, 1.0f, 0.0f),


	};

	Entity->vert.position.push_back(vertices[0]);
	Entity->vert.position.push_back(vertices[1]);
	Entity->vert.position.push_back(vertices[2]);
	Entity->vert.position.push_back(vertices[3]);


	Entity->vert.UV.push_back(glm::vec2(0.0f, 0.0f)); // Lower-left corner  
	Entity->vert.UV.push_back(glm::vec2(1.0f, 0.0f)); // Lower-right corner
	Entity->vert.UV.push_back(glm::vec2(0.0f, 1.0f));// Upper-left corner
	Entity->vert.UV.push_back(glm::vec2(1.0f, 1.0f)); // Upper-right corner


													  
	glm::vec3 norm1 = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 norm2 = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 norm3 = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 norm4 = glm::vec3(0.0f, 0.0f, 1.0f);


	Entity->vert.Normals.push_back(norm1);
	Entity->vert.Normals.push_back(norm2);
	Entity->vert.Normals.push_back(norm3);
	Entity->vert.Normals.push_back(norm4);





}
static void SetupSquare(Model *Entity)
{
	//Models are meant for different VBO's


	//they are like this so the square can rotate around its center
	glm::vec3 vertices[4] =
	{
		glm::vec3(-Entity->Width / 2, -Entity->Width / 2, 0.0f), 
		glm::vec3(Entity->Width / 2, -Entity->Width / 2, 0.0f),     
		glm::vec3(-Entity->Width / 2,  Entity->Width / 2, 0.0f),   
		glm::vec3(Entity->Width / 2,  Entity->Width / 2, 0.0f),


	};

	Entity->vert.position.push_back(vertices[0]);
	Entity->vert.position.push_back(vertices[1]);
	Entity->vert.position.push_back(vertices[2]);
	Entity->vert.position.push_back(vertices[3]);


	Entity->vert.UV.push_back(glm::vec2(0.0f, 0.0f)); // Lower-left corner  
	Entity->vert.UV.push_back(glm::vec2(1.0f, 0.0f)); // Lower-right corner
	Entity->vert.UV.push_back(glm::vec2(0.0f, 1.0f));// Upper-left corner
	Entity->vert.UV.push_back(glm::vec2(1.0f, 1.0f)); // Upper-right corner


	//Normals
	glm::vec3 norm1 = glm::vec3(0.0f,0.0f,1.0f);
	glm::vec3 norm2 = glm::vec3(0.0f,0.0f,1.0f);
	glm::vec3 norm3 = glm::vec3(0.0f,0.0f,1.0f);
	glm::vec3 norm4 = glm::vec3(0.0f,0.0f,1.0f);

	
	Entity->vert.Normals.push_back(norm1); 
	Entity->vert.Normals.push_back(norm2);
	Entity->vert.Normals.push_back(norm3);
	Entity->vert.Normals.push_back(norm4);



	

}
static void DrawSquare(Model *Entity)
{
	//Position
	glBindBuffer(GL_ARRAY_BUFFER, VBO_position);

	glEnableVertexAttribArray(0);

	glBufferData(GL_ARRAY_BUFFER, //full buffer size
		Entity->vert.position.size() * sizeof(glm::vec3), &Entity->vert.position.front(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);

	//Textures

	glBindBuffer(GL_ARRAY_BUFFER, VBO_UV);

	glEnableVertexAttribArray(1);

	glBufferData(GL_ARRAY_BUFFER, //full buffer size
		Entity->vert.UV.size() * sizeof(glm::vec2), &Entity->vert.UV.front(), GL_STATIC_DRAW);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void *)0);

	//Normals
	glBindBuffer(GL_ARRAY_BUFFER, VBO_normals);
	
	glEnableVertexAttribArray(2);
	
	glBufferData(GL_ARRAY_BUFFER, //full buffer size
		Entity->vert.Normals.size() * sizeof(glm::vec3), &Entity->vert.Normals.front(), GL_STATIC_DRAW);
	
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);


}
static void drawModel(Model *theModel )
{
	
	//Models are meant to be in seperate buffers?.
	glBindBuffer(GL_ARRAY_BUFFER,VBO_position);

	glEnableVertexAttribArray(0);

	glBufferData(GL_ARRAY_BUFFER, //full buffer size
		theModel->vert.position.size() * sizeof(glm::vec3), &theModel->vert.position.front(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);

	//Textures

	glBindBuffer(GL_ARRAY_BUFFER, VBO_UV);

	glEnableVertexAttribArray(1);

	glBufferData(GL_ARRAY_BUFFER, //full buffer size
		theModel->vert.UV.size() * sizeof(glm::vec2), &theModel->vert.UV.front(), GL_STATIC_DRAW);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void *)0);

	//Normals
	glBindBuffer(GL_ARRAY_BUFFER, VBO_normals);

	glEnableVertexAttribArray(2);

	glBufferData(GL_ARRAY_BUFFER, //full buffer size
		theModel->vert.Normals.size() * sizeof(glm::vec3), &theModel->vert.Normals.front(), GL_STATIC_DRAW);

	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);

}
static void ClearModel(Model *theModel)
{
	theModel->vert.position.clear();
	theModel->vert.Normals.clear();
	theModel->vert.UV.clear();
	theModel->index.uvindices.clear();
	theModel->index.vertexindices.clear();
	theModel->index.normalindices.clear();
}

GLuint FBO, FBO2, FBO3;
GLuint depthTexture, shadowMap, VolumeLightTexture;
GLfloat shadowMapWidth, shadowMapHeight;
GLuint SimpleProgram, VolumeLightProgram;
glm::mat4 shadowMatrix, depthMatrix;
glm::mat4 ShadowViewMatrix;
glm::mat4 ShadowProjMatrix;
glm::mat4 VolumeLightProjMatrix;
static void UpdateDepthMatrix(GLuint program, glm::mat4 modelMatrix)
{
	depthMatrix = mProjection*ViewMatrix*modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(SimpleProgram, "LightsMVP"), 1, GL_FALSE, &depthMatrix[0][0]);
}
static void UpdateShadowMatrix(GLuint program, glm::mat4 modelMatrix)
{
	glm::mat4 biasMatrix;
	biasMatrix[0][0] = 0.5; biasMatrix[0][1] = 0.0; biasMatrix[0][2] = 0.0; biasMatrix[0][3] = 0.0;
	biasMatrix[1][0] = 0.0; biasMatrix[1][1] = 0.5; biasMatrix[1][2] = 0.0; biasMatrix[1][3] = 0.0;
	biasMatrix[2][0] = 0.0; biasMatrix[2][1] = 0.0; biasMatrix[2][2] = 0.5; biasMatrix[2][3] = 0.0;
	biasMatrix[3][0] = 0.5; biasMatrix[3][1] = 0.5; biasMatrix[3][2] = 0.5; biasMatrix[3][3] = 1.0;

	ShadowViewMatrix = glm::lookAt(
		DirectionalLight.position,
		DirectionalLight.direction - DirectionalLight.position , // and looks at the origin
		glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
	);

	shadowMatrix = ShadowProjMatrix*ShadowViewMatrix*modelMatrix;
	glm::mat4 normalMatrix = biasMatrix*ShadowProjMatrix*ShadowViewMatrix*modelMatrix;
	//Names should never be the SAME as the one in the other shader.....
	glUniformMatrix4fv(glGetUniformLocation(program, "LightsMVP"), 1, GL_FALSE, &shadowMatrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(program, "lightModelViewProjectionMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);
}
static void UpdateVolumeLight(GLuint program,glm::mat4 modelMatrix)
{
	
	//IF WE WANT THE SCREEN TO BE DRAWN WITH 1 QUBE ON IT PERFECTLY
	glm::mat4 biasMatrix;
	biasMatrix[0][0] = 0.5; biasMatrix[0][1] = 0.0; biasMatrix[0][2] = 0.0; biasMatrix[0][3] = 0.0;
	biasMatrix[1][0] = 0.0; biasMatrix[1][1] = 0.5; biasMatrix[1][2] = 0.0; biasMatrix[1][3] = 0.0;
	biasMatrix[2][0] = 0.0; biasMatrix[2][1] = 0.0; biasMatrix[2][2] = 0.5; biasMatrix[2][3] = 0.0;
	biasMatrix[3][0] = 0.5; biasMatrix[3][1] = 0.5; biasMatrix[3][2] = 0.5; biasMatrix[3][3] = 1.0;

	glm::mat4 ViewInv = glm::inverse(ViewMatrix);
	glm::mat4 ProjectionInv = glm::inverse(mProjection);
	glm::mat4 ViewProjection = mProjection*ViewMatrix;
	glm::mat4 ViewProjectionInv = glm::inverse(ViewProjectionInv);

	glm::mat4 ShadowViewProjection = biasMatrix*ShadowProjMatrix * ShadowViewMatrix;
	
	
	glUniform3fv(glGetUniformLocation(program, "EyePosition"), 1, &camera_distance[0]);
	glUniformMatrix4fv(glGetUniformLocation(program, "ViewProjectionInv"), 1, GL_FALSE, &ViewProjectionInv[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(program, "ViewInv"), 1, GL_FALSE, &ViewInv[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(program, "ViewMatrix"), 1, GL_FALSE, &ViewMatrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(program, "ProjectionInv"), 1, GL_FALSE, &ProjectionInv[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(program, "ShadowViewProjection"), 1, GL_FALSE, &ShadowViewProjection[0][0]);

}
static void GetVolumeLightMap(glm::mat4 modelMatrix, game_state *GameState)
{
	glViewport(0, 0, WindowWidth, WindowHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO3);
	glEnable(GL_DEPTH_TEST);
	
//	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, VolumeLightTexture, 0);
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(VolumeLightProgram);
	
	
	//bind the maps
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, shadowMap);
	glUniform1i(glGetUniformLocation(VolumeLightProgram, "shadowMap"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glUniform1i(glGetUniformLocation(VolumeLightProgram, "depthTexture"), 1);


	//We only update the fullscreen quad
	DrawSquare(&GameState->VolumeLightQuad);
	modelMatrix = glm::mat4(1.0f);	
	UpdateVolumeLight(VolumeLightProgram,modelMatrix);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->VolumeLightQuad.vert.position.size());

   // glDisable(GL_BLEND);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
static void GetDepthMap(glm::mat4 modelMatrix, game_state *GameState)
{
	glViewport(0, 0, WindowWidth, WindowHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO2);
	glEnable(GL_DEPTH_TEST);
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
	 glUseProgram(SimpleProgram);
	glClear(GL_DEPTH_BUFFER_BIT);



	//one model
	drawModel(&GameState->Model1);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, pointLight.position);
	UpdateDepthMatrix(SimpleProgram, modelMatrix);
	glDrawArrays(GL_TRIANGLES, 0, GameState->Model1.vert.position.size());

	drawModel(&GameState->Model1);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, pointLight2.position);
	UpdateDepthMatrix(SimpleProgram, modelMatrix);
	glDrawArrays(GL_TRIANGLES, 0, GameState->Model1.vert.position.size());

	//Plane
	DrawSquare(&GameState->Plane);
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, GameState->Plane.WorldPos);// glm::vec3(50, 0, -5));
	UpdateDepthMatrix(SimpleProgram, modelMatrix);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());

	//Draw second Plane
	//BOTTOM


	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, GameState->Plane.WorldPos + glm::vec3(1.0f, -15.0f, 40.0f));
	modelMatrix = glm::rotate(modelMatrix, 30.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	UpdateDepthMatrix(SimpleProgram, modelMatrix);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());


	//Draw third Plane
	//RIGHT SECOND
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, GameState->Plane.WorldPos + glm::vec3(-52.0f, 20.0f, 40.0f));
	modelMatrix = glm::rotate(modelMatrix, 60.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	UpdateDepthMatrix(SimpleProgram, modelMatrix);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());

	//Draw fourth Plane
	//RIGHT FIRST
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, GameState->Plane.WorldPos + glm::vec3(50.0f, 20.0f, 40.0f));
	modelMatrix = glm::rotate(modelMatrix, 60.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	UpdateDepthMatrix(SimpleProgram, modelMatrix);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());

	//Draw fifth Plane
	//TOP 
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, GameState->Plane.WorldPos + glm::vec3(1.0f, 20.0f, 40.0f));
	modelMatrix = glm::rotate(modelMatrix, -30.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	UpdateDepthMatrix(SimpleProgram, modelMatrix);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());

	//Draw sixth Plane
	//BACK
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, GameState->Plane.WorldPos + glm::vec3(-50.0f, 20.0f, 40.0f));
	modelMatrix = glm::rotate(modelMatrix, -30.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	UpdateDepthMatrix(SimpleProgram, modelMatrix);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());

	//Draw seventh Plane
	//FRONT
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, GameState->Plane.WorldPos + glm::vec3(50.0f, 20.0f, 40.0f));
	modelMatrix = glm::rotate(modelMatrix, 30.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	UpdateDepthMatrix(SimpleProgram, modelMatrix);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
}
static void GetShadowMap(glm::mat4 modelMatrix, game_state *GameState)
{
	
	
   // ShadowProjMatrix = glm::perspective(glm::radians(60.0f), (float)shadowMapWidth / (float)shadowMapHeight, 0.1f, 1000.0f);
	float ratio = 8.0f;
	ShadowProjMatrix = glm::ortho(-(float)shadowMapWidth / (16.0f*ratio), (float)shadowMapWidth / (16.0f*ratio), -(float)shadowMapHeight / (9.0f*ratio), (float)shadowMapHeight / (9.0f*ratio), -1.1f, 1000.0f);
	float volume_ratio = 8.0f;
	VolumeLightProjMatrix = glm::ortho(-(float)WindowWidth / (16.0f*ratio), (float)WindowWidth / (16.0f*ratio), -(float)WindowHeight / (9.0f*ratio), (float)WindowHeight / (9.0f*ratio), 0.1f, 100.0f);
	//VolumeLightProjMatrix = glm::perspective(glm::radians(60.0f), (float)(WindowWidth/4) / (float)(WindowHeight/4), 0.1f, 1000.0f);

	glViewport(0, 0, shadowMapWidth, shadowMapHeight);

	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glCullFace(GL_FRONT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(SimpleProgram);
	glClear(GL_DEPTH_BUFFER_BIT);
	 //one model
	 drawModel(&GameState->Model1);

	 modelMatrix = glm::mat4(1.0f);
	 modelMatrix = glm::translate(modelMatrix, pointLight.position);
	 UpdateShadowMatrix(SimpleProgram, modelMatrix);
	 glDrawArrays(GL_TRIANGLES, 0, GameState->Model1.vert.position.size());
	
	 //one model
	 drawModel(&GameState->Model1);

	 modelMatrix = glm::mat4(1.0f);
	 modelMatrix = glm::translate(modelMatrix, pointLight2.position);
	 UpdateShadowMatrix(SimpleProgram, modelMatrix);
	 glDrawArrays(GL_TRIANGLES, 0, GameState->Model1.vert.position.size());
	
	 //Plane
	 DrawSquare(&GameState->Plane);
	 modelMatrix = glm::mat4(1.0f);
	 modelMatrix = glm::translate(modelMatrix, GameState->Plane.WorldPos);// glm::vec3(50, 0, -5));
	 UpdateShadowMatrix(SimpleProgram, modelMatrix);
	 glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());

	 //Draw second Plane
	 //BOTTOM
	 

	 modelMatrix = glm::mat4(1.0f);
	 modelMatrix = glm::translate(modelMatrix, GameState->Plane.WorldPos + glm::vec3(1.0f, -15.0f, 40.0f));
	 modelMatrix = glm::rotate(modelMatrix, 30.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	 UpdateShadowMatrix(SimpleProgram, modelMatrix);
	 glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());


	 //Draw third Plane
	 //RIGHT SECOND
	 modelMatrix = glm::mat4(1.0f);
	 modelMatrix = glm::translate(modelMatrix, GameState->Plane.WorldPos + glm::vec3(-52.0f, 20.0f, 40.0f));
	 modelMatrix = glm::rotate(modelMatrix, 60.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	 UpdateShadowMatrix(SimpleProgram, modelMatrix);
	 glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());

	 //Draw fourth Plane
	 //RIGHT FIRST
	 modelMatrix = glm::mat4(1.0f);
	 modelMatrix = glm::translate(modelMatrix, GameState->Plane.WorldPos + glm::vec3(50.0f, 20.0f, 40.0f));
	 modelMatrix = glm::rotate(modelMatrix, 60.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	 UpdateShadowMatrix(SimpleProgram, modelMatrix);
	 glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());

	 //Draw fifth Plane
	 //TOP 
	 modelMatrix = glm::mat4(1.0f);
	 modelMatrix = glm::translate(modelMatrix, GameState->Plane.WorldPos + glm::vec3(1.0f, 20.0f, 40.0f));
	 modelMatrix = glm::rotate(modelMatrix, -30.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	 UpdateShadowMatrix(SimpleProgram, modelMatrix);
	 glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());

	 //Draw sixth Plane
	 //BACK
	 modelMatrix = glm::mat4(1.0f);
	 modelMatrix = glm::translate(modelMatrix, GameState->Plane.WorldPos + glm::vec3(-50.0f, 20.0f, 40.0f));
	 modelMatrix = glm::rotate(modelMatrix, -30.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	 UpdateShadowMatrix(SimpleProgram, modelMatrix);
	 glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());

	 //Draw seventh Plane
	 //FRONT
	 modelMatrix = glm::mat4(1.0f);
	 modelMatrix = glm::translate(modelMatrix, GameState->Plane.WorldPos + glm::vec3(50.0f, 20.0f, 40.0f));
	 modelMatrix = glm::rotate(modelMatrix, 30.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	 UpdateShadowMatrix(SimpleProgram, modelMatrix);
	 glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());


	 //Rest of the models
	 //DRAW FULLSCREEN QUAD

	// DrawSquare(&GameState->VolumeLightQuad);


	// modelMatrix = glm::mat4(1.0);
	 //put it at a random pos so it won't block it in the shadow map 
	// modelMatrix = glm::translate(modelMatrix, GameState->Plane.WorldPos + glm::vec3(50.0f, 20.0f, 4000000.0f));
	// UpdateShadowMatrix(SimpleProgram, modelMatrix);
	// glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->VolumeLightQuad.vert.position.size());
	

	 //End
	 
	 glBindFramebuffer(GL_FRAMEBUFFER, 0);
	 glDisable(GL_CULL_FACE);
	 glViewport(0, 0, WindowWidth, WindowHeight);
	 //glCullFace(GL_BACK);
}
static void InitiateGame(game_state *TheGame)
{
	stbi_set_flip_vertically_on_load(1);
	//Program ID 

	programID = LoadShaders("shader.vert","shader.frag");
	SimpleProgram = LoadShaders("simpleShader.vert", "simpleShader.frag");
	VolumeLightProgram = LoadShaders("VolumeLight.vert", "VolumeLight.frag");

	shadowMapWidth = WindowWidth * 4;
	shadowMapHeight = WindowHeight * 4;

	
	shadowMap = CreateTexture(shadowMapWidth, shadowMapHeight, true);
	
	__glewGenFramebuffers(1, &FBO);
	__glewBindFramebuffer(GL_FRAMEBUFFER, FBO);
	__glewFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
	
	int i = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (i != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("Framebuffer is not OK, status=", i);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	depthTexture = CreateTexture(WindowWidth, WindowHeight, true);
	
	__glewGenFramebuffers(1, &FBO2);
	__glewBindFramebuffer(GL_FRAMEBUFFER, FBO2);
	__glewFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
	
	//480/270
	VolumeLightTexture = CreateTexture(WindowWidth/4, WindowHeight/4,false);
	__glewGenFramebuffers(1, &FBO3);
	__glewBindFramebuffer(GL_FRAMEBUFFER, FBO3);
	__glewFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, VolumeLightTexture, 0);

	i = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (i != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("Framebuffer is not OK, status=", i);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//VAOS and VBOS
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO_position);
	glGenBuffers(1, &VBO_color);
	glGenBuffers(1, &VBO_UV);
	glGenBuffers(1, &VBO_normals);


	//GET THE LOCATION OF THE VECTORS AND SHIT
	mvp_location = glGetUniformLocation(programID, "MVP");
	vcol_location = glGetUniformLocation(programID, "vColor");
	sampler_location = glGetUniformLocation(programID, "gSampler");

	newMousepos = glm::vec2(0, 0);
	oldMousepos = newMousepos;
	
	//Projection Matrix
	//mProjection = glm::perspective(glm::radians(60.0f), (float)WindowWidth / (float)WindowHeight, 0.0f, 10.0f);
//	mat4 unproj = 
	
	

	//setup camera
	camera_speed = glm::vec3(0.1f, 0.1f, 0.1f);
	camera_distance = glm::vec3(0, 0, 10); // To rotate around origin
	volume_camera_distance = glm::vec3(0, 0, 10);
	camera_origin = glm::vec3(0, 0, 1); // To move left and right with camera
	
	 DirectionalLight = {};
	DirectionalLight.position = glm::vec3(0.0f, 0.0f, 40.0f);
	DirectionalLight.direction = glm::vec3(0.0f, 0.0f, -1.0f);
	DirectionalLight.color = glm::vec3(0.8f, 0.5f, 0.0f);
	DirectionalLight.modelMatrix = glm::mat4(1.0f);
	pointLight = {};
	pointLight.position = glm::vec3(0, 0, 20.0f);
	pointLight.color = glm::vec3(0.0f, 1.0f, 1.0f);
	DirectionalLight.direction = glm::normalize(pointLight.position);

	pointLight2 = {};
	pointLight2.position = glm::vec3(0, 4, 20.0f);
	pointLight2.color = glm::vec3(1.0f, 1.0f, 1.0f);

	
	
	//initiate model
	//loadOBJFile("models/cube.obj", &TheGame->Model1);
    loadOBJFile("models/sphere.obj", &TheGame->Model1, "Textures2/blueice.jpg");
	TheGame->Model1.Texture = LoadTextures("Textures2/white.jpg");
	
	Grass_Texture = LoadTextures("Textures2/grass.jpg");
	Brick_Texture = LoadTextures("Textures2/stone_wall.jpg");
	
	TheGame->Plane.WorldPos = glm::vec3(0, 0, 0);
	TheGame->Plane.Width = 100;
	
	
	SetupSquare(&TheGame->Plane);
	TheGame->VolumeLightQuad.WorldPos = glm::vec3(0.0f,0.0f,0.0f);
		


	SetupQuad(&TheGame->VolumeLightQuad);

	TheGame->Model1.WorldPos = glm::vec3(1, 1, 0);
	v_models.push_back(TheGame->Model1);
	

	TheGame->Model1.WorldPos = glm::vec3(15.0f, 0.0f, 0.0f);
	
}


static void CheckMovement(game_input *Input)
{


#if 1
		if (Input->Controllers[0].CAMERA_STRAFE_LEFT.isDown)
		{
			camera_distance += (camera_speed * right_vector);
			volume_camera_distance += (camera_speed * right_vector);
		}
		
		if (Input->Controllers[0].CAMERA_STRAFE_RIGHT.isDown)
		{
			camera_distance += (-camera_speed * right_vector);
			volume_camera_distance += (-camera_speed * right_vector);
		}
		//Note: the volume_camera_distance is with the volume_camera_origin 
		//IMPORNTANT: THIS FIXES THE LOCATION OF THE VOLUME LIGHT AND ITS ROTATION 
		//Since it's going backwards in y.. 
		//it's something with the Y axis and I didn't have time to fix it.
		if (Input->Controllers[0].CAMERA_FORWARD.isDown)
		{
			camera_distance += (-camera_speed * camera_origin);
			volume_camera_distance += (-camera_speed * volume_camera_origin);
		}

		if (Input->Controllers[0].CAMERA_BACKWARD.isDown)
		{
			camera_distance += (camera_speed * camera_origin);
			volume_camera_distance += (camera_speed * volume_camera_origin);
		}

		
#endif
		if (Input->Controllers[0].APP_QUIT.isDown)
		{
			Running = false;
		}

		
}
static glm::vec2 GetMouseDelta(glm::vec2 oldpos, glm::vec2 newpos)
{
	return newpos - oldpos;
}

 static void RenderGameandUpdate(SDL_Window *Window, int WindowWidth, int WindowHeight,game_state *GameState, game_input *Input)
 {
	
	 //mouse
	 newMousepos = Input->Controllers[0].MousePos;
	  mouseDelta = GetMouseDelta(oldMousepos, newMousepos);
	 oldMousepos = newMousepos;
	 CheckMovement(Input);

	 //camera
	 if ((glm::length(mouseDelta) < 50.0f))
	 {
		   right_vector = glm::cross(camera_origin, glm::vec3(0, 1, 0));
		 //camera speed
		 camera_origin = glm::mat3(glm::rotate(-mouseDelta.x*0.005f, glm::vec3(0, 1, 0))) * camera_origin;
		 camera_origin = glm::mat3(glm::rotate(mouseDelta.y*0.005f, right_vector)) * camera_origin;

		 //For some reason the y should be backwards...
		 volume_camera_origin = glm::mat3(glm::rotate(-mouseDelta.x*0.005f, glm::vec3(0, 1, 0))) * volume_camera_origin;
		 volume_camera_origin = glm::mat3(glm::rotate(-mouseDelta.y*0.005f, right_vector)) * volume_camera_origin;
		 
	 }
     
	 ViewMatrix = glm::lookAt(
		camera_distance,
		 camera_distance - camera_origin, // and looks at the origin
		 glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
	 );


	//sphere animation
	 static float testpos_x = 0.1f;
	 static float testpos_y = 0.2f;
	 if (pointLight.position.x >= 3.0f)
	 {
		 testpos_x = -0.1f;
	 }
	 else if(pointLight.position.x <= -3.0)
	 {
		 testpos_x = 0.1f;
	 }
	 if (pointLight.position.y >= 3.0f)
	 {
		 testpos_y = -0.2f;
	 }
	 else if (pointLight.position.y <= -3.0)
	 {
		 testpos_y = 0.2f;
	 }
     pointLight.position += glm::vec3(testpos_x, testpos_y, 0);
	 pointLight2.position += glm::vec3(testpos_x, 0, 0);

	
	 //Game is playable
	 mProjection = glm::perspective(glm::radians(60.0f), (float)WindowWidth / (float)WindowHeight, 0.1f, 100.0f);
	 GameState->VolumeLightQuad.WorldPos = camera_distance + glm::vec3(0.0f,0.0f,-3.0f);
	 glDepthMask(GL_TRUE);
	 GetDepthMap(testModel, GameState);
	 GetShadowMap(testModel, GameState);
	 GetVolumeLightMap(testModel, GameState);
	 glBindFramebuffer(GL_FRAMEBUFFER, 0);
	 glUseProgram(programID);
	
	 glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
	 glDepthMask(GL_TRUE);
	 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	 
	
	
	  //shadow Map
	  glUniform1i(sampler_location, 0);
	  glActiveTexture(GL_TEXTURE1);
	  glBindTexture(GL_TEXTURE_2D, shadowMap);
	  glUniform1i(glGetUniformLocation(programID, "shadowMap"), 1);

	 drawModel(&GameState->Model1);
	  //THE SUN
	 DirectionalLight.position = glm::vec3(1.0f, 0.0f, 150.0f);//pointLight.position + glm::vec3(0.0f, 0.0f, 100.0f);
	 testModel = glm::mat4(1.0f);
	 testModel = glm::translate(testModel, DirectionalLight.position);
	 
	 mvp = mProjection * ViewMatrix * testModel;

	
	 glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &mvp[0][0]);
	 BindTexture(0, GameState->Model1.Texture, &sampler_state);
	 glUniform4f(vcol_location, 0.0f, 0.4f, 0.0f, 0.0f);
	 
	 UpdateShadowMatrix(programID, testModel);
	 glDrawArrays(GL_TRIANGLES, 0, GameState->Model1.vert.position.size());

	 //The moving model
	 
	 testModel = glm::mat4(1.0f);
	 testModel = glm::translate(testModel, pointLight.position);
	 BindTexture(0, GameState->Model1.Texture, &sampler_state);
	 mvp = mProjection * ViewMatrix * testModel;
	 glUniform4f(vcol_location, 1.0f, 1.0f, 1.0f, 0.0f);
	 
	 glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &mvp[0][0]);
	 UpdateShadowMatrix(programID, testModel);
	 glDrawArrays(GL_TRIANGLES, 0, GameState->Model1.vert.position.size());


	 //The moving model 2

	 testModel = glm::mat4(1.0f);
	 testModel = glm::translate(testModel, pointLight2.position);
	 BindTexture(0, GameState->Model1.Texture, &sampler_state);
	 mvp = mProjection * ViewMatrix * testModel;
	 glUniform4f(vcol_location, 1.0f, 1.0f, 1.0f, 0.0f);

	 glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &mvp[0][0]);
	 UpdateShadowMatrix(programID, testModel);
	 glDrawArrays(GL_TRIANGLES, 0, GameState->Model1.vert.position.size());
	
	//Draw Plane
	 //LEFT
	 DrawSquare(&GameState->Plane);
	 BindTexture(0, Brick_Texture, &sampler_state);
	 glUniform4f(vcol_location, 0.8f, 0.5f, 0.0f, 1.0f);
	 
	 testModel = glm::mat4(1.0f);
	// testModel = glm::rotate(testModel,-45.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	 testModel = glm::translate(testModel, GameState->Plane.WorldPos);// glm::vec3(50, 0, -5));
	
	 mvp = mProjection * ViewMatrix * testModel;

	 glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &mvp[0][0]);

	 UpdateShadowMatrix(programID, testModel);
	 glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());

	 //Draw second Plane
	//BOTTOM
	 glUniform4f(vcol_location, 0.0f, 0.5f, 0.5f, 1.0f);
	
	 testModel = glm::mat4(1.0f);
	 BindTexture(0, Grass_Texture, &sampler_state);
	 testModel = glm::translate(testModel, GameState->Plane.WorldPos + glm::vec3(1.0f, -15.0f, 40.0f));
	 testModel = glm::rotate(testModel, 30.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	 mvp = mProjection * ViewMatrix * testModel;

	

	 glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &mvp[0][0]);
	 UpdateShadowMatrix(programID, testModel);
	 glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());


	 //Draw third Plane
	 //RIGHT SECOND
	 testModel = glm::mat4(1.0f);
	 testModel = glm::translate(testModel, GameState->Plane.WorldPos + glm::vec3(-52.0f, 20.0f, 40.0f));
	 testModel = glm::rotate(testModel, 60.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	 BindTexture(0, Brick_Texture, &sampler_state);
	 mvp = mProjection * ViewMatrix * testModel;
	 glUniform4f(vcol_location, 0.1f, 0.1f, 0.1f, 1.0f);

	 glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &mvp[0][0]);
	 UpdateShadowMatrix(programID, testModel);
	 glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());

	 //Draw fourth Plane
	 //RIGHT FIRST
	 testModel = glm::mat4(1.0f);
	 
	 testModel = glm::translate(testModel, GameState->Plane.WorldPos + glm::vec3(50.0f, 20.0f, 40.0f));
	 testModel = glm::rotate(testModel, 60.0f, glm::vec3(1.0f, 0.0f, 0.0f));

	 mvp = mProjection * ViewMatrix * testModel;
	 glUniform4f(vcol_location, 0.1f, 0.1f, 0.1f, 1.0f);
	
	 glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &mvp[0][0]);
	 UpdateShadowMatrix(programID, testModel);
	 glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());

	 //Draw fifth Plane
	 //TOP 
	 testModel = glm::mat4(1.0f);
	 
	 testModel = glm::translate(testModel, GameState->Plane.WorldPos + glm::vec3(1.0f, 20.0f, 40.0f));
	 testModel = glm::rotate(testModel, -30.0f, glm::vec3(1.0f, 0.0f, 0.0f));

	 mvp = mProjection * ViewMatrix * testModel;

	

	 glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &mvp[0][0]);
	 UpdateShadowMatrix(programID, testModel);
	 glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());

	 //Draw sixth Plane
	 //BACK
	 testModel = glm::mat4(1.0f);

	 testModel = glm::translate(testModel, GameState->Plane.WorldPos + glm::vec3(-50.0f, 20.0f, 40.0f));
	testModel = glm::rotate(testModel, -30.0f, glm::vec3(0.0f, 1.0f, 0.0f));

	 mvp = mProjection * ViewMatrix * testModel;

	

	 glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &mvp[0][0]);
	 UpdateShadowMatrix(programID, testModel);
	 glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());

	 //Draw seventh Plane
	 //FRONT
	 testModel = glm::mat4(1.0f);

	 testModel = glm::translate(testModel, GameState->Plane.WorldPos + glm::vec3(50.0f, 20.0f, 40.0f));
	 testModel = glm::rotate(testModel, 30.0f, glm::vec3(0.0f, 1.0f, 0.0f));

	 mvp = mProjection * ViewMatrix * testModel;

	

	 glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &mvp[0][0]);
	 UpdateShadowMatrix(programID, testModel);
	 glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->Plane.vert.position.size());

	
	 
	 
	
	 
	//Rest of the models
	//DRAW FULLSCREEN QUAD

	 DrawSquare(&GameState->VolumeLightQuad);
	 glUniform4f(vcol_location, 1.0f, 1.0f, 1.0f, 1.0f);
	 
	 testModel = glm::mat4(1.0);
	 mvp = testModel;
     glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &mvp[0][0]);
	 GLuint empty_tex = 0;
	 glEnable(GL_BLEND);
	 glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	 BindTexture(0, VolumeLightTexture, &sampler_state);
	 BindTexture(2, VolumeLightTexture, &sampler_state);
	 glUniform1i(glGetUniformLocation(programID, "volumeLightTexture"), 2);

	 glm::mat4 modelMatrix = testModel;
	 modelMatrix = glm::translate(modelMatrix, GameState->Plane.WorldPos + glm::vec3(50.0f, 20.0f, 40000.0f));
	 UpdateShadowMatrix(programID,modelMatrix);
     glDrawArrays(GL_TRIANGLE_STRIP, 0, GameState->VolumeLightQuad.vert.position.size());
	 glDisable(GL_BLEND);
}


static void InitGlew()
{
	
	GLenum err = glewInit();
	if (err != GLEW_OK)
		exit(1);
}



static void CheckEvents(game_state *GameState, game_input *NewInput, SDL_Window *window)
{
	while (SDL_PollEvent(&event) != 0)
	{

		switch (event.type)
		{
		case SDL_QUIT:
		{
			Running = false;
			break;
		}
		case SDL_KEYDOWN:
		{

			if (event.key.keysym.sym == SDLK_h)
			{
				
			}
			else if (event.key.keysym.sym == SDLK_w)
			{
				NewInput->Controllers[0].UP.isDown = true;
			}
			else if (event.key.keysym.sym == SDLK_s)
			{
				NewInput->Controllers[0].DOWN.isDown = true;
			}
			else if (event.key.keysym.sym == SDLK_d)
			{
				NewInput->Controllers[0].RIGHT.isDown = true;
			}
			else if (event.key.keysym.sym == SDLK_a)
			{
				NewInput->Controllers[0].LEFT.isDown = true;
			}
			else if (event.key.keysym.sym == SDLK_LEFT)
			{
				NewInput->Controllers[0].CAMERA_STRAFE_LEFT.isDown = true;
			}
			else if (event.key.keysym.sym == SDLK_RIGHT)
			{
				NewInput->Controllers[0].CAMERA_STRAFE_RIGHT.isDown = true;
			}
			else if (event.key.keysym.sym == SDLK_UP)
			{
				NewInput->Controllers[0].CAMERA_FORWARD.isDown = true;
			}
			else if (event.key.keysym.sym == SDLK_DOWN)
			{
				NewInput->Controllers[0].CAMERA_BACKWARD.isDown = true;
			}
			else if (event.key.keysym.sym == SDLK_ESCAPE)
			{
				NewInput->Controllers[0].APP_QUIT.isDown = true;
			}
			else if (event.key.keysym.sym == SDLK_F11)
			{
				if (!window_is_fullscreen)
				{
					window_is_fullscreen = true;
					SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
				}
				else
				{
					SDL_SetWindowFullscreen(window, 0);
					window_is_fullscreen = false;
				}
			}

		}break;
		case SDL_KEYUP:
		{
			if (event.key.keysym.sym == SDLK_w)
			{
				NewInput->Controllers[0].UP.isDown = false;
			}
			else if (event.key.keysym.sym == SDLK_s)
			{
				NewInput->Controllers[0].DOWN.isDown = false;
			}
			else if (event.key.keysym.sym == SDLK_d)
			{
				NewInput->Controllers[0].RIGHT.isDown = false;
			}
			else if (event.key.keysym.sym == SDLK_a)
			{
				NewInput->Controllers[0].LEFT.isDown = false;
			}
			else if (event.key.keysym.sym == SDLK_ESCAPE)
			{
				NewInput->Controllers[0].APP_QUIT.isDown = false;
			}
			else if (event.key.keysym.sym == SDLK_LEFT)
			{
				NewInput->Controllers[0].CAMERA_STRAFE_LEFT.isDown = false;
			}
			else if (event.key.keysym.sym == SDLK_RIGHT)
			{
				NewInput->Controllers[0].CAMERA_STRAFE_RIGHT.isDown = false;
			}
			else if (event.key.keysym.sym == SDLK_UP)
			{
				NewInput->Controllers[0].CAMERA_FORWARD.isDown = false;
			}
			else if (event.key.keysym.sym == SDLK_DOWN)
			{
				NewInput->Controllers[0].CAMERA_BACKWARD.isDown = false;
			}
			else if (event.key.keysym.sym == SDLK_ESCAPE)
			{
				NewInput->Controllers[0].APP_QUIT.isDown = false;
			}
		}break;
		case SDL_MOUSEMOTION:
		{
			//this is in floating point 
			NewInput->Controllers[0].MousePos = glm::vec2(event.motion.x, event.motion.y);
		}break;
		case SDL_MOUSEBUTTONDOWN:
		{
			if(event.button.button == SDL_BUTTON_LEFT)
			{
				NewInput->Controllers[0].CLICK_LEFT.isDown = true;
			}
			else if (event.button.button == SDL_BUTTON_RIGHT)
			{
				NewInput->Controllers[0].CLICK_RIGHT.isDown = true;
			}
			else if (event.button.button == SDL_BUTTON_MIDDLE)
			{
				NewInput->Controllers[0].CLICK_MIDDLE.isDown = true;
			}
		}break;
		case SDL_MOUSEBUTTONUP:
		{

			if (event.button.button == SDL_BUTTON_LEFT)
			{
				NewInput->Controllers[0].CLICK_LEFT.isDown = false;
			}
			else if (event.button.button == SDL_BUTTON_RIGHT)
			{
				NewInput->Controllers[0].CLICK_RIGHT.isDown = false;
			}
			else if (event.button.button == SDL_BUTTON_MIDDLE)
			{
				NewInput->Controllers[0].CLICK_MIDDLE.isDown = false;
			}
		}break;
		default:
		{
			SDL_PumpEvents();
			break;
		}
		}
	}
}

int main(int argc, char* args[])
{
	
	//The window we'll be rendering to
	SDL_Window* window = NULL;

	
	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "Cannot initialize SDL video!\n");
		exit(1);
	}
	
		//Create window
	window = SDL_CreateWindow("LoL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WindowWidth, WindowHeight, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
	if (window == NULL)
	{
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return 0;
	}

	SDL_GLContext gContext = NULL;

	//Create context
	//Has to be version 3.1 ???????????????????? minor
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	gContext = SDL_GL_CreateContext(window);
	if (gContext == NULL)
	{
		printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
	}

	InitGlew();
	printf("-------------------------------------------------\n");
	printf("GLEW INITIALIZED!\n");
	printf("OpenGL Info\n");
	printf("    Version: %s\n", glGetString(GL_VERSION));
	printf("     Vendor: %s\n", glGetString(GL_VENDOR));
	printf("   Renderer: %s\n", glGetString(GL_RENDERER));
	printf("    Shading: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("-------------------------------------------------\n");
	
	
	
	game_state GameState = {};
	//Init the Game Memory
	InitiateGame(&GameState);
	

	game_input GameInput[2] = {};
	game_input* NewInput = &GameInput[0];

	ace_timer thetimer= {};

	thetimer.oldtime = SDL_GetTicks();
	thetimer.newtime = SDL_GetTicks();
	thetimer.global_oldtime = SDL_GetTicks();
	thetimer.global_newtime = SDL_GetTicks();

	glEnable(GL_TEXTURE_2D);
	
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	//glDepthFunc(GL_ALWAYS);
	
	glEnable(GL_CULL_FACE);
	glViewport(0, 0, WindowWidth, WindowHeight);
	
	Running = true;
	while (Running)
	{
	
		CheckEvents(&GameState, NewInput, window);
		//DRAW THE GAME
		RenderGameandUpdate(window, WindowWidth, WindowHeight,&GameState, NewInput);
		

		SDL_GL_SetSwapInterval(0);
		ForceFramerate(&thetimer);

		SDL_GL_SwapWindow(window);
	}
	glDeleteProgram(programID);
	//Free the sound effects
	v_models.clear();
	//Free the music
	
	SDL_GL_DeleteContext(gContext);
	SDL_DestroyWindow(window);
	window = NULL;
	SDL_Quit();


	return 0;
}