/**
 *  FlexSoC Debugger - Hi-speed debug using JTAG/SWD
 * 
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */


module flexsoc_debug
  (
   // Clock domains
   input  CLK,
   input  PHY_CLK,
   input  TRANSPORT_CLK,
   input  RESETn,

   // Host interface
   output UART_TX,
   input  UART_RX,

   // Hardware signals
   output TCK,
   output TDI,
   input  TDO,
   output TMSOUT,
   output TMSOE,
   input  TMSIN
   );

   // Include autogen CSRs
`include "flexdbg_csr.vh"   

   // Import AHB3 constants
   import ahb3lite_pkg::*;
   
   // Dropped bytes due to pipeline backup
   logic [9:0] dropped;

   // Master <=> arbiter
   wire master_RDEN, master_WREN, master_WRFULL, master_RDEMPTY;
   wire [7:0] master_RDDATA, master_WRDATA;
   
   // ARB <=> FIFO
   wire arb_RDEN, arb_WREN, arb_WRFULL, arb_RDEMPTY;
   wire [7:0] arb_RDDATA, arb_WRDATA;

   // FIFO <=> Transport
   wire trans_RDEN, trans_WREN, trans_WRFULL, trans_RDEMPTY;
   wire [7:0] trans_RDDATA, trans_WRDATA;

   // AHB3 master
   logic        host_ahb3_HWRITE, host_ahb3_HREADY, host_ahb3_HRESP;
   logic [1:0]  host_ahb3_HTRANS;
   logic [2:0]  host_ahb3_HSIZE, host_ahb3_HBURST;
   logic [3:0]  host_ahb3_HPROT;
   logic [31:0] host_ahb3_HADDR, host_ahb3_HWDATA, host_ahb3_HRDATA;

   // CSR AHB3 slave
   logic        csr_ahb3_HWRITE, csr_ahb3_HREADY, csr_ahb3_HRESP, csr_ahb3_HSEL, csr_ahb3_HREADYOUT;
   logic [1:0]  csr_ahb3_HTRANS;
   logic [2:0]  csr_ahb3_HSIZE, csr_ahb3_HBURST;
   logic [3:0]  csr_ahb3_HPROT;
   logic [31:0] csr_ahb3_HADDR, csr_ahb3_HWDATA, csr_ahb3_HRDATA;

   // Bridge AHB3 slave
   logic        bridge_ahb3_HRESP, bridge_ahb3_HSEL, bridge_ahb3_HREADYOUT;
   logic [31:0] bridge_ahb3_HRDATA;

   // Basic routing
   // >= 0xF000.0000 = CSR
   // else BRIDGE
   assign csr_ahb3_HSEL = ((host_ahb3_HTRANS == HTRANS_NONSEQ) && (host_ahb3_HADDR >= 32'hF000_0000)) ?
                          1 : 0;
   assign bridge_ahb3_HSEL = ((host_ahb3_HTRANS == HTRANS_NONSEQ) && (host_ahb3_HADDR < 32'hF000_0000)) ?
                             1 : 0;
   // Assign return path to last selection
   logic        csr_sel;
   always @(posedge CLK)
     begin
        if (csr_ahb3_HSEL)
          csr_sel <= 1;
        else
          csr_sel <= 0;
     end

   // AHB3 return path
   assign host_ahb3_HREADY  = csr_sel ? csr_ahb3_HREADYOUT : bridge_ahb3_HREADYOUT;
   assign host_ahb3_HRESP   = csr_sel ? csr_ahb3_HRESP     : bridge_ahb3_HRESP;
   assign host_ahb3_HRDATA  = csr_sel ? csr_ahb3_HRDATA    : bridge_ahb3_HRDATA;

   // CSR forward path
   assign csr_ahb3_HWRITE = host_ahb3_HWRITE;
   assign csr_ahb3_HTRANS = host_ahb3_HTRANS;
   assign csr_ahb3_HSIZE = host_ahb3_HSIZE;
   assign csr_ahb3_HADDR = host_ahb3_HADDR;
   assign csr_ahb3_HBURST = host_ahb3_HBURST;
   assign csr_ahb3_HPROT = host_ahb3_HPROT;
   assign csr_ahb3_HWDATA = host_ahb3_HWDATA;
   assign csr_ahb3_HREADY = host_ahb3_HREADY;

   // Handle CSRs
   assign flexsoc_id = 32'hf1ecdb61;
   
   // Convert host FIFO to AHB3
   ahb3lite_host_master
     u_host_master (
                    .CLK       (CLK),
                    .RESETn    (RESETn),
                    .HADDR     (host_ahb3_HADDR),
                    .HWDATA    (host_ahb3_HWDATA),
                    .HTRANS    (host_ahb3_HTRANS),
                    .HSIZE     (host_ahb3_HSIZE),
                    .HBURST    (host_ahb3_HBURST),
                    .HPROT     (host_ahb3_HPROT),
                    .HWRITE    (host_ahb3_HWRITE),
                    .HRDATA    (host_ahb3_HRDATA),
                    .HRESP     (host_ahb3_HRESP),
                    .HREADY    (host_ahb3_HREADY),
                    .RDEN      (master_RDEN),
                    .RDEMPTY   (master_RDEMPTY),
                    .RDDATA    (master_RDDATA),
                    .WREN      (master_WREN),
                    .WRFULL    (master_WRFULL),
                    .WRDATA    (master_WRDATA)
                    );

   // Arbiter <=> JTAG
   wire host_jtag_RDEN, host_jtag_WREN, host_jtag_WRFULL, host_jtag_RDEMPTY;
   wire [7:0] host_jtag_RDDATA, host_jtag_WRDATA;

   // JTAG <=> DEBUG
   wire jtag_RDEN, jtag_WREN, jtag_WRFULL, jtag_RDEMPTY;
   wire [JTAG_CMD_WIDTH-1:0] jtag_WRDATA;
   wire [JTAG_RESP_WIDTH-1:0] jtag_RDDATA;
   
   // Decapsulate channel B and pass to JTAG interface
   // Encapsulate result and pass to HOST
   host_jtag_convert
     u_host_jtag_direct (
                   .CLK            (CLK),
                   .RESETn         (RESETn),
                   // FIFO interface with host
                   .HOST_RDEN      (host_jtag_RDEN),
                   .HOST_RDEMPTY   (host_jtag_RDEMPTY),
                   .HOST_WREN      (host_jtag_WREN),
                   .HOST_WRFULL    (host_jtag_WRFULL),
                   .HOST_RDDATA    (host_jtag_RDDATA),
                   .HOST_WRDATA    (host_jtag_WRDATA),
                   // Direct FIFO interface
                   .JTAG_RDEN      (jtag_RDEN),
                   .JTAG_RDEMPTY   (jtag_RDEMPTY),
                   .JTAG_WREN      (jtag_WREN),
                   .JTAG_WRFULL    (jtag_WRFULL),
                   .JTAG_RDDATA    (jtag_RDDATA),
                   .JTAG_WRDATA    (jtag_WRDATA)
                   );

    // Debug MUX
   debug_mux
     u_debug (
              .CLK           (CLK),
              .PHY_CLK       (PHY_CLK), // TODO: use clock div
              .RESETn        (RESETn),
              .JTAGnSWD      (1'b0),  // Replace with CSR
              .JTAG_DIRECT   (1'b0),  // Replace with CSR
              // ADIv5 interface
              .ADIv5_WRDATA  (),
              .ADIv5_WREN    (),
              .ADIv5_WRFULL  (),
              .ADIv5_RDDATA  (),
              .ADIv5_RDEN    (),
              .ADIv5_RDEMPTY (),
              // ADIv5 interface
              .JTAG_WRDATA   (jtag_WRDATA),
              .JTAG_WREN     (jtag_WREN),
              .JTAG_WRFULL   (jtag_WRFULL),
              .JTAG_RDDATA   (jtag_RDDATA),
              .JTAG_RDEN     (jtag_RDEN),
              .JTAG_RDEMPTY  (jtag_RDEMPTY),
              // PHY signals
              .TCK           (TCK),
              .TDI           (TDI),
              .TDO           (TDO),
              .TMSOUT        (TMSOUT),
              .TMSOE         (TMSOE),
              .TMSIN         (TMSIN)
              );
   
   // Create fifo arbiter to share connection with host
   fifo_arb #(
              .AW  (4),
              .DW  (8))
   u_fifo_arb (
               .CLK          (CLK),
               .RESETn       (RESETn),
               // Connect to dual clock transport fifo
               .com_rden     (arb_RDEN),
               .com_rdempty  (arb_RDEMPTY),
               .com_rddata   (arb_RDDATA),
               .com_wren     (arb_WREN),
               .com_wrfull   (arb_WRFULL),
               .com_wrdata   (arb_WRDATA),
               // Connect to host_master (selmask matches)
               .c1_rden      (master_RDEN),
               .c1_rdempty   (master_RDEMPTY),
               .c1_rddata    (master_RDDATA),
               .c1_wren      (master_WREN),
               .c1_wrfull    (master_WRFULL),
               .c1_wrdata    (master_WRDATA),
               // Connect to JTAG direct IF
               .c2_rden      (host_jtag_RDEN),
               .c2_rdempty   (host_jtag_RDEMPTY),
               .c2_rddata    (host_jtag_RDDATA),
               .c2_wren      (host_jtag_WREN),
               .c2_wrfull    (host_jtag_WRFULL),
               .c2_wrdata    (host_jtag_WRDATA)
               );
   
   // Arb => Transport
   dual_clock_fifo #(
                     .ADDR_WIDTH   (8),
                     .DATA_WIDTH   (8))
   u_tx_fifo (
              .wr_clk_i   (CLK),
              .rd_clk_i   (TRANSPORT_CLK),
              .rd_rst_i   (~RESETn),
              .wr_rst_i   (~RESETn),
              .wr_en_i    (arb_WREN),
              .wr_data_i  (arb_WRDATA),
              .full_o     (arb_WRFULL),
              .rd_en_i    (trans_RDEN),
              .rd_data_o  (trans_RDDATA),
              .empty_o    (trans_RDEMPTY)
              );

   // Transport => Arb
   dual_clock_fifo #(
                     .ADDR_WIDTH   (8),
                     .DATA_WIDTH   (8))
   u_rx_fifo (
              .rd_clk_i   (CLK),
              .wr_clk_i   (TRANSPORT_CLK),
              .rd_rst_i   (~RESETn),
              .wr_rst_i   (~RESETn),
              .rd_en_i    (arb_RDEN),
              .rd_data_o  (arb_RDDATA),
              .empty_o    (arb_RDEMPTY),
              .wr_en_i    (trans_WREN),
              .wr_data_i  (trans_WRDATA),
              .full_o     (trans_WRFULL)
              );
  
 
   // Host transport
   uart_fifo 
     u_uart (
             .CLK        (TRANSPORT_CLK),
             .RESETn     (RESETn),
             // UART interface
             .TX_PIN     (UART_TX),
             .RX_PIN     (UART_RX),
             // FIFO interface
             .FIFO_WREN  (trans_WREN),
             .FIFO_FULL  (trans_WRFULL),
             .FIFO_DOUT  (trans_WRDATA),
             .FIFO_RDEN  (trans_RDEN),
             .FIFO_EMPTY (trans_RDEMPTY),
             .FIFO_DIN   (trans_RDDATA),
             // Dropped bytes
             .DROPPED    (dropped)
             );

   
endmodule // flexsoc_debug
