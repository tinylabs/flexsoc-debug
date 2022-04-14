#
# Testing framework for flexsoc-debug.
#
# Testing is based on a minimal verilated cortex-m3 implementation
# pulled in from the tinylabs-cores repo
#
# Signals are driven over network sockets to virtually connect
# JTAG/SWD from verilated DUT (flexsoc-debug) to verilated
# remote SoC (cm3_min_soc).
#

# Setup test framework
test_setup ()

# Add tests
add_subdirectory( api )

# Finalize testing
test_finalize ()
