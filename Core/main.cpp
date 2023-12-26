#include<iostream>

//include glad before GLFW to avoid header conflict or define "#define GLFW_INCLUDE_NONE"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <map>

#include "camera.h"
#include "shader.h"
#include "object.h"

// ######## Session Variables ############
const int window_width = 800;
const int window_height = 800;

bool firstMouse = true; // for mouse_callback
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right, so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;

float fov = 45.0f;

bool isCursorCaptured = true; // Initially capture the cursor
// #######################################



void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

GLuint compileShader(std::string shaderCode, GLenum shaderType);
GLuint compileProgram(GLuint vertexShader, GLuint fragmentShader);

void processKeyboardCameraInput(GLFWwindow* window);

void loadCubemapFace(const char* path, const GLenum& targetFace);

#ifndef NDEBUG
void APIENTRY glDebugOutput(GLenum source,
	GLenum type,
	unsigned int id,
	GLenum severity,
	GLsizei length,
	const char* message,
	const void* userParam)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
	case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
}
#endif

GLuint loadTexture(const char* path) {
	stbi_set_flip_vertically_on_load(true);
	int width, height, nrChannels;
	unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
	if (!data) {
		std::cout << "Failed to load texture" << std::endl;
		return 0;
	}

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(data);

	return texture;
}

Camera camera(glm::vec3(0.0, 30.0, 60.0));

// we need to make sure that the keys for selecting the pieces are only selected ones and not continuously:
bool nKeyPressed = false;
bool lKeyPressed = false;
bool fKeyPressed = false;
bool enterKeyPressed = false;
bool alternate = false;

std::pair<int, int> getSelectedCube(std::vector<std::vector<Object>>& board) {
	// find the index of the field that is currently selected
	int index_i = 0;
	int index_j = 0;
	for (int i = 0; i < board.size(); i++) {
		for (int j = 0; j < board[i].size(); j++) {
			if (board[i][j].selected == 1.0) {
				index_i = i;
				index_j = j;

			}
		}
	}
	return std::make_pair(index_i, index_j);
}

int getSelectedPawn(std::vector<Object>& pawns) {
	int index = 0;
	for (int i = 0; i < pawns.size(); i++) {
		if (pawns[i].selected == 1.0) {
			index = i;
		}
	}
	return index;
}


void processSelected(GLFWwindow* window, std::vector<Object>& object) {
	// find the index of the piece that is currently selected
	int index = 0;
	for (int i = 0; i < object.size(); i++) {
		if (object[i].selected == 1.0) {
			index = i;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS && !nKeyPressed) {			// select next pawn in array pawns and unselect the current pawn
		object[index].selected = 0.0;
		if (index < object.size() - 1) {
			object[index + 1].selected = 1.0;
		}
		else {
			object[0].selected = 1.0;		// start from beginning or array
		}
		nKeyPressed = true;
	}
	else if (glfwGetKey(window, GLFW_KEY_N) == GLFW_RELEASE) {
		nKeyPressed = false;  // Reset the n key
	}
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !lKeyPressed) {			// select last pawn in array pawns and unselect the current pawn
		object[index].selected = 0.0;
		if (index > 0) {
			object[index - 1].selected = 1.0;
		}
		else {
			object[object.size() - 1].selected = 1.0;		// select the last piece in array
		}
		lKeyPressed = true;
	}
	else if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE) {
		lKeyPressed = false;  // Reset the l key
	}

}

void processSelectedField(GLFWwindow* window, std::vector<std::vector<Object>>& board) {
	// find the index of the field that is currently selected
	int index_i = getSelectedCube(board).first;
	int index_j = getSelectedCube(board).second;
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fKeyPressed) {			// select next field in array board and unselect the current field
		board[index_i][index_j].selected = 0.0;
		int next_i = index_i;
		int next_j = index_j;

		do {
			if (next_i < board[next_j].size() - 1) {
				next_i += 1;		
			}	
			else {	
				next_i = 0;		
				next_j++;	
			}	

			if (next_j >= board.size()) {	
				next_j = 0;	
			}	

		} while (board[next_i][next_j].color != "black");	// find the next black field in the array board

		board[next_i][next_j].selected = 1.0;	
		fKeyPressed = true;
	}
	else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
		fKeyPressed = false;  // Reset the f key
	}
}


void translateMeeple(GLFWwindow* window, std::vector<std::vector<Object>>& board, std::vector<Object>& pawns) {
	int index_i = getSelectedCube(board).first;
	int index_j = getSelectedCube(board).second;
	int index_pawn = getSelectedPawn(pawns);
	//std::cout << "test translateMeeple" << std::endl;

	// get position of selected cube
	glm::vec3 cube_pos = board[index_i][index_j].getPos();
	//pawns[0].model = glm::translate(pawns[0].model, glm::vec3(cube_pos.x, cube_pos.y, cube_pos.z));


}

