cmake_minimum_required()

project(IAMATOMIC CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Simply compile all tests as executable
aux_source_directory(test TEST_SRC)
foreach(EXE_FILE ${TEST_SRC})
    get_filename_component(EXE_NAME ${EXE_FILE} NAME_WLE)
    add_executable(${EXE_NAME} ${EXE_FILE})
    target_include_directories(${EXE_NAME} PRIVATE src)
endforeach()
