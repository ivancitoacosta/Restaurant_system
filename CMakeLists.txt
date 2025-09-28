cmake_minimum_required(VERSION 3.16)

project(RestaurantSystem C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(PkgConfig REQUIRED)

option(USE_GTK4 "Build with GTK4" ON)

if(USE_GTK4)
    pkg_check_modules(GTK REQUIRED gtk4)
    add_compile_definitions(USE_GTK4)
else()
    pkg_check_modules(GTK REQUIRED gtk+-3.0)
endif()

pkg_check_modules(SQLITE3 REQUIRED sqlite3)

include_directories(${GTK_INCLUDE_DIRS} ${SQLITE3_INCLUDE_DIRS} include)
link_directories(${GTK_LIBRARY_DIRS} ${SQLITE3_LIBRARY_DIRS})
add_definitions(${GTK_CFLAGS_OTHER} ${SQLITE3_CFLAGS_OTHER})

file(GLOB_RECURSE APP_SOURCES "src/*.c")

add_executable(restaurant_app ${APP_SOURCES})

target_link_libraries(restaurant_app ${GTK_LIBRARIES} ${SQLITE3_LIBRARIES})

target_compile_options(restaurant_app PRIVATE -Wall -Wextra -pedantic -Werror=implicit-function-declaration)

enable_testing()
add_subdirectory(tests)

install(TARGETS restaurant_app RUNTIME DESTINATION bin)
