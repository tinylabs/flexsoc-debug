#
# flexsoc-cm3 top-level cmake file
# 
# All rights reserved.
# Tiny Labs Inc
# 2019,2020
#
set( CMAKE_VERSION 3.2 )
cmake_minimum_required( VERSION ${CMAKE_VERSION} )
cmake_policy( VERSION ${CMAKE_VERSION} )
set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake )

# Create project
project( flexsoc-debug )

# Enable languages
enable_language( C CXX ASM )

# Set install if not set
if( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
  set( CMAKE_INSTALL_PREFIX /opt/${CMAKE_PROJECT_NAME} CACHE PATH "" FORCE )
endif()

# Include custom functions
include( Custom )

# Make sure verilator is installed
find_package( verilator REQUIRED )

# Make sure fusesoc is installed
find_package( fusesoc REQUIRED )

# Include common defs
include_directories( common host/flexsoc host/target host/log )

# Include generated CSRs
include_directories( ${CMAKE_BINARY_DIR}/generated )

# Host application
add_subdirectory( host )

# Add gateware wrappers
add_subdirectory( gw )

# Add tests
#add_subdirectory( test )

# Get version from git
execute_process(COMMAND git log --pretty=format:%h -n 1
                OUTPUT_VARIABLE FLEXSOC_VERSION
                ERROR_QUIET)
              
# Configure pkg-config file
get_property( PRIVATE_LIBS GLOBAL PROPERTY PRIVATE_LIBS )
configure_file( pkg-config/${CMAKE_PROJECT_NAME}.pc.in ${CMAKE_PROJECT_NAME}.pc @ONLY )

# Install pkg-config file
install( FILES ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.pc
  DESTINATION /usr/share/pkgconfig )

# Generate package for debian
set( CPACK_GENERATOR "DEB" )
set( CPACK_DEBIAN_PACKAGE_MAINTAINER "elliot@tinylabs.io" )
set( CPACK_DEBIAN_PACKAGE_DEPENDS "libftdi1-dev" )
include( CPack )
