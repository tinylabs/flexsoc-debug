#
# Create target executable for IRQ forwarding
#
add_executable( armv7m_irq_forward armv7m_irq_forward.S )
set_target_properties( armv7m_irq_forward PROPERTIES
  LINK_FLAGS
  "-Wl,-zmax-page-size=4 -T ${CMAKE_SOURCE_DIR}/target/armv7m_irq_forward.ld -Wl,-Map=armv7m_irq_forward.map"
  COMPILE_FLAGS "-Os"
  COMPILE_DEFINITIONS "SPACE_OPTIMIZED=0"
  )
add_custom_command( TARGET armv7m_irq_forward POST_BUILD
  DEPENDS armv7m_irq_forward
  COMMAND arm-none-eabi-objcopy -O binary armv7m_irq_forward armv7m_irq_forward.bin
  COMMAND cmake -E copy $<TARGET_FILE:armv7m_irq_forward>.bin ${CMAKE_SOURCE_DIR}/target/bin
  COMMAND cmake -E copy $<TARGET_FILE:armv7m_irq_forward> ${CMAKE_SOURCE_DIR}/target/bin
  )

add_executable( armv7m_irq_forward_small armv7m_irq_forward.S )
set_target_properties( armv7m_irq_forward_small PROPERTIES
  LINK_FLAGS
  "-Wl,-zmax-page-size=4 -T ${CMAKE_SOURCE_DIR}/target/armv7m_irq_forward.ld -Wl,-Map=armv7m_irq_forward.map"
  COMPILE_FLAGS "-Os"
  COMPILE_DEFINITIONS "SPACE_OPTIMIZED=1"
  )
add_custom_command( TARGET armv7m_irq_forward_small POST_BUILD
  DEPENDS armv7m_irq_forward_small
  COMMAND arm-none-eabi-objcopy -O binary armv7m_irq_forward_small armv7m_irq_forward_small.bin
  COMMAND cmake -E copy $<TARGET_FILE:armv7m_irq_forward_small>.bin ${CMAKE_SOURCE_DIR}/target/bin
  COMMAND cmake -E copy $<TARGET_FILE:armv7m_irq_forward_small> ${CMAKE_SOURCE_DIR}/target/bin
  )

