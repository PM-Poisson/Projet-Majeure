cmake_minimum_required(VERSION 3.10)
project(eroded_bank LANGUAGES CXX)
 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
 
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
 
add_executable(eroded_bank
    main.cpp
    ErodedBankScene.cpp
    ErodedBankScene.h
)
 
target_link_libraries(eroded_bank PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
)
 