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
int i_row = 0;
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

Camera camera(glm::vec3(0.0, 15.0, 20.0));

// some global variables
bool nKeyPressed = false;
bool lKeyPressed = false;
bool fKeyPressed = false;
bool enterKeyPressed = false;
// storing the selected indices
int selectedMeeple_old = 0;

std::string current_Team = "bright";
int dbug_counter = 0;
bool endGame = false;
bool unpermitted_move = false;


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

std::string getCurrentTeam(std::vector<Object>& Brightmeeples, std::vector<Object>& Darkmeeples) {
	std::string currentTeam = "dark";
	for (auto& meeple : Brightmeeples) {
		if (meeple.selected == 1.0) {
			currentTeam = "bright";
			break;
		}
	}
	return currentTeam;
}


void processSelectedMeeple(GLFWwindow* window, std::vector<Object>& Brightmeeples, std::vector<Object>& Darkmeeples) {
	std::string currentTeam = getCurrentTeam(Brightmeeples, Darkmeeples);
	std::vector<Object>& meeples = (currentTeam == "dark") ? Darkmeeples : Brightmeeples;	// choose either dark or bright meeples depending on whos round it is
	// find the index of the piece that is currently selected
	int index = getSelectedPawn(meeples);
	// reset all meeples of the other team:
	if (currentTeam == "dark") {
		for (auto& meeple : Brightmeeples) {
			meeple.selected = 0.0;
		}
	}
	else {
		for (auto& meeple : Darkmeeples) {
			meeple.selected = 0.0;
		}
	}

	/*for (int i = 0; i < meeples.size(); i++) {
		if (meeples[i].selected == 1.0) {
			index = i;
		}
	}*/
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS && !nKeyPressed) {			// select next pawn in array pawns and unselect the current pawn
		/*if (index < meeples.size() - 1 && !meeples[index + 1].boardEnd_reached) {
			meeples[index].selected = 0.0;
			meeples[index + 1].selected = 1.0;
			std::cout << "test1" << std::endl;
		}
		else if (!meeples[0].boardEnd_reached) {
			meeples[0].selected = 1.0;		// start from beginning or array
			std::cout << "test1" << std::endl;
		}*/
		nKeyPressed = true;

		//
		meeples[index].selected = 0.0;
		bool no_new_found = true;
		bool break_it = true;
		while (no_new_found) {
			
			/*
			for (int i = 0; i < meeples.size(); i++) {
				if (!meeples[i].boardEnd_reached)
					break_it = false;

			}

			if (break_it)
				std::cout << "no playable meeple (kings though)" << std::endl;
				break;

			*/
			index++;

			index = index % meeples.size();

			if (!meeples[index].boardEnd_reached) {
				meeples[index].selected = 1.0;
				no_new_found = false;
			}
			//
		}


	}
	
	else if (glfwGetKey(window, GLFW_KEY_N) == GLFW_RELEASE) {
		nKeyPressed = false;  // Reset the n key
	}

}

void processSelectedField(GLFWwindow* window, std::vector<std::vector<Object>>& board, std::vector<Object>& Brightmeeples, std::vector<Object>& Darkmeeples) {
	// find the index of the field that is currently selected
	int index_i = getSelectedCube(board).first;
	int index_j = getSelectedCube(board).second;

	std::vector<Object>& meeples = (current_Team == "dark") ? Darkmeeples : Brightmeeples;	// choose either dark or bright meeples depending on whos round it is
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

			if (meeples[0].color == "bright") {
				next_column = current_column + 1;	
			}

			else if (meeples[0].color == "dark") {
				next_column = current_column - 1;		// iterate through board in other direction
			}
			break;
		}
	}

	// select the value for i_row
	if (i_row != 0 && i_row != 1) {
		i_row = 0;		// reset i_row if out of range
	}
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fKeyPressed) {
		i_row = (i_row + 1) % 2;	// alternate between the two indices
		fKeyPressed = true;

	}
	else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
		fKeyPressed = false;  // Reset the f key
	}

	// select new field depending on current field
	if (next_row[0] >= 0 && next_row[0] < board.size() && next_row[1] >= 0 && next_row[1] < board.size() &&		 // check if both indices of next_row are inside the bounds of the board
		!(board[next_row[i_row]][next_column].occupied && (next_row[i_row] == 0 || next_row[i_row] == board.size() - 1))) {		// check if meeple at edge of board
		board[index_i][index_j].selected = 0.0;	// reset current selected field
		board[next_row[i_row]][next_column].selected = 1.0;
	}
	else if (next_row[0] >= 0 && next_row[0] < board.size()) {		// if only one index is within the bounds of the board array
		board[index_i][index_j].selected = 0.0;	// reset current selected field
		//std::cout << "only index0" << std::endl;
		i_row = next_row[0];
		board[i_row][next_column].selected = 1.0;
	}
	else if (next_row[1] >= 0 && next_row[1] < board.size()) {		// if only one index is within the bounds of the board array
		board[index_i][index_j].selected = 0.0;	// reset current selected field
		//std::cout << "only index1" << std::endl;
		i_row = next_row[1];
		board[i_row][next_column].selected = 1.0;
	}

}


