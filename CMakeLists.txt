cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
  "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")

project(cserver VERSION 1 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_MODULE_STD 1)
set(Boost_USE_MULTITHREADED ON)
set(FMT_MODULE ON)
set(BOOST_NOT_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED "url;program_options")
set(BOOST_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED "asio;pfr")
set(TRY_BOOST_VERSION "1.85.0")
set(BOOST_INCLUDE_LIBRARIES
    "${BOOST_NOT_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED};${BOOST_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED}"
)
option(BUILD_SHARED_LIBS "Build shared libraries (.so)" ON)
option(BUILD_STATIC_LIBS "Build static libraries (.a)" OFF)
option(CPM_USE_LOCAL_PACKAGES "Use local packages" ON)
option(CSERVER_USE_LIBRESSL "Use LibreSSL" OFF)


set_target_properties(__cmake_cxx23 PROPERTIES CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCES "src/*.cpp")

file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.0/CPM.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
  EXPECTED_HASH
  SHA256=7b354f3a5976c4626c876850c93944e52c83ec59a159ae5de5be7983f0e17a2a
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

set(CMAKE_SKIP_INSTALL_RULES ON)

set(BOOST_URL
  "https://github.com/boostorg/boost/releases/download/boost-${TRY_BOOST_VERSION}/boost-${TRY_BOOST_VERSION}-cmake.tar.xz"
)
CPMAddPackage(
  NAME Boost
  VERSION ${TRY_BOOST_VERSION}
  URL ${BOOST_URL}
  EXCLUDE_FROM_ALL ON
)

set(TMP ${CPM_USE_LOCAL_PACKAGES})
set(CPM_USE_LOCAL_PACKAGES OFF)

CPMAddPackage(
  NAME fmt
  URL
  "https://github.com/linuxnyasha/fmt_import/archive/refs/heads/linuxnyasha-patch-1.zip"
  # Fixes for fmt module and constexpr fmt::formatted_size
  OPTIONS "CMAKE_CXX_FLAGS -DFMT_ATTACH_TO_GLOBAL_MODULE"
)

CPMAddPackage(
  NAME utempl
  URL "https://github.com/linuxnyasha/utempl/archive/refs/heads/main.zip"
  OPTIONS "ENABLE_TESTS OFF" "ENABLE_EXAMPLES OFF"
)

set(CPM_USE_LOCAL_PACKAGES ${TMP})

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
  NAME cserver.serialization
  URL "https://github.com/linuxnyasha/cserver_serialization/archive/refs/heads/main.zip"
  OPTIONS "ENABLE_TESTS OFF" "ENABLE_EXAMPLES OFF"
)

function(configure target)
  target_compile_features(${target} INTERFACE cxx_std_23)

  target_include_directories(${target} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

  target_sources(${target} PUBLIC FILE_SET ${target} TYPE CXX_MODULES
    FILES ${SOURCES})



  install(TARGETS ${target}
    EXPORT utemplTargets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      FILE_SET ${target} DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )


  if(TARGET Boost::pfr)
    target_link_libraries(${target} INTERFACE utempl::utempl fmt::fmt nameof::nameof
      llhttp::llhttp ssl crypto
      Boost::url Boost::program_options Boost::pfr Boost::asio)
  else()
    find_package(Boost 1.85.0 COMPONENTS url program_options REQUIRED)

    target_link_libraries(${target} INTERFACE utempl::utempl fmt::fmt nameof::nameof
      llhttp::llhttp ssl crypto ${Boost_LIBRARIES})
  endif()
endfunction(configure)



cmake_policy(SET CMP0079 NEW)

include(GNUInstallDirs)

if(BUILD_SHARED_LIBS)
  add_library(cserver_shared)
  add_library(cserver::cserver ALIAS cserver_shared)
  configure(cserver_shared)
endif()

if(BUILD_STATIC_LIBS)
  add_library(cserver_static)
  if(NOT BUILD_SHARED_LIBS)
    add_library(cserver::cserver ALIAS cserver_static)
  endif()
  configure(cserver_static)
endif()

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
  CPMAddPackage(
    NAME GTest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  )

  enable_testing()
  file(GLOB_RECURSE SOURCES "tests/*.cpp")
  add_executable(cserver_tests ${SOURCES})
  target_link_libraries(cserver_tests GTest::gtest_main cserver)
  set_property(TARGET cserver_tests PROPERTY CXX_STANDARD 23)
  include(GoogleTest)
  gtest_discover_tests(cserver_tests)
endif()

if(ENABLE_EXAMPLES)
  file(GLOB_RECURSE EXAMPLES_SRC "examples/src/*.cpp")
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
