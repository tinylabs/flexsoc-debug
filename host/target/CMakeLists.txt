#
# CMake for flexsoc communication library
#


# Use host interface for autogen CSRs
add_definitions( -DHOST_INTERFACE=1 )

# Create target
add_library( target
  Target.cpp
  Debug.cpp
  )

# Enable debug
#set_target_properties( target PROPERTIES COMPILE_FLAGS "-O0 -ggdb" )

# Add dependency to generate CSRs
fusesoc_gencsr( target flexsoc_debug )

# Link to comm layer
target_link_libraries( target log flexsoc )

# Install headers
install( FILES
  PluginTarget.h
  ${CMAKE_BINARY_DIR}/generated/flexdbg_csr.h
  DESTINATION include )
