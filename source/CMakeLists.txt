#Build executable
add_executable( ottsweeper 
	"ottsweeper.cpp" 
)

# Add include directories
target_include_directories(	ottsweeper
	PRIVATE
	../include
	${GLFW3_INCLUDE_DIRS}
	${GLEW_INCLUDE_DIRS}
	${OTTER_INCLUDE_DIRS}
)

# Add linker libraries
target_link_libraries( ottsweeper 
	Ott::OtterCore
	Ott::OtterMath
	Ott::OtterSystem
	${GLFW3_LIBRARY}
	${SOIL_LIBRARY}
	OpenGL::GL
	OpenGL::GLU
	GLEW::GLEW
)
if(WIN32)
	target_link_libraries( ottsweeper
		Winmm
	)
endif()

# Install executable
install(
	TARGETS ottsweeper
	DESTINATION bin
)
