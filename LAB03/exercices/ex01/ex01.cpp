#include<iostream>

//include glad before GLFW to avoid header conflict or define "#define GLFW_INCLUDE_NONE"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "../../camera.h"
#include "../../shader.h"
#include "../object.h"


const int width = 800;
const int height = 800;


GLuint compileShader(std::string shaderCode, GLenum shaderType);
GLuint compileProgram(GLuint vertexShader, GLuint fragmentShader);
void processInput(GLFWwindow* window);


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

Camera camera(glm::vec3(-1.0, 0.0, 50.0));



int main(int argc, char* argv[])
{
	std::cout << "Welcome to exercice 2: " << std::endl;
	std::cout << "Object reader\n"
		"Make an object reader and load one of the .obj file\n"
		"You can also make your own object in Blender then save it as a .obj \n"
		"You will need to: \n"
		"	- Understand what a Wavefront .obj file contain\n"
		"	- Open the .obj and read the relevant data \n"
		"	- Stock the data in the relevant format  \n"
		"	- Use the relevant buffer (VAO & VBO) to be able to draw your object  \n";

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
	GLFWwindow* window = glfwCreateWindow(width, height, "Solution 02", nullptr, nullptr);
	if (window == NULL)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window\n");
	}

	glfwMakeContextCurrent(window);

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

	const std::string sourceV = "#version 330 core\n"
		// position, tex_coord and normals come from obj file and are progressed in object.h
		"in vec3 position; \n"
		"in vec2 tex_coord; \n"
		"in vec3 normal;"

		"out vec3 v_frag_coord;"
		"out vec3 v_normal;"
		"out vec2 TexCoord; \n"

		"uniform mat4 M; \n"
		"uniform mat4 itM; \n"
		"uniform mat4 V; \n"
		"uniform mat4 P; \n"

		"void main(){ \n"
		"vec4 frag_coord = M* vec4(position, 1.0);\n"
		"gl_Position = P*V*M*vec4(position, 1);\n"
		// transform the normals
		"v_normal = vec3(itM * vec4(normal, 1.0));"
		"v_frag_coord = frag_coord.xyz;"
		"TexCoord = tex_coord; \n"
		"}\n";
	const std::string sourceF = "#version 330 core\n"
		"out vec4 FragColor;"
		"precision mediump float; \n"

		"in vec3 v_frag_coord; \n"
		"in vec3 v_normal; \n"

		"uniform vec3 u_view_pos; \n"

		// establish light
		"struct Light{\n"
		//"vec3 light_color;"
		"vec3 light_pos; \n"
		"float ambient_strength; \n"
		"float diffuse_strength; \n"
		"float specular_strength; \n"
		//attenuation factor
		"float constant;\n"
		"float linear;\n"
		"float quadratic;\n"
		"};\n"
		"uniform Light light;"

		"uniform float shininess; \n"

		"float specularCalculation(vec3 N, vec3 L, vec3 V ){ \n"
		"vec3 R = reflect (-L,N);  \n " //reflect (-L,N) is  equivalent to //max (2 * dot(N,L) * N - L , 0.0) ;
		"float cosTheta = dot(R , V); \n"
		"float spec = pow(max(cosTheta,0.0), shininess); \n"
		"return light.specular_strength * spec;\n"
		"}\n"

		"in vec2 TexCoord; \n"
		"uniform sampler2D ourTexture; \n"
		"void main() { \n"
		"vec3 N = normalize(v_normal);\n"
		"vec3 L = normalize(light.light_pos - v_frag_coord) ; \n"
		"vec3 V = normalize(u_view_pos - v_frag_coord); \n"
		"float specular = specularCalculation( N, L, V); \n"
		"float diffuse = light.diffuse_strength * max(dot(N,L),0.0);\n"
		"float distance = length(light.light_pos - v_frag_coord);"
		"float attenuation = 1 / (light.constant + light.linear * distance + light.quadratic * distance * distance);"
		"float light = light.ambient_strength + attenuation * (diffuse + specular);  \n" // + attenuation * (diffuse + 0.0)
		//"light = light.ambient_strength + attenuation * diffuse;"
		//"float light = light.ambient_strength + attenuation * diffuse;\n"
		//"FragColor = vec4(vec3(texture(ourTexture, TexCoord)) * vec3(light), 1.0); "
		" FragColor = vec4(texture(ourTexture, TexCoord).xyz  * light, 1.0);"
		//"FragColor = vec4(light.light_color, 1.0);"
		//"FragColor = vec4(materialColour * vec3(light), 1.0); \n"
		"} \n";

	// fragment shader for sphere
	const std::string sourceF_sphere = "#version 330 core\n"
		"out vec4 FragColor;"
		"precision mediump float; \n"
		//"in vec4 v_frag_coord; \n"
		"void main() { \n"
		"FragColor = vec4(vec3(1.0,0.0,0.0),1.0); \n"
		"} \n";

	Shader shader(sourceV, sourceF);
	Shader shader_sphere(sourceV, sourceF_sphere);

	char path_text[] = "C:/Users/Vroni/info-h502_202324 - Kopie/LAB03/textures/textureChessBoard.JPG";
	GLuint texture = loadTexture(path_text);

	char path[] = PATH_TO_OBJECTS "/ChessBoard.obj";

	Object board(path);
	board.makeObject(shader);

	// add a sphere in origin for reference
	char path3[] = PATH_TO_OBJECTS"/sphere_smooth.obj";
	Object sphere3(path3);
	sphere3.makeObject(shader);
	sphere3.model = glm::translate(sphere3.model, glm::vec3(0.0, 0.0, 0.0));
	sphere3.model = glm::scale(sphere3.model, glm::vec3(1.5, 1.5, 1.5));


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

	
	glm::mat4 model = glm::mat4(1.0);
	//model = glm::translate(model, glm::vec3(0.5, 0.5, -1.0));
	model = glm::scale(model, glm::vec3(0.5, 0.5, 0.5));
	model = glm::rotate(model, (float)-3.14 / 4, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::vec3 light_pos = glm::vec3(0.0, 4.0, 1.3);
	glm::vec3 light_col = glm::vec3(1.0, 0.0, 0.0);
	

	glm::mat4 inverseModel = glm::transpose(glm::inverse(model));

	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 perspective = camera.GetProjectionMatrix();

	float ambient = 0.5;
	float diffuse = 0.9;
	float specular = 0.4;
	float shininess = 2.0;

	shader.use();
	shader.setFloat("shininess", shininess);
	shader.setFloat("light.ambient_strength", ambient);
	shader.setFloat("light.diffuse_strength", diffuse);
	shader.setFloat("light.specular_strength", specular);
	shader.setFloat("light.constant", 1.0);
	shader.setFloat("light.linear", 0.14);
	shader.setFloat("light.quadratic", 0.07);


	glfwSwapInterval(1);
	//Rendering

	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		view = camera.GetViewMatrix();
		glfwPollEvents();
		double now = glfwGetTime();
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.use();
		shader.setMatrix4("M", model);
		shader.setMatrix4("itM", inverseModel);
		shader.setMatrix4("V", view);
		shader.setMatrix4("P", perspective);
		shader.setInteger("ourTexture", 0);
		shader.setVector3f("light.light_pos", light_pos);
		shader.setVector3f("light.light_color", light_col);
		shader.setVector3f("u_view_pos", camera.Position);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);

		board.draw();

		// sphere
		shader_sphere.use();
		shader_sphere.setMatrix4("V", view);
		shader_sphere.setMatrix4("P", perspective);
		shader_sphere.setMatrix4("itM", inverseModel);
		shader_sphere.setMatrix4("M", sphere3.model);
		//sphere3.draw();

		fps(now);
		glfwSwapBuffers(window);
	}

	//clean up ressource
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
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
