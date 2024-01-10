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
float lastX = 800.0f / 2.0;
float lastY = 800.0 / 2.0;
int i_row = 0;
float fov = 66.0f;
bool isCursorCaptured = true; // Initially capture the cursor

// Define camera attributes
glm::vec3 cameraPosition = glm::vec3(0.0f, 30.0f, 80.0f);
float aspectRatio = 1.0;
float nearPlane = 0.1f;
float farPlane = 500.0f;

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

// some global variables
bool nKeyPressed = false;
bool fKeyPressed = false;
bool enterKeyPressed = false;

std::string current_Team = "bright";
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
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS && !nKeyPressed) {			// select next pawn in array pawns and unselect the current pawn
		nKeyPressed = true;

		meeples[index].selected = 0.0;
		bool no_new_found = true;
		bool break_it = true;
		while (no_new_found) {
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
    }
    if (meeples[index_pawn].position.z < cube_pos.z) {
        direction = "down";
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
                //check if behind is still inside the board
                if (board_i - 1 >= 0 && board_i + 1 < board.size() && board_j - 1 >= 0 && board_j + 1 < board[0].size()) {
                    // check if behind free
                    if (direction == "up" && !board[board_i - 1][board_j + 1].occupied ||
                        direction == "down" && !board[board_i + 1][board_j + 1].occupied) {
                        // move piece special
                        normal_move = false;
                        if (direction == "up") {
                            field_to_move_to = board[board_i - 1][board_j + 1].position;
                        }
                        if (direction == "down") {
                            field_to_move_to = board[board_i + 1][board_j + 1].position;
                        }
                        // remove jumped over piece
                        Darkmeeples.erase(Darkmeeples.begin() + i);
                        break_free123 = true;
                    }
                }
            }
            if (break_free123) {
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
                //check if behind is still inside the board
                if (board_i - 1 >= 0 && board_i + 1 < board.size() && board_j - 1 >= 0 && board_j + 1 < board[0].size()) {
                    // check if behind free
                    if (direction == "up" && !board[board_i - 1][board_j - 1].occupied ||
                        direction == "down" && !board[board_i + 1][board_j - 1].occupied) {
                        // move piece special
                        normal_move = false;
                        if (direction == "up") {
                            field_to_move_to = board[board_i - 1][board_j - 1].position;
                        }
                        if (direction == "down") {
                            field_to_move_to = board[board_i + 1][board_j - 1].position;
                        }
                        // remove jumped over piece
                        Brightmeeples.erase(Brightmeeples.begin() + i);
                        break_free123 = true;
                    }
                }
            }

            if (break_free123) {
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
        // normal move
        meeples[index_pawn].position = glm::vec3(field_to_move_to.x, field_to_move_to.y + 1.2, field_to_move_to.z);	// update position of meeple
        meeples[index_pawn].model = glm::translate(glm::mat4(1.0f), meeples[index_pawn].position);		// move meeple to the new position
        // normal end turn check
        if (meeples[index_pawn].position.x >= board[0][7].position.x || meeples[index_pawn].position.x <= board[0][0].position.x) {		// end of board reached
            meeples[index_pawn].boardEnd_reached = true;
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
    char Checkers_Fragment_Shader_file[128] = PATH_TO_SHADERS"/Checkers_Fragment_Shader.frag";
    char Checkers_Vertex_Shader_file[128] = PATH_TO_SHADERS"/Checkers_Vertex_Shader.vert";
    Shader Checkers_Shader = Shader(Checkers_Vertex_Shader_file, Checkers_Fragment_Shader_file);
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

// ######## Setup Refractive Shaders ############
    char Globe_Vertex_Shader_file[128] = PATH_TO_SHADERS"/Globe_Vertex_Shader.vert";
    char Globe_Fragment_Shader_file[128] = PATH_TO_SHADERS"/Globe_Fragment_Shader.frag";
    Shader Globe_Shader = Shader(Globe_Vertex_Shader_file, Globe_Fragment_Shader_file);
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
			field.makeObject(Checkers_Shader);
			if ((i + j) % 2 == 0) {
				field.color = "white";
			}
			else { field.color = "black"; }
			row.push_back(field);
		}
		board.push_back(row);
	}

    // load and arrange meeples
    char Darkmeeple_texturePath[] = PATH_TO_TEXTURE"/meeples/Darkmeeple.jpg";
    GLuint Darkmeeple_texture = loadTexture(Darkmeeple_texturePath);
    char path_meeple[] = PATH_TO_OBJECTS"/meeple.obj";
    std::vector<Object> Darkmeeples;
    for (int i = 0; i < 12; i++) {
        Object Darkmeeple(path_meeple);
        Darkmeeple.color = "dark";
        //Darkmeeple.model = glm::translate(Darkmeeple.model, glm::vec3(2.0*i, 2.0, 2.0));
        Darkmeeple.makeObject(Checkers_Shader);
        Darkmeeples.push_back(Darkmeeple);
    }
    std::vector<Object> Brightmeeples;
    char Brightmeeple_texturePath[] = PATH_TO_TEXTURE"/meeples/Brightmeeple.jpg";
    GLuint Brightmeeple_texture = loadTexture(Brightmeeple_texturePath);
    for (int i = 0; i < 12; i++) {
        Object Brightmeeple(path_meeple);
        Brightmeeple.color = "bright";
        //Brightmeeple.model = glm::translate(Brightmeeple.model, glm::vec3(2.0 * i, 2.0, 2.0));
        Brightmeeple.makeObject(Checkers_Shader);
        Brightmeeples.push_back(Brightmeeple);
    }

    char pathRoom[] = PATH_TO_OBJECTS"/room/room_fixed.obj";
    Object room(pathRoom);
    room.makeObject(Room_Shader, false);
    room.model = glm::scale(room.model, glm::vec3(0.99, 0.99, 0.99));
    room.position = glm::vec3(7.0, -5.0, 10.0);
    room.model = glm::translate(room.model, room.position);

    char path_glass_texture[] = PATH_TO_TEXTURE"/glass.jpeg";
    GLuint glass_texture = loadTexture(path_glass_texture);
    char pathGlobe[] = PATH_TO_OBJECTS"/room/globe_relocated.obj";
    Object globe(pathGlobe);
    globe.makeObject(Globe_Shader);
    globe.model = glm::scale(globe.model, glm::vec3(0.99, 0.99, 0.99));
    globe.position = glm::vec3(13.0, 15.0, -78.0);
    globe.model = glm::translate(globe.model, globe.position);


    char pathCube[] = PATH_TO_OBJECTS "/cube.obj";
    Object cubeMap(pathCube);
    cubeMap.makeObject(cubeMapShader);


    //Rendering
	// Room Light:
// Define multiple light positions
    std::vector<glm::vec3> lightPositions = {
            glm::vec3(-8.0f, 65.0f, 20.0f),
            glm::vec3(-8.0f, 65.0f, 20.0f),
            glm::vec3(-8.0f, 65.0f, 60.9f),
            glm::vec3(28.0f, 65.0f, 60.9f),
            glm::vec3(69.0f, 65.0f, 60.9f),
            glm::vec3(69.0f, 65.0f, 20.0f),
            glm::vec3(69.0f, 65.0f, -20.0f),
            glm::vec3(69.0f, 65.0f, -63.0f),
            glm::vec3(69.0f, 65.0f, -111.0f),
    };
    float room_ambient = 0.00001f;
    float room_diffuse = 0.99f;
    float room_specular = 0.0001f;

    glm::vec3 materialColour = glm::vec3(0.5f, 0.5f, 0.5f);

    Room_Shader.use();
    Room_Shader.setFloat("shininess", 0.92f);
    Room_Shader.setVector3f("materialColour", materialColour);

    Room_Shader.setVector3f("lights[0].light_pos", lightPositions[0]);
    Room_Shader.setFloat("lights[0].ambient_strength", room_ambient);
    Room_Shader.setFloat("lights[0].diffuse_strength", room_diffuse);
    Room_Shader.setFloat("lights[0].diffuse_strength", room_specular);
    Room_Shader.setFloat("lights[0].constant", 1.0);
    Room_Shader.setFloat("lights[0].linear", 0.14);
    Room_Shader.setFloat("lights[0].quadratic", 0.07);

    Room_Shader.setVector3f("lights[1].light_pos", lightPositions[1]);
    Room_Shader.setFloat("lights[1].ambient_strength", room_ambient);
    Room_Shader.setFloat("lights[1].diffuse_strength", room_diffuse);
    Room_Shader.setFloat("lights[1].specular_strength", room_specular);
    Room_Shader.setFloat("lights[1].constant", 1.0);
    Room_Shader.setFloat("lights[1].linear", 0.14);
    Room_Shader.setFloat("lights[1].quadratic", 0.07);

    Room_Shader.setVector3f("lights[2].light_pos", lightPositions[2]);
    Room_Shader.setFloat("lights[2].ambient_strength", room_ambient);
    Room_Shader.setFloat("lights[2].diffuse_strength", room_diffuse);
    Room_Shader.setFloat("lights[2].specular_strength", room_specular);
    Room_Shader.setFloat("lights[2].constant", 1.0);
    Room_Shader.setFloat("lights[2].linear", 0.14);
    Room_Shader.setFloat("lights[2].quadratic", 0.07);

    Room_Shader.setVector3f("lights[3].light_pos", lightPositions[3]);
    Room_Shader.setFloat("lights[3].ambient_strength", room_ambient);
    Room_Shader.setFloat("lights[3].diffuse_strength", room_diffuse);
    Room_Shader.setFloat("lights[3].specular_strength", room_specular);
    Room_Shader.setFloat("lights[3].constant", 1.0);
    Room_Shader.setFloat("lights[3].linear", 0.14);
    Room_Shader.setFloat("lights[3].quadratic", 0.07);

    Room_Shader.setVector3f("lights[4].light_pos", lightPositions[4]);
    Room_Shader.setFloat("lights[4].ambient_strength", room_ambient);
    Room_Shader.setFloat("lights[4].diffuse_strength", room_diffuse);
    Room_Shader.setFloat("lights[4].specular_strength", room_specular);
    Room_Shader.setFloat("lights[4].constant", 1.0);
    Room_Shader.setFloat("lights[4].linear", 0.14);
    Room_Shader.setFloat("lights[4].quadratic", 0.07);

    Room_Shader.setVector3f("lights[5].light_pos", lightPositions[5]);
    Room_Shader.setFloat("lights[5].ambient_strength", room_ambient);
    Room_Shader.setFloat("lights[5].diffuse_strength", room_diffuse);
    Room_Shader.setFloat("lights[5].specular_strength", room_specular);
    Room_Shader.setFloat("lights[5].constant", 1.0);
    Room_Shader.setFloat("lights[5].linear", 0.14);
    Room_Shader.setFloat("lights[5].quadratic", 0.07);

    Room_Shader.setVector3f("lights[6].light_pos", lightPositions[6]);
    Room_Shader.setFloat("lights[6].ambient_strength", room_ambient);
    Room_Shader.setFloat("lights[6].diffuse_strength", room_diffuse);
    Room_Shader.setFloat("lights[6].specular_strength", room_specular);
    Room_Shader.setFloat("lights[6].constant", 1.0);
    Room_Shader.setFloat("lights[6].linear", 0.14);
    Room_Shader.setFloat("lights[6].quadratic", 0.07);

    Room_Shader.setVector3f("lights[7].light_pos", lightPositions[7]);
    Room_Shader.setFloat("lights[7].ambient_strength", room_ambient);
    Room_Shader.setFloat("lights[7].diffuse_strength", room_diffuse);
    Room_Shader.setFloat("lights[7].specular_strength", room_specular);
    Room_Shader.setFloat("lights[7].constant", 1.0);
    Room_Shader.setFloat("lights[7].linear", 0.14);
    Room_Shader.setFloat("lights[7].quadratic", 0.07);

    Room_Shader.setVector3f("lights[8].light_pos", lightPositions[8]);
    Room_Shader.setFloat("lights[8].ambient_strength", room_ambient);
    Room_Shader.setFloat("lights[8].diffuse_strength", room_diffuse);
    Room_Shader.setFloat("lights[8].specular_strength", room_specular);
    Room_Shader.setFloat("lights[8].constant", 1.0);
    Room_Shader.setFloat("lights[8].linear", 0.14);
    Room_Shader.setFloat("lights[8].quadratic", 0.07);
    

    // Board light:
    glm::vec3 light_pos = glm::vec3(0.0, 10.0, 1.3);
    glm::vec3 light_col = glm::vec3(1.0, 0.0, 0.0);

    float ambient = 0.8f;
    float diffuse = 0.7f;
    float specular = 0.7f;
    float shininess = 32.0f;

    Checkers_Shader.use();
    Checkers_Shader.setFloat("shininess", shininess);
    Checkers_Shader.setFloat("light.ambient_strength", ambient);
    Checkers_Shader.setFloat("light.diffuse_strength", diffuse);
    Checkers_Shader.setFloat("light.specular_strength", specular);
    Checkers_Shader.setFloat("light.constant", 1.0);
    Checkers_Shader.setFloat("light.linear", 0.14);
    Checkers_Shader.setFloat("light.quadratic", 0.07);

    Globe_Shader.use();
    Globe_Shader.setVector3f("light.position", glm::vec3(13.0, 40.0, -78.0));
    Globe_Shader.setFloat("shininess", 32.0f);
    Globe_Shader.setFloat("light.ambient_strength", 0.3f);
    Globe_Shader.setFloat("light.diffuse_strength", 0.8f);
    Globe_Shader.setFloat("light.specular_strength", 0.9f);
    Globe_Shader.setFloat("light.constant", 1.0);
    Globe_Shader.setFloat("light.linear", 0.14);
    Globe_Shader.setFloat("light.quadratic", 0.07);

//Cubemap loading
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


    // mark first pawn as selected
    Brightmeeples[0].selected = 1.0;

    // mark first dark field as selected;
    board[1][0].selected = 1.0;

    glm::vec3 test = board[0][0].getPos();

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
                //std::cout << i <<j << std::endl;
                glm::vec3 cube_pos = board[j][i].getPos();
                Darkmeeples[i_meeple].position = glm::vec3(cube_pos.x, cube_pos.y + 1.2, cube_pos.z);
                Darkmeeples[i_meeple].model = glm::translate(glm::mat4(1.0f), Darkmeeples[i_meeple].position);
                i_meeple += 1;

            }
        }
    }

    glfwSwapInterval(1);
	//Rendering
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(window, key_callback);

	while (!glfwWindowShouldClose(window)) {
        // reset unpermitted move global bool
        unpermitted_move = false;

        processKeyboardCameraInput(window);

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
        Checkers_Shader.use();
        Checkers_Shader.setMatrix4("V", view);
        Checkers_Shader.setMatrix4("P", perspective);
        Checkers_Shader.setVector3f("light.light_pos", light_pos);
        Checkers_Shader.setVector3f("light.light_color", light_col);
        Checkers_Shader.setVector3f("u_view_pos", camera.Position);

        for (auto& meeple : Brightmeeples) {
            Checkers_Shader.use();
            Checkers_Shader.setMatrix4("M", meeple.model);
            //Checkers_Shader.setMatrix4("itM", inverseModel);
            Checkers_Shader.setInteger("ourTexture", 0);
            Checkers_Shader.setFloat("selected", meeple.selected);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, Brightmeeple_texture);
            glDepthFunc(GL_LEQUAL);
            meeple.draw();
        }
        for (auto& meeple : Darkmeeples) {
            Checkers_Shader.use();
            Checkers_Shader.setMatrix4("M", meeple.model);
            //Checkers_Shader.setMatrix4("itM", inverseModel);
            Checkers_Shader.setInteger("ourTexture", 0);
            Checkers_Shader.setFloat("selected", meeple.selected);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, Darkmeeple_texture);
            glDepthFunc(GL_LEQUAL);
            meeple.draw();
        }

        // render the board
        for (int i = 0; i < board.size(); i++) {
            for (int j = 0; j < board.size(); j++) {
                Checkers_Shader.use();
                Checkers_Shader.setMatrix4("M", board[i][j].model);
                Checkers_Shader.setInteger("ourTexture", 0);
                Checkers_Shader.setFloat("selected", board[i][j].selected);
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

        Room_Shader.use();
        Room_Shader.setMatrix4("M", room.model);
        Room_Shader.setMatrix4("itM", glm::transpose(glm::inverse(room.model)));
        Room_Shader.setMatrix4("V", view);
        Room_Shader.setMatrix4("P", perspective);
        Room_Shader.setVector3f("u_view_pos", camera.Position);
        glDepthFunc(GL_LEQUAL);
        room.draw();

        Globe_Shader.use();
        Globe_Shader.setMatrix4("M", globe.model);
        Globe_Shader.setMatrix4("itM", glm::transpose(glm::inverse(globe.model)));
        Globe_Shader.setMatrix4("V", view);
        Globe_Shader.setMatrix4("P", perspective);
        Globe_Shader.setVector3f("u_view_pos", camera.Position);
        Globe_Shader.setInteger("ourTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, glass_texture);
        glDepthFunc(GL_LEQUAL);
        globe.draw();

        cubeMapShader.use();
        cubeMapShader.setMatrix4("V", view);
        cubeMapShader.setMatrix4("P", perspective);
        cubeMapShader.setInteger("cubemapTexture", 0);
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

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // Adjust fov based on the vertical scroll wheel movement
    fov -= static_cast<float>(yoffset);

    // Clamp the fov within a reasonable range
    if (fov < 1.0f) {
        fov = 1.0f;
    } else if (fov > 120.0f) {
        fov = 120.0f;
    }

    // Recalculate perspective matrix
    perspective = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
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