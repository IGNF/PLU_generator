cmake_minimum_required(VERSION 2.8)
PROJECT(BuildUP CXX)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
SET(CMAKE_BUILD_TYPE "Release")
OPTION(BUILD_WITH_OSG "build BuildUP with OpenSceneGraph for 3D visualization" ON)
OPTION(BUILD_EXAMPLE "build example program" ON)

FIND_PACKAGE( Boost COMPONENTS system filesystem REQUIRED )
FIND_PACKAGE( GDAL REQUIRED)
FIND_PACKAGE( rjmcmc REQUIRED)

INCLUDE_DIRECTORIES(
	include
	${Boost_INCLUDE_DIRS}
	${GDAL_INCLUDE_DIRS}
	${rjmcmc_INCLUDE_DIRS}
)

IF (BUILD_WITH_OSG)
	FIND_PACKAGE(OpenSceneGraph COMPONENTS osg osgGA osgUtil osgViewer OpenThreads REQUIRED)
	INCLUDE_DIRECTORIES(${OPENSCENEGRAPH_INCLUDE_DIRS})
	ADD_DEFINITIONS(-DUSE_OSG)
ENDIF ()

SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# build library
ADD_SUBDIRECTORY(src)

# build example
IF (BUILD_EXAMPLE)
	ADD_SUBDIRECTORY(example)
ENDIF()


