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
L and N: iterate through the pawns
F: iterate through white fields on board
A: camera moves to left
D: camera moves to right
W: camera moves forwards
S: camera moves backwards
arrows: camera rotation
Enter: moves meeple diagonally to the field selected

## Camera controls
By default, the camera is locked inside the render window. To unlock the camera, for example, to close the window, press L ALT.