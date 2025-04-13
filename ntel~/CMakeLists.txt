cmake_minimum_required(VERSION 3.27)
project(ntel~)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-pretarget.cmake)
set(CMAKE_OSX_ARCHITECTURES arm64)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_OSX_DEPLOYMENT_TARGET 13.0)

include_directories( 
	"${MAX_SDK_INCLUDES}"
	"${MAX_SDK_MSP_INCLUDES}"
	"${MAX_SDK_JIT_INCLUDES}"
)

file(GLOB PROJECT_SRC
	"*.h"
	"*.c"
	"*.cpp"
	"*.hpp"
)

add_library( 
	${PROJECT_NAME} 
	MODULE
	${PROJECT_SRC}
)

target_compile_options(${PROJECT_NAME} PUBLIC -Wall)
target_compile_options(${PROJECT_NAME} PUBLIC -Wpedantic)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-posttarget.cmake)
