cmake_minimum_required(VERSION 3.20)

project(minECS VERSION 0.1.0 LANGUAGES CXX)

add_library(minECS INTERFACE)

target_include_directories(minECS INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(minECS INTERFACE cxx_std_20)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS minECS EXPORT minECSTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT minECSTargets
    FILE minECSTargets.cmake
    NAMESPACE minECS::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/minECS
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/minECSConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/minECSConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/minECSConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/minECS
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/minECSConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/minECSConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/minECS
)
