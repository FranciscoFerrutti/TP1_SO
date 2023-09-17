cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(MyProject LANGUAGES C)

# Compilar slave.elf
add_executable(slave.elf slave.c pipe_manager.c)
target_compile_options(slave.elf PRIVATE -Wall -std=c99 -g -D_XOPEN_SOURCE=500)
target_link_libraries(slave.elf PRIVATE m rt pthread)

# Compilar md5
add_executable(md5 md5.c pipe_manager.c)
target_compile_options(md5 PRIVATE -Wall -std=c99 -g -D_XOPEN_SOURCE=500)
target_link_libraries(md5 PRIVATE m rt pthread)

# Compilar vision
add_executable(vision vision.c pipe_manager.c)
target_compile_options(vision PRIVATE -Wall -std=c99 -g -D_XOPEN_SOURCE=500)
target_link_libraries(vision PRIVATE m rt pthread)

# Regla para limpiar
add_custom_target(clean
    COMMAND ${CMAKE_COMMAND} -E remove slave.elf md5 vision resultado.txt
)