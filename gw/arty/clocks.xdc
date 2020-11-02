# Setup global configs
set_property CFGBVS Vcco [current_design]
set_property CONFIG_VOLTAGE 3.3 [current_design]

# System clock
create_clock -add -name clkin -period 10.00 -waveform {0 5} [get_ports { CLK_100M }];

# Set transport clock as async to hclk
set_clock_groups -asynchronous -group transport_clk -group sys_clk -group phy_clk -group phy_clkn
