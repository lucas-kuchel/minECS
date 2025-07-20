cmake_minimum_required(VERSION 3.20)

project(minecs VERSION 25.0 LANGUAGES CXX)

add_library(minecs INTERFACE)

target_include_directories(minecs INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(minecs INTERFACE cxx_std_20)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS minecs EXPORT minecsTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT minecsTargets
    FILE minecsTargets.cmake
    NAMESPACE minecs::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/minecs
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/minecsConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/minecsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/minecsConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/minecs
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/minecsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/minecsConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/minecs
)
