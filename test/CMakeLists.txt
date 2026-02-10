cmake_minimum_required(VERSION 3.20)
project(tests LANGUAGES CXX)

add_subdirectory(unit)

# if(ENABLE_INTEGRATION_TESTS)
#     add_subdirectory(integration)
# endif()