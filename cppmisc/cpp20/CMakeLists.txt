cmake_minimum_required(VERSION 3.10)
project(cppmisc20 LANGUAGES CXX)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK2 REQUIRED gtk+-2.0)

# Define the static library
add_library(libbi STATIC
    libbi/libbi.cc
    libbi/libbi_algebra.cc
)

# Specify include directories for the library (if any)
target_include_directories(libbi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Define the executable
add_executable(braintag
    braintag.cc
    
)

# Link the executable with the static library
target_link_libraries(braintag PRIVATE libbi ${GTK2_LIBRARIES})
target_include_directories(braintag PRIVATE libbi xpm ${GTK2_INCLUDE_DIRS})
target_compile_options(braintag PRIVATE ${GTK2_CFLAGS_OTHER})


