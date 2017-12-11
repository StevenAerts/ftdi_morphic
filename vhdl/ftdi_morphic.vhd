library ieee;
use ieee.std_logic_1164.all;

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

  signal mrxf,mtxe,moe,mrd,mwr : std_logic;
  signal mdatain, mdataout : std_logic_vector(7 downto 0);
  
  signal s1_wr,s1_txe,s1_rd,s1_rxf,s1_mrd : std_logic;
  signal s2_wr,s2_txe,s2_rd,s2_rxf,s2_mrd : std_logic;

  signal reset_n : std_logic;

  signal int_mdatain, int_mdataout : std_logic_vector(7 downto 0);
  signal int_adatain, int_adataout : std_logic_vector(7 downto 0);

  signal io_in, io_out, io_oe: std_logic_vector(79 downto 0);

begin

io_in <= io;
io_out(79 downto 1) <= (others =>'0');
io_oe <= (0=>'1', others => '0');
io_gen:	for i in io'range generate
	io(i) <= io_out(i) when io_oe(i) = '1' else 'Z';
end generate io_gen;
  
mdata_in:	mdatain <= mdata;
mdata_out: 	mdata <= mdataout when moe='0' else (others => 'Z');
reset_inv:	reset_n <= not rst; -- polarity to programme over USB
mrxf_inv:	mrxf <= not mrxfn;
mtxe_inv:	mtxe <= not mtxen;
moe_inv:	moen <= not moe;
mrd_inv:	mrdn <= not mrd;
mwr_inv:	mwrn <= not mwr;
msndimm_o:	msndimm <= '1';

dummy_proc: process(clk50)
begin
  if rising_edge(clk50) then
	--if rst = '1' then
	io_out(0) <= io_in(1);
  end if;
end process dummy_proc;

sync1 : sync_fifo 
   port map(                                 
      reset_n  => reset_n,

      s_clk    => mclk60,
      s_wr     => s1_wr,
      s_txe    => s1_txe,
      s_dbin   => int_mdatain,

      d_clk    => mclk60,
      d_rd     => s1_rd,
      d_rxf    => s1_rxf,
      d_dbout  => int_adatain
   );

sync2 : sync_fifo 
   port map(                                 
      reset_n  => reset_n,

      s_clk    => mclk60,
      s_wr     => s2_wr,
      s_txe    => s2_txe,
      s_dbin   => int_adataout,

      d_clk    => mclk60,
      d_rd     => s2_rd,
      d_rxf    => s2_rxf,
      d_dbout  => int_mdataout
   );

int_adataout <= int_adatain;
s2_wr <= s1_rd ;
s1_rd <= s1_rxf and s2_txe;

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

    int_datain  => int_mdataout,
    int_rxf     => s2_rxf,
    int_rd      => s2_rd,
    int_dataout => int_mdatain,
    int_txe     => s1_txe,
    int_wr      => s1_wr
 );




end rtl;

