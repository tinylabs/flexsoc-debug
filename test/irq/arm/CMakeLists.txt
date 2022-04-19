#
# Compile ARM elf test files
#

function( arm_elf TARGET SOURCES )
  
  # Create executable
  add_executable( ${TARGET} ${SOURCES} ${ARGN} common.S )

  # Link to common lib
  target_link_libraries( ${TARGET} c )

  # Set link properties
  set_target_properties( ${TARGET} PROPERTIES
    LINK_FLAGS
    "-Wl,-zmax-page-size=4 -T ${CMAKE_SOURCE_DIR}/test/irq/arm/linker.ld -Wl,-Map=${TARGET}.map"
    COMPILE_FLAGS
    "-O0 -ggdb" )

  # Copy back to source tree
  add_custom_command( TARGET ${TARGET} POST_BUILD
    DEPENDS ${TARGET}
    COMMENT "Copying ELF back to source tree"
    COMMAND cmake -E copy $<TARGET_FILE:${TARGET}> ${CMAKE_SOURCE_DIR}/test/irq/arm/elf/
    )
endfunction( arm_elf )

# Add test ARM executables here...
arm_elf( irq_simple src/irq_simple.c )
