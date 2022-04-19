#
# IRQ scanning tests of flexdbg gateware
#

# Add IRQ test lib
add_library( irq irq.c )

# Add tests here...
fusesoc_irq_test( test-remote-irq irq_simple remote-irq.cpp )
