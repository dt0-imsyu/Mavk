cmake_minimum_required(VERSION 3.22)
project(Mavk VERSION 0.1.0 LANGUAGES CXX)

# Reusable native resource/synchronization core. Compatibility/ICD/WSI adapters
# keep their own optional dependencies; this target does not pretend to be an ICD.
find_package(Vulkan REQUIRED)
add_library(mavk-core STATIC core/src/core.cpp)
add_library(Mavk::Core ALIAS mavk-core)
set_target_properties(mavk-core PROPERTIES EXPORT_NAME Core POSITION_INDEPENDENT_CODE ON)
target_compile_features(mavk-core PUBLIC cxx_std_17)
target_include_directories(mavk-core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(mavk-core PUBLIC Vulkan::Vulkan)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
install(TARGETS mavk-core EXPORT MavkTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY core/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT MavkTargets NAMESPACE Mavk:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Mavk)
configure_package_config_file(cmake/MavkConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/MavkConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Mavk)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/MavkConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Mavk)
