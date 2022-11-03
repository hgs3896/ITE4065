cmake_minimum_required(VERSION 3.16)

project(bwtreeplus VERSION 1.0.0)

configure_file(BwtreeConfig.h.in BwtreeConfig.h)

include(CTest)

# C++ settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Options for libraries
option(USE_BWTREE "Use the Bw-tree library" ON)
option(USE_GOOGLE_TEST "Use GoogleTest for testing" ON)

# Bw-tree project library
if(USE_BWTREE)
  add_subdirectory(bwtree)
  list(APPEND EXTRA_LIBS bwtree)
endif()

# GoogleTest
if(USE_GOOGLE_TEST)
  add_subdirectory(test)
endif()

add_executable(${CMAKE_PROJECT_NAME} main.cc)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
list(APPEND EXTRA_LIBS Threads::Threads)

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${EXTRA_LIBS})

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
  "${PROJECT_BINARY_DIR}"
  )

