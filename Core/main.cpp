#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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
glm::vec3 cameraPosition = glm::vec3(0.0f, 17.0f, 30.0f);
float aspectRatio = 1.0;
float nearPlane = 0.1f;
float farPlane = 1000.0f;

// Create camera and projection matrix
Camera camera(cameraPosition);
glm::mat4 view = camera.GetViewMatrix();
glm::mat4 perspective = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);

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

//Camera camera(glm::vec3(0.0, 30.0, 60.0));

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

void processSelectedField(GLFWwindow* window, std::vector<std::vector<Object>>& board, std::vector<Object>& meeples) {
	// find the index of the field that is currently selected
	int index_i = getSelectedCube(board).first;
	int index_j = getSelectedCube(board).second;
	// find the position of meeple that is currently selected
	int i_selectedMeeple = getSelectedPawn(meeples);
	//std::cout << i_selectedMeeple << std::endl;
	glm::vec3 selectdMeeple_pos = meeples[i_selectedMeeple].getPos();
	selectdMeeple_pos.y=0;		// compensate for translation of meeples wrt the board in y direction
	//std::cout << selectdMeeple_pos.x << selectdMeeple_pos.y << selectdMeeple_pos.z << std::endl;
	// find cube that corresponds to the position of the selected meeple
	int current_row = 0;
	int current_column = 0;
	std::vector<int> next_row = { 0, 0 };
	int next_column = 0;
	bool flag = false;
	for (int row = 0; row < board.size(); row++) {
		for (int column = 0; column < board[row].size(); column++) {
			if (board[row][column].position == selectdMeeple_pos) {
				//std::cout << row << std::endl;
				//std::cout << board[row][column].position.x << board[row][column].position.y << board[row][column].position.z << std::endl;
				current_row = row;
				current_column = column;
				flag = true;
				break;
			}
		}
		if (flag) {
			next_row[0] = current_row + 1;	// the fields that can be selected for the selected meeple are one row in front of the selected meeple (meeples can only move forward)
			next_row[1] = current_row - 1;
			next_column = current_column + 1;	//todo: use current_column - 1; if meeple is dark colored.
			break;
		}
	}
	/*std::cout << "current row" << current_row << std::endl;
	std::cout << "next row0" << next_row[0] << std::endl;
	std::cout << "next row1" << next_row[1] << std::endl;
	std::cout << "current column"<< current_column << std::endl;
	std::cout << "next column"<< next_column << std::endl;*/
	if (next_row[0] >= 0 && next_row[0] < board.size() && next_row[1] >= 0 && next_row[1] < board.size()) {		// check if both indices of next_row are inside the bounds of the board
		if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fKeyPressed) {
			i_row = (i_row + 1) % 2;	// alternate between the two indices
			board[index_i][index_j].selected = 0.0;	// reset current selected field
			board[next_row[i_row]][next_column].selected = 1.0;
			std::cout << "both" << std::endl;
			fKeyPressed = true;
		}
		else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
			fKeyPressed = false;  // Reset the f key
		}
	}
	else if (next_row[0] >= 0 && next_row[0] < board.size()) {		// if only one index is within the bounds of the board array
		board[index_i][index_j].selected = 0.0;	// reset current selected field
		std::cout << "only index0" << std::endl;
		i_row = next_row[0];
		board[i_row][next_column].selected = 1.0;
	}
	else if (next_row[1] >= 0 && next_row[1] < board.size()) {		// if only one index is within the bounds of the board array
		board[index_i][index_j].selected = 0.0;	// reset current selected field
		std::cout << "only index1" << std::endl;
		i_row = next_row[1];
		board[i_row][next_column].selected = 1.0;
	}
	
	/*if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fKeyPressed) {			// select next field in array board and unselect the current field
		
		int next_i = index_i;
		std::cout << "irow" << i_row << std::endl;



		/*if (i_row < next_row.size() - 1 && next_row[i_row] >= 0 && next_row[i_row] < board.size()) {
			board[next_row[i_row]][next_column].selected = 1.0;
			//std::cout << next_row.size() << std::endl;
			
		}
		/*else if (i_row >= next_row.size() - 1) {
			board[next_row[i_row]][next_column].selected = 1.0;
			std::cout << "test2" << std::endl;
			i_row = 0;
		}*/
		//else if (next_row[i_row] < 0) {	// array out of bounds
		//	i_row = 0;
		//}

		/*
		do {
			if (next_i < board[next_row].size() - 1) {	// iterate through black fields of row
				next_i += 1;
			}
			else {
				next_i = 0;		// start iteration through same row again
			}

		} while (board[next_i][next_row].color != "black");	// find the next black field in the array board

		board[next_i][next_row].selected = 1.0;
		*/
	}


