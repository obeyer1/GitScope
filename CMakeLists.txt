cmake_minimum_required(VERSION 3.22)

project(GitScope
    VERSION 0.1.0
    DESCRIPTION "Local-only, read-only Git repository visualization client"
    HOMEPAGE_URL "https://github.com/obeyer1/GitScope"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(GITSCOPE_BUILD_TESTS "Build GitScope unit tests" ON)
option(GITSCOPE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGIT2 REQUIRED IMPORTED_TARGET libgit2)

qt_standard_project_setup()

add_library(gitscope_warnings INTERFACE)
if(MSVC)
    target_compile_options(gitscope_warnings INTERFACE /W4)
    if(GITSCOPE_WARNINGS_AS_ERRORS)
        target_compile_options(gitscope_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(gitscope_warnings INTERFACE -Wall -Wextra -Wpedantic)
    if(GITSCOPE_WARNINGS_AS_ERRORS)
        target_compile_options(gitscope_warnings INTERFACE -Werror)
    endif()
endif()

# Core library: libgit2 wrappers and graph layout. Deliberately Qt-free so it
# can be unit-tested without a display server.
add_library(gitscope_core STATIC
    src/git/GitException.h
    src/git/RaiiHandles.h
    src/git/Types.h
    src/git/Repository.h
    src/git/Repository.cpp
    src/git/GraphBuilder.h
    src/git/GraphBuilder.cpp)
target_include_directories(gitscope_core PUBLIC src)
target_link_libraries(gitscope_core
    PUBLIC PkgConfig::LIBGIT2
    PRIVATE gitscope_warnings)

qt_add_executable(gitscope
    src/main.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/CommitTableModel.h
    src/ui/CommitTableModel.cpp
    src/ui/GraphDelegate.h
    src/ui/GraphDelegate.cpp
    src/ui/SummaryDelegate.h
    src/ui/SummaryDelegate.cpp
    src/ui/BranchPanel.h
    src/ui/BranchPanel.cpp
    src/ui/DiffViewer.h
    src/ui/DiffViewer.cpp
    src/ui/DiffHighlighter.h
    src/ui/DiffHighlighter.cpp)
target_link_libraries(gitscope PRIVATE gitscope_core Qt6::Widgets gitscope_warnings)
target_compile_definitions(gitscope PRIVATE GITSCOPE_VERSION="${PROJECT_VERSION}")

set_target_properties(gitscope PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "GitScope"
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.gitscope"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")

install(TARGETS gitscope
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin)

if(GITSCOPE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
