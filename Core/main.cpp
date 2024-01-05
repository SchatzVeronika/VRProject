#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include<glm/gtc/matrix_inverse.hpp>
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
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
int i_row = 0;
float fov = 66.0f;
bool isCursorCaptured = true; // Initially capture the cursor

// Define camera attributes
glm::vec3 cameraPosition = glm::vec3(0.0f, 16.0f, 30.0f);
float aspectRatio = 1.0;
float nearPlane = 0.1f;
float farPlane = 1000.0f;

// Create camera and projection matrix
Camera camera(cameraPosition);

// Function Declarations
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

// ######## Setup Background CubeMap Shaders ############
    char Background_CubeMap_Vertex_Shader_file[128] = PATH_TO_SHADERS"/Background_CubeMap_Vertex_Shader.vert";
    char Background_CubeMap_Fragment_Shader_file[128] = PATH_TO_SHADERS"/Background_CubeMap_Fragment_Shader.frag";
	Shader cubeMapShader = Shader(Background_CubeMap_Vertex_Shader_file, Background_CubeMap_Fragment_Shader_file);
// ###########################################

// ######## Setup Room Shaders ############
    char Room_Vertex_Shader_file[128] = PATH_TO_SHADERS"/Room_Vertex_Shader.vert";
    char Room_Fragment_Shader_file[128] = PATH_TO_SHADERS"/Room_Fragment_Shader.frag";
    Shader Room_Shader = Shader(Room_Vertex_Shader_file, Room_Fragment_Shader_file);
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

    char pathCube[] = PATH_TO_OBJECTS "/cube.obj";
    Object cubeMap(pathCube);
    cubeMap.makeObject(cubeMapShader);

    char pathRoom[] = PATH_TO_OBJECTS"/room/room_full.obj";
    Object room(pathRoom);
    room.makeObject(Room_Shader, false);
    room.model = glm::scale(room.model, glm::vec3(0.5, 0.5, 0.5));
    room.position = glm::vec3(7.0, -5.0, 10.0);
    room.model = glm::translate(room.model, room.position);



    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 perspective = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);

	// Light:
