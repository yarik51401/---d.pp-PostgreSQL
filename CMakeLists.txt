// для кто использует CMake
cmake_minimum_required(VERSION 3.10) // минимальная версия CMake
project(BotProject) // название проекта

set(CMAKE_CXX_STANDARD 17) // стандарт C++
add_executable(Bot main.cpp) исполняемый файл

find_package(PostgreSQL REQUIRED) // Проверяет наличие установленного пакета PostgreSQL
find_package(dpp REQUIRED) // Проверяет наличие установленного пакета DPP

target_link_libraries(Bot PRIVATE pqxx dpp) // Связывает исполняемый файл
