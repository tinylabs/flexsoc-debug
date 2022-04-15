#
# API level testing of flexdbg gateware
#

# Add tests here...
fusesoc_api_test( test-connect connect.cpp )
fusesoc_api_test( test-jtag-dp jtag-dp.cpp )
fusesoc_api_test( test-swd-dp swd-dp.cpp )
fusesoc_api_test( test-jtag-ap jtag-ap.cpp )
fusesoc_api_test( test-swd-ap swd-ap.cpp )
fusesoc_api_test( test-jtag-mem-access jtag-mem-access.cpp )
fusesoc_api_test( test-swd-mem-access swd-mem-access.cpp )

# Debug
#add_executable( test-jtag-mem-bridge jtag-mem-bridge.cpp )
#target_link_libraries( test-jtag-mem-bridge flexsoc target )
