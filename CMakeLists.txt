cmake_minimum_required(VERSION 3.27)
project(cserver VERSION 1)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS NO)
set(Boost_USE_MULTITHREADED ON)
set(BOOST_NOT_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED "url;program_options")
set(BOOST_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED "asio;pfr")
set(TRY_BOOST_VERSION "1.85.0")
set(BOOST_INCLUDE_LIBRARIES
    "${BOOST_NOT_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED};${BOOST_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED}"
)

option(CPM_USE_LOCAL_PACKAGES "Use local packages" ON)
option(CSERVER_USE_LIBRESSL "Use LibreSSL" OFF)

file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.0/CPM.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
  EXPECTED_HASH
  SHA256=7b354f3a5976c4626c876850c93944e52c83ec59a159ae5de5be7983f0e17a2a
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

set(CMAKE_SKIP_INSTALL_RULES ON)
set(BOOST_SKIP_INSTALL_RULES ON)

set(BOOST_URL
  "https://github.com/boostorg/boost/releases/download/boost-${TRY_BOOST_VERSION}/boost-${TRY_BOOST_VERSION}-cmake.tar.xz"
)
CPMAddPackage(
  NAME Boost
  VERSION ${TRY_BOOST_VERSION}
  URL ${BOOST_URL}
  EXCLUDE_FROM_ALL ON
)

CPMAddPackage("gh:fmtlib/fmt#10.2.1")
CPMAddPackage(
  NAME llhttp
  VERSION 9.2.1
  URL "https://github.com/nodejs/llhttp/archive/refs/tags/release/v9.2.1.tar.gz"
  URL_HASH SHA256=3c163891446e529604b590f9ad097b2e98b5ef7e4d3ddcf1cf98b62ca668f23e
)

CPMAddPackage("gh:Neargye/nameof@0.10.4")

if(CSERVER_USE_LIBRESSL)

set(LIBRESSL_TESTS OFF)
set(LIBRESSL_APP OFF)

CPMAddPackage(
  NAME LibreSSL
  VERSION 3.9.2
  URL "https://github.com/libressl/portable/releases/download/v3.9.2/libressl-3.9.2.tar.gz"
  URL_HASH SHA256=7b031dac64a59eb6ee3304f7ffb75dad33ab8c9d279c847f92c89fb846068f97
)

else()
CPMAddPackage(
  NAME OpenSSL
  VERSION 3.3.1
  URL "https://github.com/openssl/openssl/releases/download/openssl-3.3.1/openssl-3.3.1.tar.gz"
  URL_HASH SHA256=777cd596284c883375a2a7a11bf5d2786fc5413255efab20c50d6ffe6d020b7e
)

endif()

CPMAddPackage(
  NAME utempl
  URL "https://github.com/linuxnyasha/utempl/archive/refs/heads/main.zip"
  OPTIONS "ENABLE_TESTS OFF" "ENABLE_EXAMPLES OFF"
)



cmake_policy(SET CMP0079 NEW)

include(GNUInstallDirs)
add_library(cserver INTERFACE)
target_include_directories(
  cserver
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_features(cserver INTERFACE cxx_std_23)




if(TARGET Boost::asio)
  target_link_libraries(cserver INTERFACE utempl::utempl nameof::nameof
    llhttp::llhttp ssl crypto 
    Boost::url Boost::program_options Boost::pfr Boost::asio)
else()
  find_package(Boost 1.85.0 COMPONENTS url program_options REQUIRED)

  target_link_libraries(cserver INTERFACE utempl::utempl nameof::nameof
    llhttp::llhttp ssl crypto ${Boost_LIBRARIES})
endif()


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
              "${PROJECT_BINARY_DIR}/cmake/check.cpp"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cserver/cmake)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/cserver DESTINATION include)

if(ENABLE_TESTS) 
  find_package(GTest REQUIRED)
  enable_testing()
  file(GLOB SOURCES tests/* tests/*/* tests/*/*/*)
  add_executable(cserver_tests ${SOURCES})
  target_link_libraries(cserver_tests GTest::gtest_main cserver)
  set_property(TARGET cserver_tests PROPERTY CXX_STANDARD 23)
  include(GoogleTest)
  gtest_discover_tests(cserver_tests)
endif()

if(ENABLE_EXAMPLES)
  file(GLOB EXAMPLES_SRC "examples/src/*.cpp")
  foreach(EXAMPLE_SRC ${EXAMPLES_SRC})
      get_filename_component(EXAMPLE_NAME ${EXAMPLE_SRC} NAME_WE)
      if(NOT ${EXAMPLE_NAME} STREQUAL "constexpr_check")
          add_executable(${EXAMPLE_NAME} ${EXAMPLE_SRC})
          target_link_libraries(${EXAMPLE_NAME} cserver)
          set_property(TARGET ${EXAMPLE_NAME} PROPERTY CXX_STANDARD 23)
          set_target_properties(${EXAMPLE_NAME} PROPERTIES
              RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/examples/output/")
      endif()
  endforeach()
endif()

