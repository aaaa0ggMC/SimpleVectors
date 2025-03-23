cmake_minimum_required(VERSION 3.10)
project(SimpleVectors)

set(CMAKE_CXX_STANDARD 20)

include_directories(./UnlimitedLife-Linux/CDep/headers)
link_directories(./links/)

set(build_dir "${CMAKE_SOURCE_DIR}/CBuild" CACHE PATH "Where to store the built bins.")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${build_dir})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${build_dir})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${build_dir})

aux_source_directory("./src" FFT-SRC)
add_executable(FFT ${FFT-SRC})
target_link_libraries(FFT PRIVATE comdlg32 glew32 glfw3 opengl32 winmm gdi32 user32 kernel32)
target_compile_definitions(FFT PRIVATE GLEW_STATIC)


