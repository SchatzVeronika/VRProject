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
// #######################################



void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

GLuint compileShader(std::string shaderCode, GLenum shaderType);
GLuint compileProgram(GLuint vertexShader, GLuint fragmentShader);

void processInput(GLFWwindow* window);

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
bool enterKeyPressed = false;

void processSelected(GLFWwindow* window, std::vector<Object>& pawns) {
	// find the index of the piece that is currently selected
	int index = 0;
	for (int i = 0; i < pawns.size(); i++) {
		if (pawns[i].selected == 1.0) {
			index = i;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS && !nKeyPressed) {			// select next pawn in array pawns and unselect the current pawn
		pawns[index].selected = 0.0;
		if (index < pawns.size()-1) {
			pawns[index + 1].selected = 1.0;
		}
		else {
			pawns[0].selected = 1.0;		// start from beginning or array
		}
		nKeyPressed = true;
	}
	else if (glfwGetKey(window, GLFW_KEY_N) == GLFW_RELEASE) {
		nKeyPressed = false;  // Reset the n key
	}
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !lKeyPressed) {			// select last pawn in array pawns and unselect the current pawn
		pawns[index].selected = 0.0;
		if (index > 0) {
			pawns[index - 1].selected = 1.0;
		}
		else {
			pawns[pawns.size() - 1].selected = 1.0;		// select the last piece in array
		}
		lKeyPressed = true;
	}
	else if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE) {
		lKeyPressed = false;  // Reset the l key
	}

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
	char path_Board_Colour_1[] = PATH_TO_TEXTURE"/Checkers_Board/Board_Colour_1.png";
    GLuint Board_Texture_1 = loadTexture(path_Board_Colour_1);

    char path_Board_Colour_2[] = PATH_TO_TEXTURE"/Checkers_Board/Board_Colour_2.png";
    GLuint Board_Texture_2 = loadTexture(path_Board_Colour_2);


	char path_Board_0x_0y[] = PATH_TO_OBJECTS"/Chess_Board_Chopped/Board_0x_0y.obj";
	Object Board_0x_0y(path_Board_0x_0y);
    Board_0x_0y.makeObject(Generic_Shader);

    char path_Board_0x_2y[] = PATH_TO_OBJECTS"/Chess_Board_Chopped/Board_0x_2y.obj";
    Object Board_0x_2y(path_Board_0x_2y);
    Board_0x_2y.makeObject(Generic_Shader);

    char path_Board_0x_4y[] = PATH_TO_OBJECTS"/Chess_Board_Chopped/Board_0x_4y.obj";
    Object Board_0x_4y(path_Board_0x_4y);
    Board_0x_4y.makeObject(Generic_Shader);

    char path_Board_0x_6y[] = PATH_TO_OBJECTS"/Chess_Board_Chopped/Board_0x_6y.obj";
    Object Board_0x_6y(path_Board_0x_6y);
    Board_0x_6y.makeObject(Generic_Shader);

    char path_Board_2x_0y[] = PATH_TO_OBJECTS"/Chess_Board_Chopped/Board_2x_0y.obj";
    Object Board_2x_0y(path_Board_2x_0y);
    Board_2x_0y.makeObject(Generic_Shader);

    char path_Board_2x_2y[] = PATH_TO_OBJECTS"/Chess_Board_Chopped/Board_2x_2y.obj";
    Object Board_2x_2y(path_Board_2x_2y);
    Board_2x_2y.makeObject(Generic_Shader);

    char path_Board_2x_4y[] = PATH_TO_OBJECTS"/Chess_Board_Chopped/Board_2x_4y.obj";
    Object Board_2x_4y(path_Board_2x_4y);
    Board_2x_4y.makeObject(Generic_Shader);

    char path_Board_2x_6y[] = PATH_TO_OBJECTS"/Chess_Board_Chopped/Board_2x_6y.obj";
    Object Board_2x_6y(path_Board_2x_6y);
    Board_2x_6y.makeObject(Generic_Shader);

    char path_Board_4x_0y[] = PATH_TO_OBJECTS"/Chess_Board_Chopped/Board_4x_0y.obj";
    Object Board_4x_0y(path_Board_4x_0y);
    Board_4x_0y.makeObject(Generic_Shader);

    char path_Board_4x_2y[] = PATH_TO_OBJECTS"/Chess_Board_Chopped/Board_4x_2y.obj";
    Object Board_4x_2y(path_Board_4x_2y);
    Board_4x_2y.makeObject(Generic_Shader);

    char path_Board_4x_4y[] = PATH_TO_OBJECTS"/Chess_Board_Chopped/Board_4x_4y.obj";
    Object Board_4x_4y(path_Board_4x_4y);
    Board_4x_4y.makeObject(Generic_Shader);

    char path_Board_4x_6y[] = PATH_TO_OBJECTS"/Chess_Board_Chopped/Board_4x_6y.obj";
    Object Board_4x_6y(path_Board_4x_6y);
    Board_4x_6y.makeObject(Generic_Shader);

    char path_Board_6x_0y[] = PATH_TO_OBJECTS"/Chess_Board_Chopped/Board_6x_0y.obj";
    Object Board_6x_0y(path_Board_6x_0y);
    Board_6x_0y.makeObject(Generic_Shader);

    char path_Board_6x_2y[] = PATH_TO_OBJECTS"/Chess_Board_Chopped/Board_6x_2y.obj";
    Object Board_6x_2y(path_Board_6x_2y);
    Board_6x_2y.makeObject(Generic_Shader);

    char path_Board_6x_4y[] = PATH_TO_OBJECTS"/Chess_Board_Chopped/Board_6x_4y.obj";
    Object Board_6x_4y(path_Board_6x_4y);
    Board_6x_4y.makeObject(Generic_Shader);

    char path_Board_6x_6y[] = PATH_TO_OBJECTS"/Chess_Board_Chopped/Board_6x_6y.obj";
    Object Board_6x_6y(path_Board_6x_6y);
    Board_6x_6y.makeObject(Generic_Shader);



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

	// board
	//glm::mat4 model = glm::mat4(1.0);
	//model = glm::translate(model, glm::vec3(0.0, 0.0, -10.0));
    //Board_0x_0y.model = glm::scale(Board_0x_0y.model, glm::vec3(0.5, 0.5, 0.5));
    //Board_0x_0y.model = glm::rotate(Board_0x_0y.model, (float)-3.14 / 2, glm::vec3(1.0f, 0.0f, 0.0f));
	//glm::mat4 inverseModel = glm::transpose(glm::inverse(model));

    //glm::mat4 model2 = glm::mat4(1.0);
    //Board_0x_2y.model = glm::translate(Board_0x_2y.model, glm::vec3(0.0, 0.0, -10.0));
    //Board_0x_2y.model = glm::scale(Board_0x_2y.model, glm::vec3(0.5, 0.5, 0.5));
    //Board_0x_2y.model = glm::rotate(Board_0x_2y.model, (float)-3.14 / 2, glm::vec3(1.0f, 0.0f, 0.0f));
    //glm::mat4 inverseModel2 = glm::transpose(glm::inverse(model));



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

	glfwSwapInterval(1);
	//Rendering

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		processSelected(window, pawns);
		view = camera.GetViewMatrix();
		glfwPollEvents();
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
        Generic_Shader.setMatrix4("M", Board_0x_0y.model);
        //Generic_Shader.setMatrix4("itM", inverseModel);
        Generic_Shader.setInteger("ourTexture", 0);
        Generic_Shader.setFloat("selected", 0.0);		// we never want the board to be selected
		// add texture to board
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Board_Texture_1);
		glDepthFunc(GL_LEQUAL);
        Board_0x_0y.draw();

        Generic_Shader.setMatrix4("M", Board_0x_2y.model);
        //Generic_Shader.setMatrix4("itM", inverseModel2);
        Generic_Shader.setInteger("ourTexture", 0);
        Generic_Shader.setFloat("selected", 0.0);		// we never want the board to be selected
        // add texture to board
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Board_Texture_2);
        glDepthFunc(GL_LEQUAL);
        Board_0x_2y.draw();

        Generic_Shader.setMatrix4("M", Board_0x_4y.model);
        //Generic_Shader.setMatrix4("itM", inverseModel2);
        Generic_Shader.setInteger("ourTexture", 0);
        Generic_Shader.setFloat("selected", 0.0);		// we never want the board to be selected
        // add texture to board
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Board_Texture_1);
        glDepthFunc(GL_LEQUAL);
        Board_0x_4y.draw();

        Generic_Shader.setMatrix4("M", Board_0x_6y.model);
        //Generic_Shader.setMatrix4("itM", inverseModel2);
        Generic_Shader.setInteger("ourTexture", 0);
        Generic_Shader.setFloat("selected", 0.0);		// we never want the board to be selected
        // add texture to board
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Board_Texture_2);
        glDepthFunc(GL_LEQUAL);
        Board_0x_6y.draw();

        Generic_Shader.setMatrix4("M", Board_2x_0y.model);
        //Generic_Shader.setMatrix4("itM", inverseModel2);
        Generic_Shader.setInteger("ourTexture", 0);
        Generic_Shader.setFloat("selected", 0.0);		// we never want the board to be selected
        // add texture to board
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Board_Texture_2);
        glDepthFunc(GL_LEQUAL);
        Board_2x_0y.draw();

        Generic_Shader.setMatrix4("M", Board_2x_2y.model);
        //Generic_Shader.setMatrix4("itM", inverseModel2);
        Generic_Shader.setInteger("ourTexture", 0);
        Generic_Shader.setFloat("selected", 0.0);		// we never want the board to be selected
        // add texture to board
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Board_Texture_1);
        glDepthFunc(GL_LEQUAL);
        Board_2x_2y.draw();

        Generic_Shader.setMatrix4("M", Board_2x_4y.model);
        //Generic_Shader.setMatrix4("itM", inverseModel2);
        Generic_Shader.setInteger("ourTexture", 0);
        Generic_Shader.setFloat("selected", 0.0);		// we never want the board to be selected
        // add texture to board
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Board_Texture_2);
        glDepthFunc(GL_LEQUAL);
        Board_2x_4y.draw();

        Generic_Shader.setMatrix4("M", Board_2x_6y.model);
        //Generic_Shader.setMatrix4("itM", inverseModel2);
        Generic_Shader.setInteger("ourTexture", 0);
        Generic_Shader.setFloat("selected", 0.0);		// we never want the board to be selected
        // add texture to board
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Board_Texture_1);
        glDepthFunc(GL_LEQUAL);
        Board_2x_6y.draw();

        Generic_Shader.setMatrix4("M", Board_4x_0y.model);
        //Generic_Shader.setMatrix4("itM", inverseModel2);
        Generic_Shader.setInteger("ourTexture", 0);
        Generic_Shader.setFloat("selected", 0.0);		// we never want the board to be selected
        // add texture to board
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Board_Texture_1);
        glDepthFunc(GL_LEQUAL);
        Board_4x_0y.draw();

        Generic_Shader.setMatrix4("M", Board_4x_2y.model);
        //Generic_Shader.setMatrix4("itM", inverseModel2);
        Generic_Shader.setInteger("ourTexture", 0);
        Generic_Shader.setFloat("selected", 0.0);		// we never want the board to be selected
        // add texture to board
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Board_Texture_2);
        glDepthFunc(GL_LEQUAL);
        Board_4x_2y.draw();

        Generic_Shader.setMatrix4("M", Board_4x_4y.model);
        //Generic_Shader.setMatrix4("itM", inverseModel2);
        Generic_Shader.setInteger("ourTexture", 0);
        Generic_Shader.setFloat("selected", 0.0);		// we never want the board to be selected
        // add texture to board
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Board_Texture_1);
        glDepthFunc(GL_LEQUAL);
        Board_4x_4y.draw();

        Generic_Shader.setMatrix4("M", Board_4x_6y.model);
        //Generic_Shader.setMatrix4("itM", inverseModel2);
        Generic_Shader.setInteger("ourTexture", 0);
        Generic_Shader.setFloat("selected", 0.0);		// we never want the board to be selected
        // add texture to board
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Board_Texture_2);
        glDepthFunc(GL_LEQUAL);
        Board_4x_6y.draw();

        Generic_Shader.setMatrix4("M", Board_6x_0y.model);
        //Generic_Shader.setMatrix4("itM", inverseModel2);
        Generic_Shader.setInteger("ourTexture", 0);
        Generic_Shader.setFloat("selected", 0.0);		// we never want the board to be selected
        // add texture to board
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Board_Texture_2);
        glDepthFunc(GL_LEQUAL);
        Board_6x_0y.draw();

        Generic_Shader.setMatrix4("M", Board_6x_2y.model);
        //Generic_Shader.setMatrix4("itM", inverseModel2);
        Generic_Shader.setInteger("ourTexture", 0);
        Generic_Shader.setFloat("selected", 0.0);		// we never want the board to be selected
        // add texture to board
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Board_Texture_1);
        glDepthFunc(GL_LEQUAL);
        Board_6x_2y.draw();

        Generic_Shader.setMatrix4("M", Board_6x_4y.model);
        //Generic_Shader.setMatrix4("itM", inverseModel2);
        Generic_Shader.setInteger("ourTexture", 0);
        Generic_Shader.setFloat("selected", 0.0);		// we never want the board to be selected
        // add texture to board
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Board_Texture_2);
        glDepthFunc(GL_LEQUAL);
        Board_6x_4y.draw();

        Generic_Shader.setMatrix4("M", Board_6x_6y.model);
        //Generic_Shader.setMatrix4("itM", inverseModel2);
        Generic_Shader.setInteger("ourTexture", 0);
        Generic_Shader.setFloat("selected", 0.0);		// we never want the board to be selected
        // add texture to board
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Board_Texture_1);
        glDepthFunc(GL_LEQUAL);
        Board_6x_6y.draw();
		

		// render the sphere
//        Debug_Sphere_Shader.use();
//        Debug_Sphere_Shader.setMatrix4("V", view);
//        Debug_Sphere_Shader.setMatrix4("P", perspective);
//        Debug_Sphere_Shader.setMatrix4("itM", inverseModel);
//        Debug_Sphere_Shader.setMatrix4("M", sphere3.model);
//		sphere3.draw();

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

void processInput(GLFWwindow* window) {
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

//taken from https://learnopengl.com/Getting-started/Camera
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll((yoffset));
}
