cmake_minimum_required(VERSION 3.20)
project(T-Event VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Автоматизация для Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Тестирование на корневом уровне
enable_testing()


# Поиск Qt
find_package(Qt6 REQUIRED COMPONENTS Core Test)

add_subdirectory(app)
add_subdirectory(test)
