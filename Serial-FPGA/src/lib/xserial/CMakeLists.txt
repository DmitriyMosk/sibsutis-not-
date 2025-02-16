# src/lib/xserial/CMakeLists.txt

set(LD_NAME "xserial")

add_library(${LD_NAME} STATIC
    src/${LD_NAME}.cpp

    #addons 
    src/uart.cpp
)

# тута указываем зависимости
set(LD_DEPENDENCIES 
    AtomicQueue
) 

target_include_directories(${LD_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${LD_NAME} PUBLIC ${LD_DEPENDENCIES}) 