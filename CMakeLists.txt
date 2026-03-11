cmake_minimum_required(VERSION 3.16)
project(dice)

set(CMAKE_CXX_STANDARD 17)

# Enable Qt's automatic processing of moc, uic, and rcc files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find the Qt libraries (prefer Qt6, fall back to Qt5)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

# pthreads (needed by std::thread on Linux; no-op on macOS/Windows)
find_package(Threads REQUIRED)

# Add the executable and include all necessary source files
add_executable(dice
        main.cpp
        qt/MainWindow.cpp
        qt/DieVisualization.cpp
        qt/PointsWindow.cpp
        qt/BuildModelDialog.cpp
        OptimizationThread.cpp
        Vec3.cpp
        PointSphere.cpp
        Die.cpp
        stl/STL.cpp
        stl/Sphere.cpp
        stl/Font.cpp
        stl/Engrave.cpp
        PT.cpp
        )

# Include header files
target_include_directories(dice PRIVATE ${PROJECT_SOURCE_DIR})

# M_PI is not part of the C++ standard; MSVC requires this define to expose it.
# Harmless on GCC/Clang where it is already available.
target_compile_definitions(dice PRIVATE _USE_MATH_DEFINES)

# Link Qt and threading libraries
target_link_libraries(dice
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Threads::Threads
        )

# Platform-specific settings
if(WIN32)
    # Hide the console window for a pure GUI app on Windows.
    # Remove this line if you want to keep the console (useful for debugging).
    set_target_properties(dice PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(APPLE)
    target_link_libraries(dice "-framework Cocoa")
    set_target_properties(dice PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME "Dice Optimizer"
            MACOSX_BUNDLE_BUNDLE_VERSION "1.1"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "1.1"
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "@loader_path/../Frameworks"
            )
elseif(UNIX)
    # Nothing extra needed; Threads::Threads above adds -lpthread.
endif()
