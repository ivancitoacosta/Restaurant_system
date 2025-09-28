file(GLOB TEST_SOURCES "*.c")
add_executable(restaurant_tests ${TEST_SOURCES})

target_link_libraries(restaurant_tests ${GTK_LIBRARIES} ${SQLITE3_LIBRARIES})
target_include_directories(restaurant_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_sources(restaurant_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/util/config.c
    ${CMAKE_SOURCE_DIR}/src/util/hash.c
)

target_compile_options(restaurant_tests PRIVATE -Wall -Wextra -pedantic)

add_test(NAME restaurant_tests COMMAND restaurant_tests)
