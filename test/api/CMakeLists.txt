#
# API level testing of flexdbg gateware
#

# Add tests here...
fusesoc_api_test( test-connect connect.cpp )
fusesoc_api_test( test-jtag-dp jtag-dp.cpp )
fusesoc_api_test( test-swd-dp swd-dp.cpp )
