cmake_minimum_required(VERSION 3.25)
project(dice)

set(CMAKE_CXX_STANDARD 17)

# Enable Qt's automatic processing of moc, uic, and rcc files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find the Qt libraries
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

# Add the executable and include all necessary source files
add_executable(dice
        main.cpp
        qt/MainWindow.cpp
        qt/DieVisualization.cpp
        qt/PointsWindow.cpp
        OptimizationThread.cpp
        Vec3.cpp
        PointSphere.cpp
        Die.cpp
        STL.cpp
        PT.cpp
        )

# Include header files
target_include_directories(dice PRIVATE ${PROJECT_SOURCE_DIR})

# Link the Qt libraries
target_link_libraries(dice
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        )


# Platform-specific settings
if(WIN32)
    # Windows-specific libraries (if any)
    # Example: target_link_libraries(dice ws2_32)
elseif(APPLE)
    # macOS-specific frameworks
    target_link_libraries(dice "-framework Cocoa")
    # Optional: Set RPATH to find Qt frameworks at runtime
    set_target_properties(dice PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "@loader_path/../Frameworks"
            )
elseif(UNIX)
    # Linux-specific libraries (if any)
    # Example: target_link_libraries(dice pthread)
endif()