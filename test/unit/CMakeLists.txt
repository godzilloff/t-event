cmake_minimum_required(VERSION 3.20)

project(unit_tests LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

find_package(Qt6 REQUIRED COMPONENTS
    Core Test
    #Gui #Widgets #Sql #SerialPort #Network
)

# Собираем все исходники из src/
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    ../../app/src/serial/*.cpp
    ../../app/src/serial/*.h
)
list(FILTER APP_SOURCES EXCLUDE REGEX "main\\.cpp$")

add_executable(qtest_crc tst_crc.cpp)
add_test(NAME test COMMAND test)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../app/src)
target_link_libraries(qtest_crc PRIVATE Qt6::Test Qt6::Core )

add_executable(qtest_decoder tst_si_decoder.cpp)
#add_test(NAME test COMMAND test)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../app/src)
target_link_libraries(qtest_decoder PRIVATE Qt6::Test Qt6::Core )

# foreach(test_src ${TEST_SOURCES})
#     get_filename_component(test_name ${test_src} NAME_WE)
#     add_executable(${test_name} ${test_src} ${APP_SOURCES})
#     target_include_directories(${test_name} PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}/../../app/src
#     )
#     target_link_libraries(${test_name} PRIVATE Qt6::Core Qt6::Test)
#     add_test(NAME ${test_name} COMMAND ${test_name})
# endforeach()
