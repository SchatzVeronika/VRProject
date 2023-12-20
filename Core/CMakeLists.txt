project("Core")

set(CORE "main.cpp" "camera.h" "shader.h")

add_compile_definitions(PATH_TO_OBJECTS="${CMAKE_CURRENT_SOURCE_DIR}/objects")
add_compile_definitions(PATH_TO_TEXTURE="${CMAKE_CURRENT_SOURCE_DIR}/textures")
add_compile_definitions(PATH_TO_SHADERS="${CMAKE_CURRENT_SOURCE_DIR}/shaders")

add_executable(${PROJECT_NAME} ${CORE})
#Specify which libraries you want to use with your executable
target_link_libraries(${PROJECT_NAME} PUBLIC OpenGL::GL glfw glad BulletCollision)
set(BULLET_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../3rdParty/bullet3-3.25/src")
target_include_directories(${PROJECT_NAME} PUBLIC ${BULLET_INCLUDE})
#target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCL_INCLUDE_DIRS})
#target_include_directories(${PROJECT_NAME} PUBLIC ${BULLET_SOURCE_DIR})
#target_include_directories(${PROJECT_NAME} PUBLIC Adder PUBLIC "3rdParty/bullet3-3.25/src")
#target_link_libraries(${PROJECT_NAME} PRIVATE Adder PRIVATE "3rdParty/bullet3-3.25/src")
#target_link_libraries(bullet3-3.25 PRIVATE ${OpenCL_LIBRARIES})
#target_link_libraries(${PROJECT_NAME} "3rdParty/bullet3-3.25/build/libBulletCollision.a" "3rdParty/bullet3-3.25/build/libBulletDynamics.a")