int main(int argc, char* argv[])
{
	std::cout << "Welcome to the demo by Igors and Veronika" << std::endl;

	//Boilerplate
	//Create the OpenGL context
	if (!glfwInit()) {
		throw std::runtime_error("Failed to initialise GLFW \n");
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef NDEBUG
	//create a debug context to help with Debugging
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif


	//Create the window
	GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Main_Window", nullptr, nullptr);
	if (window == NULL)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window\n");
	}

	glfwMakeContextCurrent(window);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	//load openGL function
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		throw std::runtime_error("Failed to initialize GLAD");
	}

	glEnable(GL_DEPTH_TEST);

#ifndef NDEBUG
	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}
#endif


// ######## Setup Generic Shaders ############
    char Generic_Fragment_Shader_file[128] = PATH_TO_SHADERS"/Generic_Fragment_Shader.frag";
    char Generic_Vertex_Shader_file[128] = PATH_TO_SHADERS"/Generic_Vertex_Shader.vert";
    Shader Generic_Shader = Shader(Generic_Vertex_Shader_file, Generic_Fragment_Shader_file);
// ###########################################

// ######## Setup Debug Sphere Shaders ############
//    char Debug_Sphere_Fragment_Shader_file[128] = PATH_TO_SHADERS"/Debug_Sphere_Fragment_Shader.frag";
//    char Debug_Sphere_Vertex_Shader_file[128] = PATH_TO_SHADERS"/Debug_Sphere_Vertex_Shader.vert";
//    Shader Debug_Sphere_Shader = Shader(Debug_Sphere_Vertex_Shader_file, Debug_Sphere_Fragment_Shader_file);
// ###########################################


// ######## Setup Background CubeMap Shaders ############
    char Background_CubeMap_Vertex_Shader_file[128] = PATH_TO_SHADERS"/Background_CubeMap_Vertex_Shader.vert";
    char Background_CubeMap_Fragment_Shader_file[128] = PATH_TO_SHADERS"/Background_CubeMap_Fragment_Shader.frag";
	Shader cubeMapShader = Shader(Background_CubeMap_Vertex_Shader_file, Background_CubeMap_Fragment_Shader_file);
// ###########################################


// ######## FPS Counter #######################
	double prev = 0;
	int deltaFrame = 0;
	//fps function
	auto fps = [&](double now) {
		double deltaTime = now - prev;
		deltaFrame++;
		if (deltaTime > 0.5) {
			prev = now;
			const double fpsCount = (double)deltaFrame / deltaTime;
			deltaFrame = 0;
			std::cout << "\r FPS: " << fpsCount;
		}
		};
// ###########################################


