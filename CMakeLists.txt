cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

##
## PROJECT
## name and version
##
project(cachyos-sysctl-manager
        VERSION 1.0.0
        LANGUAGES CXX)


##
## INCLUDE
##
include(GNUInstallDirs)
include(StandardProjectSettings)
include(CompilerWarnings)
include(EnableCcache)
include(Linker)
include(StaticAnalyzers)
include(Sanitizers)
include(CPM)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

CPMAddPackage(
  NAME fmt
  GITHUB_REPOSITORY fmtlib/fmt
  GIT_TAG 9.1.0
  EXCLUDE_FROM_ALL YES
)
CPMAddPackage(
  NAME range-v3
  GITHUB_REPOSITORY ericniebler/range-v3
  GIT_TAG 4624c63972c6f2c2871c7b87813c42048ddb80ad
  EXCLUDE_FROM_ALL YES
)

find_package(PolkitQt5-1 REQUIRED)

##
## CONFIGURATION
##
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fwhole-program -fuse-linker-plugin")
endif()

# Link this 'library' to set the c++ standard / compile-time options requested
add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_20)

##
## Target
##
add_executable(${PROJECT_NAME}
    src/utils.hpp src/utils.cpp
    src/sysctl_option.hpp src/sysctl_option.cpp
    src/sm-window.hpp src/sm-window.cpp
    src/sm-window.ui
    src/main.cpp
    )

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)
set_project_warnings(project_warnings)

# Add linker configuration
configure_linker(project_options)

# sanitizer options if supported by compiler
enable_sanitizers(project_options)

include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE project_warnings project_options Qt5::Widgets Threads::Threads fmt::fmt range-v3::range-v3)

option(ENABLE_UNITY "Enable Unity builds of projects" OFF)
if(ENABLE_UNITY)
   # Add for any project you want to apply unity builds for
   set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ON)
endif()

install(
   TARGETS ${PROJECT_NAME}
   RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
   PROGRAMS ${CMAKE_SOURCE_DIR}/src/terminal-helper
   DESTINATION ${CMAKE_INSTALL_LIBDIR}/cachyos-sysctl-manager
)

install(
   PROGRAMS ${CMAKE_SOURCE_DIR}/src/rootshell.sh
   DESTINATION ${CMAKE_INSTALL_LIBDIR}/cachyos-sysctl-manager
)

install(
   FILES org.cachyos.cachyos-sysctl-manager.pkexec.policy
   DESTINATION "${POLKITQT-1_POLICY_FILES_INSTALL_DIR}"
)

install(
   FILES cachyos-sysctl-manager.desktop
   DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
install(
   FILES cachyos-sysctl-manager.png
   DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)
