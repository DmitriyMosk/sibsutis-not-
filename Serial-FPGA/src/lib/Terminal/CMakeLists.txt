# src/lib/Terminal/CMakeLists.txt

set(LD_NAME "Terminal")

file(GLOB_RECURSE SRC_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/painter/*.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/terminal_api/windows/*.cpp 
)

add_library(${LD_NAME} STATIC
    ${SRC_FILES}
)

target_include_directories(${LD_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)