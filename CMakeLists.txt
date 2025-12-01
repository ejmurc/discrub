cmake_minimum_required(VERSION 3.24)
project(Discrub
  VERSION 0.1.0
  LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if (MSVC)
    set(WARNING_FLAGS /W4 /permissive-)
else()
    set(WARNING_FLAGS -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()

include(FetchContent)
FetchContent_Declare(
  yyjson
  GIT_REPOSITORY https://github.com/ibireme/yyjson.git
  GIT_TAG master
)
FetchContent_MakeAvailable(yyjson)

find_package(OpenSSL REQUIRED)

add_library(discrub_lib
  src/openssl_client.c
)
target_include_directories(discrub_lib PUBLIC include)
target_link_libraries(discrub_lib
  PUBLIC
  OpenSSL::SSL
  OpenSSL::Crypto
  yyjson
)
target_compile_options(discrub_lib PRIVATE ${WARNING_FLAGS})

add_executable(discrub src/main.c)
target_link_libraries(discrub PRIVATE discrub_lib)
target_compile_options(discrub PRIVATE ${WARNING_FLAGS})

