cmake_minimum_required(VERSION 3.10)
project(Approximation_2D)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Найти Qt пакеты
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# Включить заголовки
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Исходные файлы
set(SOURCES
    main.cpp
    renderer.cpp
    window.cpp
    # Другие cpp файлы
)

# Заголовочные файлы
set(HEADERS
    renderer.hpp
    window.hpp
    all_includes.h
    # Другие заголовочные файлы
)

# Создать исполняемый файл
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Линковка с Qt
target_link_libraries(${PROJECT_NAME} Qt5::Widgets)

# Для Debug сборки
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -O0 -g")

# Для Release сборки
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2") 