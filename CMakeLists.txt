cmake_minimum_required(VERSION 3.16)
project(RoadDefectTracker VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 and modules
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Widgets)

# Find OpenCV components
find_package(OpenCV REQUIRED COMPONENTS core imgproc videoio imgcodecs dnn highgui)

# Qt meta-code generation (MOC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Add sources
qt_add_executable(appRoadDefectTracker
    src/main.cpp
    src/videoitem.h
    src/videobackend.h
    src/videobackend.cpp
)

# QML
qt_add_qml_module(appRoadDefectTracker
    URI App
    VERSION 1.0
    QML_FILES qml/Main.qml
)

# Add folders with title files
target_include_directories(appRoadDefectTracker PRIVATE src)

# Libraries
target_link_libraries(appRoadDefectTracker
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Widgets
    ${OpenCV_LIBS}
)
