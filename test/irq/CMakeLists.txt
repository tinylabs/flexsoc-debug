#
# IRQ scanning tests of flexdbg gateware
#

# Add IRQ test lib
add_library( irq irq.c )

# Test application to inject IRQ
add_executable( irq-inject irq-inject.c )
target_link_libraries( irq-inject irq )

# Add tests here...
fusesoc_irq_test( test-remote-irq irq_simple remote-irq.cpp )