// Define multiple light positions
    std::vector<glm::vec3> lightPositions = {
            glm::vec3(-5.0f, 25.0f, 10.0f),
            glm::vec3(-5.0f, 25.0f, 31.0f),
            glm::vec3(33.0f, 25.0f, 12.0f),
            glm::vec3(33.0f, 25.0f, 33.0f),
            glm::vec3(35.0f, 25.0f, -10.0f),
            glm::vec3(35.0f, 25.0f, -31.0f),
            glm::vec3(35.0f, 25.0f, -57.0f),
            glm::vec3(14.0f, 25.0f, 32.0f),
    };
    float ambient = 0.00001f;
    float diffuse = 0.99f;
    float specular = 0.0001f;

    glm::vec3 materialColour = glm::vec3(0.9f, 0.9f, 0.9f);

    Room_Shader.use();
    Room_Shader.setFloat("shininess", 0.2f);
    Room_Shader.setVector3f("materialColour", materialColour);

    Room_Shader.setVector3f("lights[0].light_pos", lightPositions[0]);
    Room_Shader.setFloat("lights[0].ambient_strength", ambient);
    Room_Shader.setFloat("lights[0].diffuse_strength", diffuse);
    Room_Shader.setFloat("lights[0].specular_strength", specular);
    Room_Shader.setFloat("lights[0].constant", 1.0);
    Room_Shader.setFloat("lights[0].linear", 0.14);
    Room_Shader.setFloat("lights[0].quadratic", 0.07);

    Room_Shader.setVector3f("lights[1].light_pos", lightPositions[1]);
    Room_Shader.setFloat("lights[1].ambient_strength", ambient);
    Room_Shader.setFloat("lights[1].diffuse_strength", diffuse);
    Room_Shader.setFloat("lights[1].specular_strength", specular);
    Room_Shader.setFloat("lights[1].constant", 1.0);
    Room_Shader.setFloat("lights[1].linear", 0.14);
    Room_Shader.setFloat("lights[1].quadratic", 0.07);

    Room_Shader.setVector3f("lights[2].light_pos", lightPositions[2]);
    Room_Shader.setFloat("lights[2].ambient_strength", ambient);
    Room_Shader.setFloat("lights[2].diffuse_strength", diffuse);
    Room_Shader.setFloat("lights[2].specular_strength", specular);
    Room_Shader.setFloat("lights[2].constant", 1.0);
    Room_Shader.setFloat("lights[2].linear", 0.14);
    Room_Shader.setFloat("lights[2].quadratic", 0.07);

    Room_Shader.setVector3f("lights[3].light_pos", lightPositions[3]);
    Room_Shader.setFloat("lights[3].ambient_strength", ambient);
    Room_Shader.setFloat("lights[3].diffuse_strength", diffuse);
    Room_Shader.setFloat("lights[3].specular_strength", specular);
    Room_Shader.setFloat("lights[3].constant", 1.0);
    Room_Shader.setFloat("lights[3].linear", 0.14);
    Room_Shader.setFloat("lights[3].quadratic", 0.07);

    Room_Shader.setVector3f("lights[4].light_pos", lightPositions[4]);
    Room_Shader.setFloat("lights[4].ambient_strength", ambient);
    Room_Shader.setFloat("lights[4].diffuse_strength", diffuse);
    Room_Shader.setFloat("lights[4].specular_strength", specular);
    Room_Shader.setFloat("lights[4].constant", 1.0);
    Room_Shader.setFloat("lights[4].linear", 0.14);
    Room_Shader.setFloat("lights[4].quadratic", 0.07);

    Room_Shader.setVector3f("lights[5].light_pos", lightPositions[5]);
    Room_Shader.setFloat("lights[5].ambient_strength", ambient);
    Room_Shader.setFloat("lights[5].diffuse_strength", diffuse);
    Room_Shader.setFloat("lights[5].specular_strength", specular);
    Room_Shader.setFloat("lights[5].constant", 1.0);
    Room_Shader.setFloat("lights[5].linear", 0.14);
    Room_Shader.setFloat("lights[5].quadratic", 0.07);

    Room_Shader.setVector3f("lights[6].light_pos", lightPositions[6]);
    Room_Shader.setFloat("lights[6].ambient_strength", ambient);
    Room_Shader.setFloat("lights[6].diffuse_strength", diffuse);
    Room_Shader.setFloat("lights[6].specular_strength", specular);
    Room_Shader.setFloat("lights[6].constant", 1.0);
    Room_Shader.setFloat("lights[6].linear", 0.14);
    Room_Shader.setFloat("lights[6].quadratic", 0.07);

    Room_Shader.setVector3f("lights[7].light_pos", lightPositions[7]);
    Room_Shader.setFloat("lights[7].ambient_strength", ambient);
    Room_Shader.setFloat("lights[7].diffuse_strength", diffuse);
    Room_Shader.setFloat("lights[7].specular_strength", specular);
    Room_Shader.setFloat("lights[7].constant", 1.0);
    Room_Shader.setFloat("lights[7].linear", 0.14);
    Room_Shader.setFloat("lights[7].quadratic", 0.07);


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

    //stbi_set_flip_vertically_on_load(true);

    std::string pathToCubeMap = PATH_TO_TEXTURE "/cubemaps/Night/";

    std::map<std::string, GLenum> facesToLoad = {
            {pathToCubeMap + "px.png",GL_TEXTURE_CUBE_MAP_POSITIVE_X},
            {pathToCubeMap + "py.png",GL_TEXTURE_CUBE_MAP_POSITIVE_Y},
            {pathToCubeMap + "pz.png",GL_TEXTURE_CUBE_MAP_POSITIVE_Z},
            {pathToCubeMap + "nx.png",GL_TEXTURE_CUBE_MAP_NEGATIVE_X},
            {pathToCubeMap + "ny.png",GL_TEXTURE_CUBE_MAP_NEGATIVE_Y},
            {pathToCubeMap + "nz.png",GL_TEXTURE_CUBE_MAP_NEGATIVE_Z},
    };
    //load the six faces
    for (std::pair<std::string, GLenum> pair : facesToLoad) {
        loadCubemapFace(pair.first.c_str(), pair.second);
    }


    glfwSwapInterval(1);
	//Rendering
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(window, key_callback);

	while (!glfwWindowShouldClose(window)) {
        processKeyboardCameraInput(window);
		view = camera.GetViewMatrix();
		glfwPollEvents();
        glfwSetKeyCallback(window, key_callback); //Lookout for ALT keypress
		double now = glfwGetTime();
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
        cubeMapShader.setInteger("cubemapTexture", 0);
        cubeMapShader.use();
        cubeMapShader.setMatrix4("V", view);
        cubeMapShader.setMatrix4("P", perspective);
        cubeMapShader.setInteger("cubemapTexture", 0);
        cubeMap.draw();


        Room_Shader.use();
        Room_Shader.setMatrix4("M", room.model);
        Room_Shader.setMatrix4("itM", glm::transpose(glm::inverse(room.model)));
        Room_Shader.setMatrix4("V", view);
        Room_Shader.setMatrix4("P", perspective);
        Room_Shader.setVector3f("u_view_pos", camera.Position);

        glDepthFunc(GL_LEQUAL);
        room.draw();


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