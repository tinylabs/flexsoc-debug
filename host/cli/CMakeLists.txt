#
# Host side flexsoc_cm3 CLI
#

# Create executable
add_executable( flexdbg
  flexdbg.cpp
  main.cpp
  #remote.cpp
  )

# Link to comm lib
target_link_libraries( flexdbg log target dl )
#set_target_properties( flexdbg PROPERTIES COMPILE_FLAGS "-O0 -ggdb" )

# Install into bin
install( TARGETS flexdbg
  DESTINATION bin )

