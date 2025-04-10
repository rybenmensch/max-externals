cmake_minimum_required(VERSION 3.27)
project(copyfct~ LANGUAGES C)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-pretarget.cmake)
set(CMAKE_OSX_ARCHITECTURES arm64)
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
)

add_library( 
	${PROJECT_NAME} 
	MODULE
	${PROJECT_SRC}
)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
target_compile_options(${PROJECT_NAME} PRIVATE -Wpedantic)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-posttarget.cmake)