void moveMeeple(GLFWwindow* window, std::vector<std::vector<Object>>& board, std::vector<Object>& Brightmeeples, std::vector<Object>& Darkmeeples) {
	int board_i = getSelectedCube(board).first;
	int board_j = getSelectedCube(board).second;
	std::string currentTeam = getCurrentTeam(Brightmeeples, Darkmeeples);
	std::vector<Object>& meeples = (currentTeam == "dark") ? Darkmeeples : Brightmeeples;	// choose either dark or bright meeples depending on whos round it is

	int index_pawn = getSelectedPawn(meeples);

	// get position of selected cube
	glm::vec3 cube_pos = board[board_i][board_j].getPos();

	// get direction
	std::string direction = "";

	glm::vec3 direction3 = meeples[index_pawn].position - cube_pos;

	if (meeples[index_pawn].position.z > cube_pos.z) {
		direction = "up";
		std::cout << "up" << std::endl;
	}
	if (meeples[index_pawn].position.z < cube_pos.z) {
		direction = "down";
		std::cout << "down" << std::endl;
	}

	// manage occupied fields
	for (int i = 0; i < board.size(); i++) {
		for (int j = 0; j < board[1].size(); j++) {

			// set all to free
			board[i][j].occupied = false;

			// set occuppied where meeple are
			for (int k = 0; k < Brightmeeples.size(); k++) {

				if (Brightmeeples[k].position.x == board[i][j].position.x && Brightmeeples[k].position.z == board[i][j].position.z)
					board[i][j].occupied = true;
			}

			for (int k = 0; k < Darkmeeples.size(); k++) {
				if (Darkmeeples[k].position.x == board[i][j].position.x && Darkmeeples[k].position.z == board[i][j].position.z)
					board[i][j].occupied = true;
			}
		}
	}

	// bool if normal move or not
	bool normal_move = true;

	// field to be moved too
	glm::vec3 field_to_move_to(0.0f, 0.0f, 0.0f);

	bool break_free123 = false;


	// check if kick possible for brightmeeple
	if (current_Team == "bright") {

		//loop through all darkmeeples
		for (int i = 0; i < Darkmeeples.size(); i++) {

			// check if enemy there
			if (Darkmeeples[i].position.x == cube_pos.x && Darkmeeples[i].position.z == cube_pos.z) {
				std::cout << "oioioioi" << std::endl;

				//check if behind is still inside the board
				if (board_i - 1 >= 0 && board_i + 1 < board.size() && board_j - 1 >= 0 && board_j + 1 < board[0].size()) {

					// check if behind free
					if (direction == "up" && !board[board_i - 1][board_j + 1].occupied ||
						direction == "down" && !board[board_i + 1][board_j + 1].occupied) {

						std::cout << "krassomat digga" << std::endl;

						// move piece special
						normal_move = false;

						if (direction == "up") {
							field_to_move_to = board[board_i - 1][board_j + 1].position;

							std::cout << "whopper moved up" << std::endl;
						}
						if (direction == "down") {
							field_to_move_to = board[board_i + 1][board_j + 1].position;

							std::cout << "whopper moved down" << std::endl;
						}

						// remove jumped over piece
						Darkmeeples.erase(Darkmeeples.begin() + i);
						break_free123 = true;


					}
				}

			}		
			
			if (break_free123) {
				std::cout << "breaked for" << std::endl;
				break;
			}
		}
	}

	// check if kick possible for darkmeeple
	if (current_Team == "dark") {

		//loop through all darkmeeples
		for (int i = 0; i < Brightmeeples.size(); i++) {

			// check if enemy there
			if (Brightmeeples[i].position.x == cube_pos.x && Brightmeeples[i].position.z == cube_pos.z) {
				std::cout << "oioioioi" << std::endl;

				//check if behind is still inside the board
				if (board_i - 1 >= 0 && board_i + 1 < board.size() && board_j - 1 >= 0 && board_j + 1 < board[0].size()) {

					// check if behind free
					if (direction == "up" && !board[board_i - 1][board_j - 1].occupied ||
						direction == "down" && !board[board_i + 1][board_j - 1].occupied) {

						std::cout << "krassomat digga" << std::endl;

						// move piece special
						normal_move = false;

						if (direction == "up") {
							field_to_move_to = board[board_i - 1][board_j - 1].position;

							std::cout << "whopper moved up" << std::endl;
						}
						if (direction == "down") {
							field_to_move_to = board[board_i + 1][board_j - 1].position;

							std::cout << "whopper moved down" << std::endl;
						}

						// remove jumped over piece
						Brightmeeples.erase(Brightmeeples.begin() + i);
						break_free123 = true;


					}
				}

			}
				
			if (break_free123) {
				std::cout << "breaked for" << std::endl;
				break;
			}
		}
	}


	if (normal_move)
		field_to_move_to = cube_pos;

	//check if move ends up on top of a piece
	for (auto& Brightmeeple : Brightmeeples) {
		glm::vec3 field_to_move_to_meeple = field_to_move_to;
		field_to_move_to_meeple.y = field_to_move_to_meeple.y + 1.2;

		if (Brightmeeple.position == field_to_move_to_meeple) {
			std::cout << "unpermitted move" << std::endl;
			unpermitted_move = true;
		}
	}

	//check if move ends up on top of a piece
	for (auto& Darkmeeple : Darkmeeples) {
		glm::vec3 field_to_move_to_meeple = field_to_move_to;
		field_to_move_to_meeple.y = field_to_move_to_meeple.y + 1.2;

		if (Darkmeeple.position == field_to_move_to_meeple) {
			std::cout << "unpermitted move" << std::endl;
			unpermitted_move = true;
		}
	}

	if (!unpermitted_move) {

		std::cout << "i moved" << std::endl;

		// normal move
		meeples[index_pawn].position = glm::vec3(field_to_move_to.x, field_to_move_to.y + 1.2, field_to_move_to.z);	// update position of meeple
		meeples[index_pawn].model = glm::translate(glm::mat4(1.0f), meeples[index_pawn].position);		// move meeple to the new position

		// normal end turn check
		if (meeples[index_pawn].position.x >= board[0][7].position.x || meeples[index_pawn].position.x <= board[0][0].position.x) {		// end of board reached
			meeples[index_pawn].boardEnd_reached = true;
			std::cout << "end reached" << std::endl;
		}
	}

}

