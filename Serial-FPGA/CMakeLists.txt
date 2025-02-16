cmake_minimum_required(VERSION 3.30)
project(mingw_cmake_release LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set(CMAKE_CXX_FLAGS "-O3")
# set(CMAKE_CXX_FLAGS_INIT "-O3")
# set(CMAKE_CXX_FLAGS_RELEASE "-O3") 

# Опции для выбора приложения
option(BUILD_FPGA_APP       "Build FPGA application"      ON)
option(BUILD_MAIN_APP       "Build main application"      ON)
option(BUILD_SHIFT_APP      "Build Shift application"     ON)

# Указание статических библиотек
link_directories(${CMAKE_SOURCE_DIR}/lib/static)

configure_file(${CMAKE_SOURCE_DIR}/src/test_input_hex.txt ${CMAKE_BINARY_DIR}/test_input_hex.txt COPYONLY)

# Добавляем поддиректории для библиотек
add_subdirectory(src/lib/AtomicQueue)
add_subdirectory(src/lib/Audio_player_v2)
add_subdirectory(src/lib/Audio_recorder_v2)
add_subdirectory(src/lib/xserial)
add_subdirectory(src/lib/ToneGenerator)

# Общие исходные файлы
set(SHARED_SOURCE_FILES
    src/shared/aes256.cpp
    src/shared/CodeDurationTimer.cpp
    src/shared/file_open_close.cpp
    src/shared/Mode_switcher.cpp
)

# Добавляем зависимости
set(SHARED_DEPENDENCIES
    AtomicQueue
    Audio_player_v2
    Audio_recorder_v2
    xserial
    ToneGenerator
)

###
### Удаление файлов при повторной сборке
###
set(FILE_TO_DELETE "${CMAKE_BINARY_DIR}/fpga_logs.txt") 

add_custom_target( 
    DeleteFiles ALL 
    COMMAND ${CMAKE_COMMAND} -E remove -f ${FILE_TO_DELETE} 
    COMMENT "Delete file: ${FILE_TO_DELETE}"
)  

# FPGA Application
if(BUILD_FPGA_APP)
    add_executable(fpga_app
        ${SHARED_SOURCE_FILES}
        src/cmd/fpga_app/main.cpp
    )
    add_dependencies(fpga_app DeleteFiles)
    target_include_directories(fpga_app PRIVATE
        ${CMAKE_SOURCE_DIR}/src/shared
        ${CMAKE_SOURCE_DIR}/src/cmd/fpga_app
    )
    target_link_libraries(fpga_app PRIVATE
        ${SHARED_DEPENDENCIES}
        dsound_x64
    )
endif()

# Main Application
if(BUILD_MAIN_APP)
    add_executable(main_app
        ${SHARED_SOURCE_FILES}
        src/cmd/main_app/main.cpp
    )
    add_dependencies(fpga_app DeleteFiles)
    target_include_directories(main_app PRIVATE
        ${CMAKE_SOURCE_DIR}/src/shared
    )
    target_link_libraries(main_app PRIVATE
        ${SHARED_DEPENDENCIES}
        dsound_x64
    )
endif()

if(BUILD_SHIFT_APP)
    add_executable(fpga_shift_app
        ${SHARED_SOURCE_FILES}
        src/cmd/fpga_shift_app/main.cpp
    )
    add_dependencies(fpga_shift_app DeleteFiles)
    target_include_directories(fpga_shift_app PRIVATE
        ${CMAKE_SOURCE_DIR}/src/shared
    )
    target_link_libraries(fpga_shift_app PRIVATE
        ${SHARED_DEPENDENCIES}
        dsound_x64
    )
endif()

if(NOT BUILD_FPGA_APP AND NOT BUILD_MAIN_APP AND NOT BUILD_SHIFT_APP)
    message(FATAL_ERROR "Please enable at least one option: BUILD_FPGA_APP or BUILD_MAIN_APP or BUILD_TERMINAL_APP or BUILD_SHIFT_APP.")
endif()