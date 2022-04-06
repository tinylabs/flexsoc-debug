#
# flexsoc-debug top-level cmake file
# 
# All rights reserved.
# Tiny Labs Inc
# 2019-2022
#
set( CMAKE_VERSION 3.2 )
cmake_minimum_required( VERSION ${CMAKE_VERSION} )
cmake_policy( VERSION ${CMAKE_VERSION} )
set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake )

# Create project
project( flexsoc_debug )

# Enable languages
enable_language( C CXX ASM )

# Set install if not set
if( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
  set( CMAKE_INSTALL_PREFIX /opt/${CMAKE_PROJECT_NAME} CACHE PATH "" FORCE )
endif()

if( DEFINED ENV{XILINX_VIVADO} )
  message( STATUS "Found Vivado: $ENV{XILINX_VIVADO}" )
else ()
  message( STATUS "Vivado not found! Run `source /path/to/vivado/settings64.sh` to synthesize" )
endif ()

# Make sure verilator is installed
find_package( verilator REQUIRED )

# Make sure fusesoc is installed
find_package( fusesoc REQUIRED )

# Cmake fusesoc bindings
include( fusesoc )

# Add fusesoc libraries
fusesoc_add_lib( fusesoc-cores https://github.com/fusesoc/fusesoc-cores )
fusesoc_add_lib( elf-loader https://github.com/fusesoc/elf-loader.git )
#fusesoc_add_lib( tinylabs-cores https://github.com/tinylabs/tinylabs-cores )
# Debug with local copy
fusesoc_add_lib( tinylabs-cores /home/elliot/projects/tinylabs-cores )
fusesoc_add_lib( ${CMAKE_PROJECT_NAME} ${PROJECT_SOURCE_DIR}/gw )

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
