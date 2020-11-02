/**
 *  Top wrapper for FPGA. This is necessary as Verilator doesn't handle INOUT nets currently
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 **/

module arty_top
  (
   input  CLK_100M,
   input  RESETn,
   // UART to host
   input  UART_RX,
   output UART_TX,
   // JTAG/SWD master
   output TCK,
   output TDI,
   input  TDO,
   inout  TMS
   );

   // Clocks and PLL
   logic     transport_clk, sys_clk, phy_clk, phy_clkn;
   logic     tpll_locked, tpll_feedback;
   logic     spll_locked, spll_feedback;

   // Transport PLL
   PLLE2_BASE #(
                .BANDWIDTH ("OPTIMIZED"),
                .CLKFBOUT_MULT (12),
                .CLKOUT0_DIVIDE(25),    // 48MHz HCLK
                .CLKFBOUT_PHASE(0.0),   // Phase offset in degrees of CLKFB, (-360-360)
                .CLKIN1_PERIOD(10.0),   // 100MHz input clock
                .CLKOUT0_DUTY_CYCLE(0.5),
                .CLKOUT0_PHASE(0.0),
                .DIVCLK_DIVIDE(1),    // Master division value , (1-56)
                .REF_JITTER1(0.0),    // Reference input jitter in UI (0.000-0.999)
                .STARTUP_WAIT("FALSE") // Delay DONE until PLL Locks, ("TRUE"/"FALSE")
                ) u_tpll (
                         // Clock outputs: 1-bit (each) output
                         .CLKOUT0(transport_clk),
                         .CLKOUT1(),
                         .CLKOUT2(),
                         .CLKOUT3(),
                         .CLKOUT4(),
                         .CLKOUT5(),
                         .CLKFBOUT(tpll_feedback), // 1-bit output, feedback clock
                         .LOCKED(tpll_locked),
                         .CLKIN1(CLK_100M),
                         .PWRDWN(1'b0),
                         .RST(1'b0),
                         .CLKFBIN(tpll_feedback)    // 1-bit input, feedback clock
                         );

   // System clock
   PLLE2_BASE #(
                .BANDWIDTH ("OPTIMIZED"),
                .CLKFBOUT_MULT (10),
                .CLKFBOUT_PHASE(0.0),    // Phase offset in degrees of CLKFB, (-360-360)
                .CLKOUT0_DIVIDE (5),     // 200MHz
                .CLKOUT1_DIVIDE (5),     // 200MHz
                .CLKOUT2_DIVIDE (5),     // 200MHz
                .CLKIN1_PERIOD(10.0),    // 100MHz input clock
                .CLKOUT0_DUTY_CYCLE(0.5),
                .CLKOUT0_PHASE(0.0),
                .CLKOUT1_DUTY_CYCLE(0.5),
                .CLKOUT1_PHASE(0.0),
                .CLKOUT2_DUTY_CYCLE(0.5), 
                .CLKOUT2_PHASE(180.0),   // phy_clkn shifted 180 degrees
                .DIVCLK_DIVIDE(1),    // Master division value , (1-56)
                .REF_JITTER1(0.0),    // Reference input jitter in UI (0.000-0.999)
                .STARTUP_WAIT("FALSE") // Delay DONE until PLL Locks, ("TRUE"/"FALSE")
                ) u_spll (
                         // Clock outputs: 1-bit (each) output
                         .CLKOUT0(sys_clk),
                         .CLKOUT1(phy_clk),
                         .CLKOUT2(phy_clkn),
                         .CLKOUT3(),
                         .CLKOUT4(),
                         .CLKOUT5(),
                         .CLKFBOUT(spll_feedback), // 1-bit output, feedback clock
                         .LOCKED(spll_locked),
                         .CLKIN1(CLK_100M),
                         .PWRDWN(1'b0),
                         .RST(1'b0),
                         .CLKFBIN(spll_feedback)    // 1-bit input, feedback clock
                         );

   
   // TMS IOMUX
   wire      tmsoe, tmsout, tmsin;
   IOBUF u_tmsio (.IO (TMS), .I (tmsout), .O (tmsin), .T (!tmsoe));

   // Instantiate flexsoc_debug
   flexsoc_debug
     u_flexsoc_dbg (
                    .CLK            (sys_clk),
                    .PHY_CLK        (phy_clk),
                    .PHY_CLKn       (phy_clkn),
                    .TRANSPORT_CLK  (transport_clk),
                    .RESETn         (RESETn & tpll_locked & spll_locked),
                    .UART_TX        (UART_TX),
                    .UART_RX        (UART_RX),
                    .TCK            (TCK),
                    .TDI            (TDI),
                    .TDO            (TDO),
                    .TMSOUT         (tmsout),
                    .TMSIN          (tmsin),
                    .TMSOE          (tmsoe)
                    );
   
               
endmodule // arty_top
