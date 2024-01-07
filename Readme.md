# Delegating tasks:
## Veronika:
- [NO] Geometry shader explosion of a selected pawn
- [NO] Ray tracing effects
- [x] Pawn selection and movement
- [x] Game logic
- [x] Better lightning

## Igors:
- [x] New cubemap + new floor
- [KIND OF] Bullet library / Physics implementation
- [x] Move shaders from main.cpp to /shaders
- [x] Find a small checkers board / Create small boxes and merge them to represent the board (4x4)
- [x] Mouse scrolling

## Extra
- [X] Different texture for meeples (two colored)
- [X] Reflective/Refractive objects
- [NO] Compute shaders


## Key Inputs
N: iterate through the pawns<br>
F: iterate through white fields on board<br>
A: camera moves to left<br>
D: camera moves to right<br>
W: camera moves forwards<br>
S: camera moves backwards<br>
Keyboard arrows: camera rotation<br>
Enter: moves meeple diagonally to the field selected<br>

## Camera controls
By default, the camera is locked inside the render window. To unlock the camera, for example, to close the window, press L ALT.


# Project structure
## Files
The folder Core contains the project files, including the header and source code cpp files, model folder, shader folder
and texture folder. The main code is mostly concentrated in main.cpp.

## Dependencies
The project depends on glad, glfw, glm, stb libraries, that are included in the 3rdParty folder together with the project
and are linked in th CMakeLists.txt using relative paths. Essentially, the project structure is based on the exercise
repository, used during practicals with a slightly changed structure. 

## Execution
Execute "Core" folder. The corresponding CMakeLists.txt describes the dependencies and executable. There is only on 
executable in the project.