// ######## Objects #######################
// Chess Board Chopped

    //texture

	
	std::vector<std::vector<Object>> board;		// 2Dvector for all fields
	char path_Board_Colour_1[] = PATH_TO_TEXTURE"/Checkers_Board/Board_Colour_1.png";
	GLuint Board_Texture_1 = loadTexture(path_Board_Colour_1);

	char path_Board_Colour_2[] = PATH_TO_TEXTURE"/Checkers_Board/Board_Colour_2.png";
	GLuint Board_Texture_2 = loadTexture(path_Board_Colour_2);

	char pathBoard[] = PATH_TO_OBJECTS"/Chess_Board_Chopped/Board_0x_0y.obj";
	for (int i = 0; i < 4; i++) {
		std::vector<Object> row;
		for (int j = 0; j < 4; j++) {
			Object field(pathBoard);
			field.position = glm::vec3(2.0 * i, 0.0, 2.0 * j);
			field.model = glm::translate(field.model, field.position);
			field.makeObject(Generic_Shader);
			row.push_back(field);
		}
		board.push_back(row);
	}

	// load and arrange pawns
	char path_text_pawn[] = PATH_TO_TEXTURE"/texPawn.jpg";
	GLuint texture_pawn = loadTexture(path_text_pawn);
	char pathPawn[] = PATH_TO_OBJECTS"/pawn.obj";
	std::vector<Object> pawns;
	for (int i = 0; i < 4; i++) {
		Object pawn(pathPawn);
		pawn.model = glm::scale(pawn.model, glm::vec3(0.3, 0.3, 0.3));
		pawn.model = glm::rotate(pawn.model, (float)-3.14 / 2, glm::vec3(1.0f, 0.0f, 0.0f));
		pawn.model = glm::translate(pawn.model, glm::vec3(5.0 + 7.5 * i, 8.0, 3.4));
		pawn.makeObject(Generic_Shader);
		pawns.push_back(pawn);
	}

	// add a sphere in origin for reference
	char path3[] = PATH_TO_OBJECTS"/sphere_smooth.obj";
	Object sphere3(path3);
	sphere3.makeObject(Generic_Shader);

	// cubemap (background)
	char pathCube[] = PATH_TO_OBJECTS"/cube.obj";
	Object cubeMap(pathCube);
	cubeMap.makeObject(cubeMapShader);


	// orienting the objects in the scene:
	// sphere
	sphere3.model = glm::translate(sphere3.model, glm::vec3(0.0, 0.0, 0.0));
	sphere3.model = glm::scale(sphere3.model, glm::vec3(1.5, 1.5, 1.5));

	// camera variables:
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 perspective = camera.GetProjectionMatrix();


	// light:
	glm::vec3 light_pos = glm::vec3(0.0, 4.0, 1.3);
	glm::vec3 light_col = glm::vec3(1.0, 0.0, 0.0);

	float ambient = 0.5;
	float diffuse = 0.9;
	float specular = 0.4;
	float shininess = 2.0;

    Generic_Shader.use();
    Generic_Shader.setFloat("shininess", shininess);
    Generic_Shader.setFloat("light.ambient_strength", ambient);
    Generic_Shader.setFloat("light.diffuse_strength", diffuse);
    Generic_Shader.setFloat("light.specular_strength", specular);
    Generic_Shader.setFloat("light.constant", 1.0);
    Generic_Shader.setFloat("light.linear", 0.14);
    Generic_Shader.setFloat("light.quadratic", 0.07);


	// give texture to background (cubmap)
	GLuint cubeMapTexture;
	glGenTextures(1, &cubeMapTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);

	// texture parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	std::string pathToCubeMap = PATH_TO_TEXTURE"/cubemaps/Apartment/";

	std::map<std::string, GLenum> facesToLoad = {
		{pathToCubeMap + "posx.jpg",GL_TEXTURE_CUBE_MAP_POSITIVE_X},
		{pathToCubeMap + "posy.jpg",GL_TEXTURE_CUBE_MAP_POSITIVE_Y},
		{pathToCubeMap + "posz.jpg",GL_TEXTURE_CUBE_MAP_POSITIVE_Z},
		{pathToCubeMap + "negx.jpg",GL_TEXTURE_CUBE_MAP_NEGATIVE_X},
		{pathToCubeMap + "negy.jpg",GL_TEXTURE_CUBE_MAP_NEGATIVE_Y},
		{pathToCubeMap + "negz.jpg",GL_TEXTURE_CUBE_MAP_NEGATIVE_Z},
	};
	//load the six faces
	for (std::pair<std::string, GLenum> pair : facesToLoad) {
		loadCubemapFace(pair.first.c_str(), pair.second);
	}

	// mark first pawn as selected
	pawns[3].selected = 1.0;

	// mark first dark field as selected;
	board[1][0].selected = 1.0;

	glm::vec3 test = board[0][0].getPos();

	//std::cout << test.x << test.y << test.z << std::endl;

	// initialize first set up of pawns:
	/*pawns[0].model = glm::mat4(1.0f);
	board[1][0].model = glm::mat4(1.0f);
	glm::vec3 cube_pos = board[1][0].getPos();
	glm::vec3 pawn_pos = pawns[0].getPos();
	// get position of cube and pawn without translations
	std::cout << "before translation" << std::endl;
	std::cout << cube_pos.x << cube_pos.y << cube_pos.z << std::endl;
	std::cout << pawn_pos.x << pawn_pos.y << pawn_pos.z << std::endl;
	//translate pawn and cube by 1.0
	pawns[0].model = glm::translate(pawns[0].model, glm::vec3(1.0,1.0,1.0));
	board[1][0].model = glm::translate(board[1][0].model, glm::vec3(1.0, 1.0, 1.0));
	cube_pos = board[1][0].getPos();
	pawn_pos = pawns[0].getPos();
	std::cout << "after translation" << std::endl;
	std::cout << cube_pos.x << cube_pos.y << cube_pos.z << std::endl;
	std::cout << pawn_pos.x << pawn_pos.y << pawn_pos.z << std::endl;


	int index_i = getSelectedCube(board).first;
	int index_j = getSelectedCube(board).second;
	cube_pos = board[index_i][index_j].getPos();
	pawn_pos = pawns[0].getPos();
	pawns[0].position = glm::vec3(cube_pos.x, cube_pos.y, cube_pos.z);
	glm::vec3 pawn_pos_new = pawns[0].getPos();
	pawns[0].model = glm::mat4(1.0f);
	pawns[0].model = glm::translate(pawns[0].model, glm::vec3(cube_pos.x* -1.07374e+08, cube_pos.y * -1.07374e+08, cube_pos.z * -1.07374e+08));
	//glm::vec3 pawn_pos_new = pawns[0].getPos();
	//std::cout << cube_pos.x << cube_pos.y << cube_pos.z << std::endl;
	//std::cout << pawn_pos.x << pawn_pos.y << pawn_pos.z << std::endl;
	//std::cout << pawn_pos_new.x << pawn_pos_new.y << pawn_pos_new.z << std::endl;
	*/


	glfwSwapInterval(1);
	//Rendering
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(window, key_callback);

	while (!glfwWindowShouldClose(window)) {
        processKeyboardCameraInput(window);
		processSelected(window, pawns);
		processSelectedField(window, board);
		//translateMeeple(window, board, pawns);
		view = camera.GetViewMatrix();
		glfwPollEvents();
        glfwSetKeyCallback(window, key_callback); //Lookout for ALT keypress
		double now = glfwGetTime();
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// initialize rendering (send parameters to the shader)
        Generic_Shader.use();
        Generic_Shader.setMatrix4("V", view);
        Generic_Shader.setMatrix4("P", perspective);
        Generic_Shader.setVector3f("light.light_pos", light_pos);
        Generic_Shader.setVector3f("light.light_color", light_col);
        Generic_Shader.setVector3f("u_view_pos", camera.Position);

		
		// render the pawns
		for (auto& pawn : pawns) {
			// move selected piece on chessboard
			if (pawn.selected == 1.0 && glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS && !enterKeyPressed) {
				pawn.model = pawn.model + glm::translate(pawn.model, glm::vec3(0.0, 15.0, 0.0));
				enterKeyPressed = true;
			}
			else if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE) {
				enterKeyPressed = false; 
			}
            Generic_Shader.use();
            Generic_Shader.setMatrix4("M", pawn.model);
            //Generic_Shader.setMatrix4("itM", inverseModel);
            Generic_Shader.setInteger("ourTexture", 0);
            Generic_Shader.setFloat("selected", pawn.selected);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture_pawn);
			glDepthFunc(GL_LEQUAL);
			pawn.draw();
		}

		// render the board
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				Generic_Shader.use();
				Generic_Shader.setMatrix4("M", board[i][j].model);
				Generic_Shader.setInteger("ourTexture", 0);
				Generic_Shader.setFloat("selected", board[i][j].selected);
				glActiveTexture(GL_TEXTURE0);
				if ((i + j) % 2 == 0) { 
					glBindTexture(GL_TEXTURE_2D, Board_Texture_1); 
					board[i][j].color = "white"; 
				}
				else { 
					glBindTexture(GL_TEXTURE_2D, Board_Texture_2); board[i][j].color = "black";
				}
				glDepthFunc(GL_LEQUAL);
				board[i][j].draw();
			}
		}

		// render the cubemap
		cubeMapShader.use();
		cubeMapShader.setMatrix4("V", view);
		cubeMapShader.setMatrix4("P", perspective);
		cubeMapShader.setInteger("cubemapTexture", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
		cubeMap.draw();
		glDepthFunc(GL_LESS);




		fps(now);
		glfwSwapBuffers(window);
	}

	//clean up resources
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}


