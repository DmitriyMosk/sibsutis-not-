# src/lib/ToneGenerator/CMakeLists.txt

set(LD_NAME "ToneGenerator")

# тута указываем зависимости
set(LD_DEPENDENCIES 
    AtomicQueue
) 

add_library(${LD_NAME} STATIC
    src/${LD_NAME}.cpp
)

target_include_directories(${LD_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${LD_NAME} PUBLIC ${LD_DEPENDENCIES})