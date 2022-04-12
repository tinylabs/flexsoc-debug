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
   input  PHY_CLKn,
   input  TRANSPORT_CLK,
   input  RESETn,

   // Host interface
   output UART_TX,
   input  UART_RX,

   // Hardware signals
   output TCK,
   output TDI,
   output TMSOUT,
   output TMSOE,
   input  TDO,
   input  TMSIN
   );

   // CSR AHB3 slave
   logic        csr_ahb3_HWRITE, csr_ahb3_HREADY, csr_ahb3_HRESP, csr_ahb3_HSEL, csr_ahb3_HREADYOUT;
   logic [1:0]  csr_ahb3_HTRANS;
   logic [2:0]  csr_ahb3_HSIZE, csr_ahb3_HBURST;
   logic [3:0]  csr_ahb3_HPROT;
   logic [31:0] csr_ahb3_HADDR, csr_ahb3_HWDATA, csr_ahb3_HRDATA;

   // Include autogen CSRs
`include "flexdbg_csr.vh"   

   // Import AHB3 constants
   import ahb3lite_pkg::*;
   import adiv5_pkg::*;
   
   // Dropped bytes due to pipeline backup
   logic [9:0] dropped;

   // Master <=> FIFO
   wire master_RDEN, master_WREN, master_WRFULL, master_RDEMPTY;
   wire [7:0] master_RDDATA, master_WRDATA;
   
   // FIFO <=> Transport
   wire trans_RDEN, trans_WREN, trans_WRFULL, trans_RDEMPTY;
   wire [7:0] trans_RDDATA, trans_WRDATA;

   // AHB3 master
   logic        host_ahb3_HWRITE, host_ahb3_HREADY, host_ahb3_HRESP;
   logic [1:0]  host_ahb3_HTRANS;
   logic [2:0]  host_ahb3_HSIZE, host_ahb3_HBURST;
   logic [3:0]  host_ahb3_HPROT;
   logic [31:0] host_ahb3_HADDR, host_ahb3_HWDATA, host_ahb3_HRDATA;

   // Bridge AHB3 slave
   logic        bridge_ahb3_HRESP, bridge_ahb3_HSEL, bridge_ahb3_HREADYOUT;
   logic [31:0] bridge_ahb3_HRDATA;

   // CSR compound assignments
   logic [ADIv5_CMD_WIDTH-1:0] csr_adiv5_wrdata;
   logic                       csr_adiv5_wren;
   logic                       csr_adiv5_rden;
   logic [2:0]                 csr_adiv5_stat;
                      
   // Bridge ADIv5 FIFO
   logic [ADIv5_CMD_WIDTH-1:0]  brg_adiv5_wrdata;
   logic                        brg_adiv5_wren;
   logic                        brg_adiv5_wrfull;
   logic [ADIv5_RESP_WIDTH-1:0] brg_adiv5_rddata;
   logic                        brg_adiv5_rden;
   logic                        brg_adiv5_rdempty;
   // Bridge side channel output for errors
   logic [2:0]                  brg_adiv5_stat;
   
   // ADIv5 mux
   logic [ADIv5_CMD_WIDTH-1:0]  mux_adiv5_wrdata;
   logic                        mux_adiv5_wren;
   logic                        mux_adiv5_wrfull;
   logic [ADIv5_RESP_WIDTH-1:0] mux_adiv5_rddata;
   logic                        mux_adiv5_rden;
   logic                        mux_adiv5_rdempty;
   
   // ADIv5 CSR logic
   logic [31:0]               adiv5_data;
   logic                      adiv5_ready, adiv5_busy;   
   logic                      adiv5_rden, adiv5_rdempty;
   logic [2:0]                adiv5_stat;

   
   // Basic routing
   // >= 0xF000.0000 = CSR
   // else BRIDGE
   assign csr_ahb3_HSEL = ((host_ahb3_HTRANS == HTRANS_NONSEQ) && (host_ahb3_HADDR >= 32'hF000_0000)) ?
                          1 : 0;
   assign bridge_ahb3_HSEL = ((host_ahb3_HTRANS == HTRANS_NONSEQ) && (host_ahb3_HADDR < 32'hF000_0000)) ?
                             1 : 0;

   // CSR <=> ADIv5 FIFO interface
   assign adiv5_rden = !adiv5_rdempty & !adiv5_ready;
   assign adiv5_status = {adiv5_stat, adiv5_ready, adiv5_busy};
   assign adiv5_data_i = adiv5_ready ? adiv5_data : adiv5_data_o;

   // Feed RW CSRs back
   assign jtag_n_swd_i = jtag_n_swd_o;
   assign clkdiv_i = clkdiv_o;
   assign bridge_en_i = bridge_en_o;
   
   // Assign return path to last selection
   logic        csr_sel;
   always @(posedge CLK)
     begin

        // Select peripheral to route back to AHB3 master
        if (csr_ahb3_HSEL)
          csr_sel <= 1;
        else
          csr_sel <= 0;

        // Set ready when data available
        if (adiv5_rden)
          adiv5_ready <= 1;
        // For WRITE clear on READ status
        else if (!adiv5_cmd[0] & adiv5_status_stb)
          adiv5_ready <= 0;
        // For READ clear on READ data
        else if (adiv5_cmd[0] & adiv5_data_stb)
          adiv5_ready <= 0;
     end

   // Connect CSRs to ADIv5 MUX
   assign csr_adiv5_wrdata = {adiv5_data_o, adiv5_cmd};
   assign csr_adiv5_wren = adiv5_cmd_stb;
   assign adiv5_busy = mux_adiv5_wrfull;
   assign {adiv5_data, csr_adiv5_stat} = mux_adiv5_rddata;
   assign csr_adiv5_rden = adiv5_rden;
   assign adiv5_rdempty = mux_adiv5_rdempty;

   // Connect bridge inputs
   assign brg_adiv5_wrfull = mux_adiv5_wrfull;
   assign brg_adiv5_rddata = mux_adiv5_rddata;
   assign brg_adiv5_rdempty = mux_adiv5_rdempty;
   
   // Mux bridge and CSR regs
   assign mux_adiv5_wrdata = bridge_en_o ? brg_adiv5_wrdata : csr_adiv5_wrdata;
   assign mux_adiv5_wren = bridge_en_o ? brg_adiv5_wren : csr_adiv5_wren;
   assign mux_adiv5_rden = bridge_en_o ? brg_adiv5_rden : csr_adiv5_rden;

   // Mux stat CSR between bridge and FIFO interface
   assign adiv5_stat = bridge_en_o ? brg_adiv5_stat : csr_adiv5_stat;
   
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

   // Generate reset for each clock domain
   logic SYS_RESETn, TRANSPORT_RESETn, PHY_RESETn;
   sync2_pgen
     u_sys_reset (
                  .c (CLK),
                  .d (RESETn),
                  .p (),
                  .q (SYS_RESETn)
                  );
   sync2_pgen
     u_trn_reset (
                  .c (TRANSPORT_CLK),
                  .d (RESETn),
                  .p (),
                  .q (TRANSPORT_RESETn)
                  );
   sync2_pgen
     u_phy_reset (
                  .c (PHY_CLK),
                  .d (RESETn),
                  .p (),
                  .q (PHY_RESETn)
                  );
   
   // Convert host FIFO to AHB3
   ahb3lite_host_master
     u_host_master (
                    .CLK       (CLK),
                    .RESETn    (SYS_RESETn),
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

   // AHB3lite debug bridge
   ahb3lite_debug_bridge
     u_debug_bridge (
                     .CLK           (CLK),
                     .RESETn        (SYS_RESETn),
                     .ENABLE        (bridge_en_o),
                     .STAT          (brg_adiv5_stat),
                     // AHB3 interface
                     .HSEL          (bridge_ahb3_HSEL),
                     .HADDR         (host_ahb3_HADDR),
                     .HWDATA        (host_ahb3_HWDATA),
                     .HTRANS        (host_ahb3_HTRANS),
                     .HSIZE         (host_ahb3_HSIZE),
                     .HBURST        (host_ahb3_HBURST),
                     .HPROT         (host_ahb3_HPROT),
                     .HWRITE        (host_ahb3_HWRITE),
                     .HREADY        (host_ahb3_HREADY),
                     .HRDATA        (bridge_ahb3_HRDATA),
                     .HRESP         (bridge_ahb3_HRESP),
                     .HREADYOUT     (bridge_ahb3_HREADYOUT),
                     // Debug interface
                     .ADIv5_WRDATA  (brg_adiv5_wrdata),
                     .ADIv5_WREN    (brg_adiv5_wren),
                     .ADIv5_WRFULL  (brg_adiv5_wrfull),
                     .ADIv5_RDDATA  (brg_adiv5_rddata),
                     .ADIv5_RDEN    (brg_adiv5_rden),
                     .ADIv5_RDEMPTY (brg_adiv5_rdempty)
                     );

   // Debug MUX
   adiv5_mux
     u_adiv5_mux (
              .CLK           (CLK),
              //.PHY_CLK       (phy_clk_div),
              .SYS_RESETn    (SYS_RESETn),
              .PHY_CLK       (PHY_CLK),
              .PHY_CLKn      (PHY_CLKn),
              .PHY_RESETn    (PHY_RESETn),
              .JTAGnSWD      (jtag_n_swd_o),
              // ADIv5 interface
              .ADIv5_WRDATA  (mux_adiv5_wrdata),
              .ADIv5_WREN    (mux_adiv5_wren),
              .ADIv5_WRFULL  (mux_adiv5_wrfull),
              .ADIv5_RDDATA  (mux_adiv5_rddata),
              .ADIv5_RDEN    (mux_adiv5_rden),
              .ADIv5_RDEMPTY (mux_adiv5_rdempty),
              // PHY signals
              .TCK           (TCK),
              .TDI           (TDI),
              .TDO           (TDO),
              .TMSOUT        (TMSOUT),
              .TMSOE         (TMSOE),
              .TMSIN         (TMSIN)
              );
   
   // AHB3-master => Transport
   dual_clock_fifo #(
                     .ADDR_WIDTH   (8),
                     .DATA_WIDTH   (8))
   u_tx_fifo (
              .wr_clk_i   (CLK),
              .rd_clk_i   (TRANSPORT_CLK),
              .rd_rst_i   (~TRANSPORT_RESETn),
              .wr_rst_i   (~SYS_RESETn),
              .wr_en_i    (master_WREN),
              .wr_data_i  (master_WRDATA),
              .full_o     (master_WRFULL),
              .rd_en_i    (trans_RDEN),
              .rd_data_o  (trans_RDDATA),
              .empty_o    (trans_RDEMPTY)
              );

   // Transport => AHB3-master
   dual_clock_fifo #(
                     .ADDR_WIDTH   (8),
                     .DATA_WIDTH   (8))
   u_rx_fifo (
              .rd_clk_i   (CLK),
              .wr_clk_i   (TRANSPORT_CLK),
              .rd_rst_i   (~SYS_RESETn),
              .wr_rst_i   (~TRANSPORT_RESETn),
              .rd_en_i    (master_RDEN),
              .rd_data_o  (master_RDDATA),
              .empty_o    (master_RDEMPTY),
              .wr_en_i    (trans_WREN),
              .wr_data_i  (trans_WRDATA),
              .full_o     (trans_WRFULL)
              );
  
 
   // Host transport
   uart_fifo 
     u_uart (
             .CLK        (TRANSPORT_CLK),
             .RESETn     (TRANSPORT_RESETn),
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