void loadCubemapFace(const char* path, const GLenum& targetFace)
{
	int imWidth, imHeight, imNrChannels;
	stbi_set_flip_vertically_on_load(false);
	unsigned char* data = stbi_load(path, &imWidth, &imHeight, &imNrChannels, 0);
	if (data)
	{

		glTexImage2D(targetFace, 0, GL_RGB, imWidth, imHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		//glGenerateMipmap(targetFace);
	}
	else {
		std::cout << "Failed to Load texture" << std::endl;
		const char* reason = stbi_failure_reason();
		std::cout << (reason == NULL ? "Probably not implemented by the student" : reason) << std::endl;
	}
	stbi_image_free(data);
}

void processKeyboardCameraInput(GLFWwindow* window) {
	// Use the cameras class to change the parameters of the camera
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(LEFT, 0.1);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(RIGHT, 0.1);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(FORWARD, 0.1);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(BACKWARD, 0.1);

	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(1, 0.0, 1);
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(-1, 0.0, 1);

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(0.0, 1.0, 1);
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		camera.ProcessKeyboardRotation(0.0, -1.0, 1);
}


//taken from https://learnopengl.com/Getting-started/Camera
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

//Catch ALT being pressed on the keyboard, to show the cursor again
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS) {
        isCursorCaptured = !isCursorCaptured;

        if (isCursorCaptured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

//taken from https://learnopengl.com/Getting-started/Camera
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll((yoffset));
}
