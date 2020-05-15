cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# option(BUILD_COVERAGE "Build coverage" OFF)

set(
  HUNTER_CACHE_SERVERS
  "https://github.com/bmstu-iu8-cpp-sem-3/hunter-cache"
  CACHE STRING "Default cache server"
)
set(CURL_LIBRARY "-lcurl")

include("tools/gate/cmake/HunterGate.cmake")

HunterGate(
  URL "https://github.com/cpp-pm/hunter/archive/v0.23.218.tar.gz"
  SHA1 "9a3594a61227967fb92159339ba326701b287b19"
)

project(producer_consumer)

hunter_add_package(GTest)
hunter_add_package(Boost COMPONENTS program_options
thread filesystem)
find_package(Boost CONFIG REQUIRED program_options
thread filesystem)
find_package(GTest CONFIG REQUIRED)
# find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

add_library(directory_manager INTERFACE
)
add_library(parser INTERFACE
)
add_library(beast INTERFACE
)
add_library(header INTERFACE
)
add_library(producer_consumer INTERFACE
)
add_library(safe_deque INTERFACE
)
add_library(webcrawler INTERFACE
)

add_executable(tests
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.cpp
)
add_executable(work
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/source.cpp
)

target_include_directories(directory_manager
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(parser
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(header
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(webcrawler
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(producer_consumer
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(tests
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(work
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
include_directories(${CURL_INCLUDE_DIR})

target_link_libraries(producer_consumer INTERFACE safe_deque)
target_link_libraries(directory_manager INTERFACE Boost::thread Boost::filesystem beast parser)
target_link_libraries(webcrawler INTERFACE producer_consumer Boost::program_options
Boost::thread Boost::filesystem parser directory_manager ${CURL_LIBRARIES}
)
target_link_libraries(tests GTest::main webcrawler)
target_link_libraries(work webcrawler)
enable_testing()
add_test(NAME unit_tests COMMAND tests)