void moveMeeple(GLFWwindow* window, std::vector<std::vector<Object>>& board, std::vector<Object>& meeples) {
	int board_i = getSelectedCube(board).first;
	int board_j = getSelectedCube(board).second;
	int index_pawn = getSelectedPawn(meeples);

	// get position of selected cube
	glm::vec3 cube_pos = board[board_i][board_j].getPos();
	meeples[index_pawn].position = glm::vec3(cube_pos.x, cube_pos.y + 1.2, cube_pos.z);	// update position of meeple
	meeples[index_pawn].model = glm::translate(glm::mat4(1.0f), meeples[index_pawn].position);		// move meeple to the new position

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
    char Debug_Sphere_Fragment_Shader_file[128] = PATH_TO_SHADERS"/Debug_Sphere_Fragment_Shader.frag";
    char Debug_Sphere_Vertex_Shader_file[128] = PATH_TO_SHADERS"/Debug_Sphere_Vertex_Shader.vert";
    Shader Debug_Sphere_Shader = Shader(Debug_Sphere_Vertex_Shader_file, Debug_Sphere_Fragment_Shader_file);
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

    char pathRoom[] = PATH_TO_OBJECTS"/room/room_full.obj";
    Object room(pathRoom);
    room.model = glm::scale(room.model, glm::vec3(0.5, 0.5, 0.5));
    room.position = glm::vec3(7.0, -5.0, 10.0);
    room.model = glm::translate(room.model, room.position);
    room.makeObject(Generic_Shader);

    char pathRoomTexture[] = PATH_TO_TEXTURE"/room/subtle.png";
    GLuint RoomTexture = loadTexture(pathRoomTexture);



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
			field.position = glm::vec3(2.0 * j, 1.0, 2.0 * i);
			field.model = glm::translate(field.model, field.position);
			field.makeObject(Generic_Shader);
			if ((i + j) % 2 == 0) {
				field.color = "white";
			}
			else { field.color = "black"; }
			row.push_back(field);
		}
		board.push_back(row);
	}

	// load and arrange pawns
	/*char path_text_pawn[] = PATH_TO_TEXTURE"/texPawn.jpg";
	GLuint texture_pawn = loadTexture(path_text_pawn);
	char pathPawn[] = PATH_TO_OBJECTS"/pawn.obj";
	/*std::vector<Object> pawns;
	for (int i = 0; i < 4; i++) {
		Object pawn(pathPawn);
		pawn.model = glm::scale(pawn.model, glm::vec3(0.3, 0.3, 0.3));
		pawn.model = glm::rotate(pawn.model, (float)-3.14 / 2, glm::vec3(1.0f, 0.0f, 0.0f));
		pawn.model = glm::translate(pawn.model, glm::vec3(5.0 + 7.5 * i, 8.0, 3.4));
		pawn.makeObject(Generic_Shader);
		pawns.push_back(pawn);
	}
	*/

	// load and arrange meeples
	char Darkmeeple_texturePath[] = PATH_TO_TEXTURE"/meeples/Darkmeeple.jpg";
	GLuint Darkmeeple_texture = loadTexture(Darkmeeple_texturePath);
	char path_meeple[] = PATH_TO_OBJECTS"/meeple.obj";
	std::vector<Object> Darkmeeples;
	for (int i = 0; i < 2; i++) {
		Object Darkmeeple(path_meeple);
		Darkmeeple.color = "dark";
		Darkmeeple.model = glm::translate(Darkmeeple.model, glm::vec3(2.0*i, 2.0, 2.0));
		Darkmeeple.makeObject(Generic_Shader);
		Darkmeeples.push_back(Darkmeeple);
	}
	std::vector<Object> Brightmeeples;
	char Brightmeeple_texturePath[] = PATH_TO_TEXTURE"/meeples/Brightmeeple.jpg";
	GLuint Brightmeeple_texture = loadTexture(Brightmeeple_texturePath);
	for (int i = 0; i < 2; i++) {
		Object Brightmeeple(path_meeple);
		Brightmeeple.color = "bright";
		Brightmeeple.model = glm::translate(Brightmeeple.model, glm::vec3(2.0 * i, 3.0, 2.0));
		Brightmeeple.makeObject(Generic_Shader);
		Brightmeeples.push_back(Brightmeeple);
	}
	//meeple.model = glm::scale(meeple.model, glm::vec3(1.5, 1.5, 1.5));

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


	// light:
    glm::vec3 light_pos = glm::vec3(30.0, 50.0, -20.0);
    glm::vec3 light_col = glm::vec3(1.5, 1.5, 1.5);

    float ambient = 1.0;
	float diffuse = 0.8;
	float specular = 0.8;
	float shininess = 10.0;

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

	std::string pathToCubeMap = PATH_TO_TEXTURE"/cubemaps/Field/";

	std::map<std::string, GLenum> facesToLoad = {
		{pathToCubeMap + "posx.png",GL_TEXTURE_CUBE_MAP_POSITIVE_X},
		{pathToCubeMap + "posy.png",GL_TEXTURE_CUBE_MAP_POSITIVE_Y},
		{pathToCubeMap + "posz.png",GL_TEXTURE_CUBE_MAP_POSITIVE_Z},
		{pathToCubeMap + "negx.png",GL_TEXTURE_CUBE_MAP_NEGATIVE_X},
		{pathToCubeMap + "negy.png",GL_TEXTURE_CUBE_MAP_NEGATIVE_Y},
		{pathToCubeMap + "negz.png",GL_TEXTURE_CUBE_MAP_NEGATIVE_Z},
	};
	//load the six faces
	for (std::pair<std::string, GLenum> pair : facesToLoad) {
		loadCubemapFace(pair.first.c_str(), pair.second);
	}

	// mark first pawn as selected
	//pawns[3].selected = 1.0;
	Brightmeeples[0].selected = 1.0;

	// mark first dark field as selected;
	board[1][0].selected = 1.0;

	glm::vec3 test = board[0][0].getPos();

	//std::cout << test.x << test.y << test.z << std::endl;


	// arrange meeples onto black cubes for initial set up
	/*int index_i = getSelectedCube(board).first;
	int index_j = getSelectedCube(board).second;
	glm::vec3 cube_pos = board[index_i][index_j].getPos();
	meeples[0].position = glm::vec3(cube_pos.x, cube_pos.y + 1.2, cube_pos.z);
	meeples[0].model = glm::translate(glm::mat4(1.0f), meeples[0].position);
	*/


	// place first half of meeples on one side of the board
	int i_meeple = 0;
	for (int i = 0; i < board.size(); i++) {
		for (int j = 0; j < board[i].size(); j++) {
			if (i_meeple == Brightmeeples.size()) {
				break;
			}
			if (board[j][i].color == "black") {
				glm::vec3 cube_pos = board[j][i].getPos();
				Brightmeeples[i_meeple].position = glm::vec3(cube_pos.x, cube_pos.y + 1.2, cube_pos.z);
				Brightmeeples[i_meeple].model = glm::translate(glm::mat4(1.0f), Brightmeeples[i_meeple].position);
				i_meeple += 1;

			}
		}
	}

	// place second half meeples on one side of the board
	i_meeple = 0;
	for (int i = board.size()-1; i >= 0; i--) {
		for (int j = board[i].size()-1; j >= 0; j--) {
			if (i_meeple == Darkmeeples.size()) {
				break;
			}
			if (board[j][i].color == "black") {
				std::cout << i <<j << std::endl;
				glm::vec3 cube_pos = board[j][i].getPos();
				Darkmeeples[i_meeple].position = glm::vec3(cube_pos.x, cube_pos.y + 1.2, cube_pos.z);
				Darkmeeples[i_meeple].model = glm::translate(glm::mat4(1.0f), Darkmeeples[i_meeple].position);
				i_meeple += 1;

			}
		}
	}
	

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
		//processSelected(window, pawns);
		processSelected(window, Brightmeeples);
		processSelectedField(window, board, Brightmeeples);
		view = camera.GetViewMatrix();
		glfwPollEvents();
        glfwSetKeyCallback(window, key_callback); //Lookout for ALT keypress
		double now = glfwGetTime();
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Enter moves meeples to selected cube
		if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS && !enterKeyPressed) {
			moveMeeple(window, board, Brightmeeples);
			enterKeyPressed = true;
		}
		else if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE) {
			enterKeyPressed = false;
		}
		// initialize rendering (send parameters to the shader)
        Generic_Shader.use();
        Generic_Shader.setMatrix4("V", view);
        Generic_Shader.setMatrix4("P", perspective);
        Generic_Shader.setVector3f("light.light_pos", light_pos);
        Generic_Shader.setVector3f("light.light_color", light_col);
        Generic_Shader.setVector3f("u_view_pos", camera.Position);

		for (auto& meeple : Brightmeeples) {
			Generic_Shader.use();
			Generic_Shader.setMatrix4("M", meeple.model);
			//Generic_Shader.setMatrix4("itM", inverseModel);
			Generic_Shader.setInteger("ourTexture", 0);
			Generic_Shader.setFloat("selected", meeple.selected);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, Brightmeeple_texture);
			glDepthFunc(GL_LEQUAL);
			meeple.draw();
		}
		for (auto& meeple : Darkmeeples) {
			Generic_Shader.use();
			Generic_Shader.setMatrix4("M", meeple.model);
			//Generic_Shader.setMatrix4("itM", inverseModel);
			Generic_Shader.setInteger("ourTexture", 0);
			Generic_Shader.setFloat("selected", meeple.selected);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, Darkmeeple_texture);
			glDepthFunc(GL_LEQUAL);
			meeple.draw();
		}

		
		// render the pawns
		/*for (auto& pawn : pawns) {
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
		*/

		// render the board
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				Generic_Shader.use();
				Generic_Shader.setMatrix4("M", board[i][j].model);
				Generic_Shader.setInteger("ourTexture", 0);
				Generic_Shader.setFloat("selected", board[i][j].selected);
				glActiveTexture(GL_TEXTURE0);
				if (board[i][j].color == "white") {
					glBindTexture(GL_TEXTURE_2D, Board_Texture_1); 
				}
				else { 
					glBindTexture(GL_TEXTURE_2D, Board_Texture_2);
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

		Debug_Sphere_Shader.use();
		Debug_Sphere_Shader.setMatrix4("V", view);
		Debug_Sphere_Shader.setMatrix4("P", perspective);
		Debug_Sphere_Shader.setMatrix4("M", sphere3.model);
		//sphere3.draw();

        Generic_Shader.use();
        Generic_Shader.setMatrix4("M", room.model);
        Generic_Shader.setInteger("ourTexture", 0);
        Generic_Shader.setFloat("selected", room.selected);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, RoomTexture);
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
