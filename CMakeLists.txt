cmake_minimum_required(VERSION 3.27)
project(cserver VERSION 1)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS NO)
set(Boost_USE_MULTITHREADED ON) 
find_package(fmt REQUIRED)
find_package(Boost 1.84.0 REQUIRED COMPONENTS url)
find_package(utempl REQUIRED)
find_package(llhttp REQUIRED)
find_package(OpenSSL REQUIRED)

cmake_policy(SET CMP0079 NEW)

include(GNUInstallDirs)
add_library(cserver INTERFACE)
target_include_directories(
  cserver
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(cserver INTERFACE utempl::utempl ${Boost_LIBRARIES} llhttp uring ${OPENSSL_LIBRARIES} )

target_compile_features(cserver INTERFACE cxx_std_20)

install(TARGETS cserver
  EXPORT cserverTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file("cserverConfigVersion.cmake"
                                 VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY SameMajorVersion)

configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/cserverConfig.cmake.in"
  "${PROJECT_BINARY_DIR}/cserverConfig.cmake"
  INSTALL_DESTINATION
  ${CMAKE_INSTALL_DATAROOTDIR}/cserver/cmake)

install(EXPORT cserverTargets
        FILE cserverTargets.cmake
        NAMESPACE cserver::
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cserver/cmake)

install(FILES "${PROJECT_BINARY_DIR}/cserverConfig.cmake"
              "${PROJECT_BINARY_DIR}/cserverConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cserver/cmake)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/cserver DESTINATION include)

