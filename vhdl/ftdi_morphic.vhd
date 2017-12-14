library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity ftdi_morphic is
   port (-- Inputs
	  clk50     : in  std_logic;       -- 50MHz clock input unused
      rst       : in  std_logic;       -- Active high reset via BD7

      mdata     : inout std_logic_vector(7 downto 0);   -- Port A Data Bus
      mclk60    : in std_logic;
      mrxfn     : in std_logic;
      mtxen     : in std_logic;
      mrdn      : out std_logic;
      mwrn      : out std_logic;
      moen      : out std_logic;
	  msndimm   : out std_logic;
	  io		: inout std_logic_vector(79 downto 0)
);
   end ftdi_morphic;

architecture rtl of ftdi_morphic is

component sync_fifo 
   port(                                 
      reset_n   : in  std_logic;          -- active low reset
--
      s_clk    : in  std_logic;           -- clock from data source
      s_wr     : in  std_logic;           -- source write
      s_txe    : out std_logic;           -- source space available
      s_dbin   : in  std_logic_vector(7 downto 0);  -- source data in
--
      d_clk    : in  std_logic;           -- clock from data destination
      d_rd     : in  std_logic;           -- destination read
      d_rxf    : out std_logic;           -- destination data available
      d_dbout  : out std_logic_vector(7 downto 0)  -- destination data out
   );
end component;

component hs245_sif 
  port(                                 
    clk        : in  std_logic;           -- system clock input
    reset_n    : in  std_logic;           -- active high reset
--
    ext_txe    : in  std_logic;           -- external txe
    ext_rxf    : in  std_logic;           -- 
    ext_wr     : out std_logic;           -- 
    ext_rd     : out std_logic;           -- 
    ext_oe     : out std_logic;           -- 
    ext_datain : in  std_logic_vector(7 downto 0);   -- 
    ext_dataout : out  std_logic_vector(7 downto 0); -- 
--
    int_datain  : in  std_logic_vector(7 downto 0);  -- 
    int_rxf     : in  std_logic;                     -- internal rxf
    int_rd      : out std_logic;                     -- 
--
    int_dataout : out std_logic_vector(7 downto 0);           -- 
    int_txe     : in  std_logic;           -- internal txe
    int_wr      : out std_logic 

    );
end component;

component decoder is
   port (-- Inputs
	  clk50     : in  std_logic;       -- 50MHz clock input
      rst       : in  std_logic;       -- Active high reset

      decin     : in 	std_logic_vector(7 downto 0);
      dec_next	: in	std_logic;
      
      dec_islen : out	std_logic;
      dec_isdata: out	std_logic;
	  cmd		: out	std_logic_vector(4 downto 0);
	  cmd_new	: out	std_logic
  );
end component;

component application is
   port (-- Inputs
	  clk50     	: in  std_logic;       -- 50MHz clock input
      rst       	: in  std_logic;       -- Active high reset

      app_din   	: in 	std_logic_vector(7 downto 0);
      app_din_rdy	: in	std_logic;
      app_din_rd	: out	std_logic;

      app_dout  	: out 	std_logic_vector(7 downto 0);
      app_dout_rdy	: in	std_logic;
      app_dout_wr	: out	std_logic;
      
      dec_next		: out	std_logic;
      dec_islen 	: in	std_logic;
      dec_isdata 	: in	std_logic;
	  cmd			: in	std_logic_vector(4 downto 0);
	  cmd_new		: in	std_logic;

	  io			: inout std_logic_vector(79 downto 0)
  );
end component;


  signal mrxf,mtxe,moe,mrd,mwr : std_logic;
  signal mdatain, mdataout : std_logic_vector(7 downto 0);
  
  signal s1_wr,s1_txe,s2_rd,s2_rxf : std_logic;

  signal reset_n : std_logic;

  signal s1_din, s2_dout : std_logic_vector(7 downto 0);
  signal app_din, app_dout : std_logic_vector(7 downto 0);
  signal app_din_rd, app_din_rdy: std_logic;
  signal app_dout_wr, app_dout_rdy: std_logic;
  
  signal cmd: std_logic_vector(4 downto 0);
  signal cmd_new, dec_islen, dec_isdata, dec_next : std_logic;
  
begin

  
mdata_in:	mdatain <= mdata;
mdata_out: 	mdata <= mdataout when moe='0' else (others => 'Z');
reset_inv:	reset_n <= not rst;
mrxf_inv:	mrxf <= not mrxfn;
mtxe_inv:	mtxe <= not mtxen;
moe_inv:	moen <= not moe;
mrd_inv:	mrdn <= not mrd;
mwr_inv:	mwrn <= not mwr;
msndimm_o:	msndimm <= '1';

sync1 : sync_fifo 
   port map(                                 
      reset_n  => reset_n,

      s_clk    => mclk60,
      s_wr     => s1_wr,
      s_txe    => s1_txe,
      s_dbin   => s1_din,

      d_clk    => clk50,
      d_rd     => app_din_rd,
      d_rxf    => app_din_rdy,
      d_dbout  => app_din
   );

sync2 : sync_fifo 
   port map(                                 
      reset_n  => reset_n,

      s_clk    => clk50,
      s_wr     => app_dout_wr,
      s_txe    => app_dout_rdy,
      s_dbin   => app_dout,

      d_clk    => mclk60,
      d_rd     => s2_rd,
      d_rxf    => s2_rxf,
      d_dbout  => s2_dout
   );

xfer1 : hs245_sif 
  port map(                                 
    clk        => mclk60,
    reset_n    => reset_n,

    ext_txe    => mtxe,
    ext_rxf    => mrxf,
    ext_wr     => mwr,
    ext_rd     => mrd,
    ext_oe     => moe,
    ext_datain => mdatain,
    ext_dataout => mdataout,

    int_datain  => s2_dout,
    int_rxf     => s2_rxf,
    int_rd      => s2_rd,
    int_dataout => s1_din,
    int_txe     => s1_txe,
    int_wr      => s1_wr
 );

dec: decoder
   port map(
	  clk50		=> clk50,
      rst       => rst,

      decin     => app_din,
      dec_next	=> dec_next,
      
      dec_islen => dec_islen,
      dec_isdata => dec_isdata,
	  cmd		=> cmd,
	  cmd_new	=> cmd_new
   );

app: application
   port map (
	  clk50     	=> clk50,
      rst       	=> rst,

      app_din   	=> app_din,
      app_din_rdy	=> app_din_rdy,
      app_din_rd	=> app_din_rd,

      app_dout  	=> app_dout,
      app_dout_rdy	=> app_dout_rdy,
      app_dout_wr	=> app_dout_wr,
      
      dec_next		=> dec_next,
      dec_islen 	=> dec_islen,
      dec_isdata 	=> dec_isdata,
	  cmd			=> cmd,
	  cmd_new		=> cmd_new,

	  io			=> io
  );

end rtl;