void checkforwin(GLFWwindow* window, std::vector<Object>& Darkmeeples, std::vector<Object>& Brightmeeples) {
	bool noWinPossible = false;



	// counts all meeples, if all have end reached = true ----> no win possible
	int brightmeeplecounter = 0;
	for (auto& Brightmeeple : Brightmeeples) {
		if (Brightmeeple.boardEnd_reached == true) {

			brightmeeplecounter++;
		}
	}

	if (brightmeeplecounter == Brightmeeples.size())
		noWinPossible = true;

	// counts all meeples, if all have end reached = true ----> no win possible
	int darkmeeplecounter = 0;
	for (auto& Darkmeeple : Darkmeeples) {
		if (Darkmeeple.boardEnd_reached == true) {
			
			darkmeeplecounter++;
		}
	}

	if (darkmeeplecounter == Darkmeeples.size())
		noWinPossible = true;


	// different win states
	if (Darkmeeples.empty()) {
		std::cout << "Team Bright wins" << std::endl;
		endGame = true;
	}
	else if (Brightmeeples.empty()) {
		std::cout << "Team Dark wins" << std::endl;
		endGame = true;
	}
	else if (noWinPossible){
		std::cout << "Stalemate" << std::endl;
		endGame = true;
	}
}

void processnextTurn(GLFWwindow* window, std::vector<Object>& Darkmeeples, std::vector<Object>& Brightmeeples) {
	int currentMeeple = 0;
	// find out which team's turn it is
	//std::string currentTeam = getCurrentTeam(Brightmeeples, Darkmeeples);

	std::cout << "current meeple team: " << current_Team << std::endl;
	//std::cout << "current meeple index: " << currentMeeple << std::endl;

	// unselect the meeple of the current team and select the other teams meeple
	if (current_Team == "bright") {
		//std::cout << "test" << std::endl;
		//Brightmeeples[currentMeeple].selected = 0.0;

		// std::cout << "Dark: boardEnd reached" << Darkmeeples[selectedMeeple_old].boardEnd_reached << std::endl;
		// Darkmeeples[selectedMeeple_old].selected = 1.0;	// mark pawn of last turn red again
		// selectedMeeple_old = currentMeeple;
		
		// fix
		for (int i = 0; i < Brightmeeples.size(); i++) {
				Brightmeeples[i].selected = 0.0;
		}

		for (int i = 0; i < Darkmeeples.size(); i++) {
			if (!Darkmeeples[i].boardEnd_reached) {
				Darkmeeples[i].selected = 1.0;
				break;
			}
		}

		current_Team = "dark";
		// fix end

	}
	else if (current_Team == "dark") {
		// Darkmeeples[currentMeeple].selected = 0.0;
		// std::cout << "Bright: boardEnd reached" << Brightmeeples[selectedMeeple_old].boardEnd_reached << std::endl;
		// Brightmeeples[selectedMeeple_old].selected = 1.0;	// mark pawn of last turn red again
		// selectedMeeple_old = currentMeeple;

		// fix
		// current dark, prep for next white
		for (int i = 0; i < Darkmeeples.size(); i++) {
			Darkmeeples[i].selected = 0.0;
		}

		for (int i = 0; i < Brightmeeples.size(); i++) {
			if (!Brightmeeples[i].boardEnd_reached) {
				Brightmeeples[i].selected = 1.0;
				break;
			}
		}

		current_Team = "bright";
		// fix end
		
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
			// std::cout << "\r FPS: " << fpsCount;
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
	for (int i = 0; i < 8; i++) {
		std::vector<Object> row;
		for (int j = 0; j < 8; j++) {
			Object field(pathBoard);
			field.position = glm::vec3(2.0 * j, 0.0, 2.0 * i);
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
	for (int i = 0; i < 12; i++) {
		Object Darkmeeple(path_meeple);
		Darkmeeple.color = "dark";
		//Darkmeeple.model = glm::translate(Darkmeeple.model, glm::vec3(2.0*i, 2.0, 2.0));
		Darkmeeple.makeObject(Generic_Shader);
		Darkmeeples.push_back(Darkmeeple);
	}
	std::vector<Object> Brightmeeples;
	char Brightmeeple_texturePath[] = PATH_TO_TEXTURE"/meeples/Brightmeeple.jpg";
	GLuint Brightmeeple_texture = loadTexture(Brightmeeple_texturePath);
	for (int i = 0; i < 12; i++) {
		Object Brightmeeple(path_meeple);
		Brightmeeple.color = "bright";
		//Brightmeeple.model = glm::translate(Brightmeeple.model, glm::vec3(2.0 * i, 2.0, 2.0));
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

		// reset unpermitted move global bool
		unpermitted_move = false;

        processKeyboardCameraInput(window);
		//processSelected(window, pawns);

		if (!endGame) {
			processSelectedField(window, board, Brightmeeples, Darkmeeples);
			processSelectedMeeple(window, Brightmeeples, Darkmeeples);
		}


		view = camera.GetViewMatrix();
		glfwPollEvents();
        glfwSetKeyCallback(window, key_callback); //Lookout for ALT keypress
		double now = glfwGetTime();
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (!endGame) {

			// Enter moves meeples to selected cube
			if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS && !enterKeyPressed) {
				moveMeeple(window, board, Brightmeeples, Darkmeeples);

				if (!unpermitted_move) {
					checkforwin(window, Darkmeeples, Brightmeeples);
					if (!endGame) {
						processnextTurn(window, Darkmeeples, Brightmeeples);
					}
				}
				enterKeyPressed = true;
			}
			else if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE) {
				enterKeyPressed = false;
			}

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
		for (int i = 0; i < board.size(); i++) {
			for (int j = 0; j < board.size(); j++) {
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
