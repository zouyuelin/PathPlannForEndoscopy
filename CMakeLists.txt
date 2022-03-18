cmake_minimum_required(VERSION 3.5)

project(PathPlanning LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Eigen3
find_package( Eigen3 REQUIRED )
include_directories(${EIGEN3_INCLUDE_DIR})
# VMTK
find_package( VMTK REQUIRED)
include(${VMTK_USE_FILE})
# ITK
find_package( ITK REQUIRED)
include(${ITK_USE_FILE})
# VTK
find_package( VTK 8.1.2 EXACT REQUIRED)
include(${VTK_USE_FILE})

set(source main.cpp Centerline.cpp interactorStyleCenterline.cpp)

add_executable(PathPlanning ${source} )

target_link_libraries(PathPlanning ${VTK_LIBRARIES})
