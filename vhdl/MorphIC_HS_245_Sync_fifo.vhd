-------------------------------------------------------------------------------
-- Title      : MorphIC 2 v1.2 wrapper using synchronous 245 Interface for FT2232H
-------------------------------------------------------------------------------
-- File       : MorphIC_HS_245_Sync_Fifo.vhd
-- Author     : AJ DOUGAN 
-- Company    : Future Technology Devices International
-- Created    : 2009-10-29
-- Last update: 2009-10-29
-- Platform   : 
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: synchronous 245 interface between FT2232H and FT2232H device.
--              Data will be transferred in one direction at a time between the
--              two devices. If full duplex operation is required then arbitration
--              logic should be added at this level.
-------------------------------------------------------------------------------
-- Copyright (c) 2009 
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author  Description
-- 2007-10-28  1.0      AJD     Created
-------------------------------------------------------------------------------


library ieee;
use ieee.std_logic_1164.all;

entity morphic_hs_245_sync_fifo is
   generic ( loopback_to_hsext : integer := 1 );
   port (-- Inputs
					-- clk50     : in  std_logic;       -- 50MHz clock input unused
      rst       : in  std_logic;       -- Active high reset via BD7

-- Morphic on board FT2232H signals
      mdata     : inout std_logic_vector(7 downto 0);   -- Port A Data Bus
      mclk60    : in std_logic;
      mrxfn     : in std_logic;
      mtxen     : in std_logic;
      mrdn      : out std_logic;
      mwrn      : out std_logic;
      moen      : out std_logic;
		msndimm   : out std_logic;           -- unused
      
-- High speed Synchronous 245 signals
		hsndimm   : out std_logic;           -- unused
      hclk60    : in  std_logic;           -- 60MHz clock input
      hdata     : inout std_logic_vector(7 downto 0);
      hrxfn     : in std_logic;            -- RX Full #
      htxen     : in std_logic;            -- TX Full #
      hoen      : out std_logic;           -- OE# HBDBUS6
      hrdn      : out std_logic;           -- RD#
      hwrn      : out std_logic            -- WR#
);
   end morphic_hs_245_sync_fifo;

architecture rtl of morphic_hs_245_sync_fifo is

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

component sync_pll
	PORT
	(
		inclk0		: IN STD_LOGIC  := '0';
		c0		: OUT STD_LOGIC ;
		locked		: OUT STD_LOGIC 
	);
end component;


component seq_trig 
  port (
--------------------
-- Inputs
--------------------
    clk                 : in std_logic; 
    reset_n             : in std_logic;
    sync_reset          : in std_logic;
    strobe              : in std_logic;
    mon_bus             : in std_logic_vector(7 downto 0);

--------------------
-- Outputs
--------------------
    trigger             : out std_logic
  );
  
  end component;

  --
  --  

  signal mrxf,mtxe,moe,mrd,mwr : std_logic;
  signal mdatain  : std_logic_vector(7 downto 0);
  signal mdataout : std_logic_vector(7 downto 0);
  signal mdataen  : std_logic;
  
  signal s1_wr,s1_txe,s1_rd,s1_rxf,s1_mrd : std_logic;

  signal reset_n : std_logic;

  signal hrxf,htxe,hoe,hrd,hwr : std_logic;
  signal hdatain  : std_logic_vector(7 downto 0);
  signal hdataout : std_logic_vector(7 downto 0);
  signal hdataen  : std_logic;
  
  signal s2_wr,s2_txe,s2_rd,s2_rxf,s2_hrd : std_logic;

  signal hclk60_pll,pll2_lock : std_logic;

  signal int_hdatain  : std_logic_vector(7 downto 0);
  signal int_hdataout : std_logic_vector(7 downto 0);
  signal int_mdatain  : std_logic_vector(7 downto 0);
  signal int_mdataout : std_logic_vector(7 downto 0);


  signal tie_low: std_logic;
  signal strobe_mor,strobe_hs : std_logic;
  

begin
  
tie_low <= '0';

--pll1_mor : sync_pll
--	port map
--	(
--		inclk0	=> mclk60,
--		c0		=> mclk60_pll,
--		locked	=> pll1_lock
--	);

--========================================================
--== Create buffers for all Bidi I/Os for Morphic FT2232H side
--========================================================

mdataen <= not moe;

GEN_MORPHIC_DATABUS:

for I in 0 to 7 generate

mdbus_I : process(mdataout,mdataen)
begin
if (mdataen='1') then
   mdata(I) <= mdataout(I);
else
   mdata(I) <= 'Z';
end if;
end process mdbus_I;

mdatain(i) <= mdata(i);

end generate GEN_MORPHIC_DATABUS;

--reset_n <= rst; -- polarity to programme over JTAG
reset_n <= not rst; -- polarity to programme over USB

GENPASS : if loopback_to_hsext = 0 generate

strobe_hs <= hwr and htxe;

trig1 : seq_trig 
  port map(
    clk          => hclk60,
    reset_n      => reset_n,
    sync_reset   => tie_low,
    strobe       => strobe_hs,
    mon_bus      => hdataout

  );


strobe_mor <= mwr and mtxe;

trig2 : seq_trig 
  port map(
    clk          => mclk60,
    reset_n      => reset_n,
    sync_reset   => tie_low,
    strobe       => strobe_mor,
    mon_bus      => mdataout

  );

--------------------------------------------------------------------------------
-- Instantiate synchronous fifo from morphic2 to external HS chip
--------------------------------------------------------------------------------

sync1 : sync_fifo 
   port map(                                 
      reset_n  => reset_n,
--
      s_clk    => mclk60,
      s_wr     => s1_wr,
      s_txe    => s1_txe,
      s_dbin   => int_mdatain,
--
--      d_clk    => hclk60_pll,
      d_clk    => hclk60,
      d_rd     => s1_rd,
      d_rxf    => s1_rxf,
      d_dbout  => int_hdataout
   );


mrxf <= not mrxfn;
mtxe <= not mtxen;


xfer1 : hs245_sif 
  port map(                                 
    clk        => mclk60,
    reset_n    => reset_n,
--
    ext_txe    => mtxe,
    ext_rxf    => mrxf,
    ext_wr     => mwr,
    ext_rd     => mrd,
    ext_oe     => moe,
    ext_datain => mdatain,
    ext_dataout => mdataout,
--
    int_datain  => int_mdataout,
    int_rxf     => s2_rxf,
    int_rd      => s2_rd,
--
    int_dataout => int_mdatain,
    int_txe     => s1_txe,
    int_wr      => s1_wr

    );




 --------------------------------------------------------------------------------
-- Instantiate synchronous fifo from external HS chip to morphic2
--------------------------------------------------------------------------------

sync2 : sync_fifo 
   port map(                                 
      reset_n  => reset_n,
--
--      s_clk    => hclk60_pll,
      s_clk    => hclk60,
      s_wr     => s2_wr,
      s_txe    => s2_txe,
      s_dbin   => int_hdatain,
--
      d_clk    => mclk60,
      d_rd     => s2_rd,
      d_rxf    => s2_rxf,
      d_dbout  => int_mdataout
   );


hrxf <= not hrxfn;
htxe <= not htxen;

xfer2 : hs245_sif 
  port map(                                 
--    clk        => hclk60_pll,
    clk        => hclk60,
    reset_n    => reset_n,
--
    ext_txe    => htxe,
    ext_rxf    => hrxf,
    ext_wr     => hwr,
    ext_rd     => hrd,
    ext_oe     => hoe,
    ext_datain => hdatain,
    ext_dataout => hdataout,
--
    int_datain  => int_hdataout,
    int_rxf     => s1_rxf,
    int_rd      => s1_rd,
--
    int_dataout => int_hdatain,
    int_txe     => s2_txe,
    int_wr      => s2_wr

    );

end generate GENPASS;

GENLOOP : if loopback_to_hsext = 1 generate




sync1 : sync_fifo 
   port map(                                 
      reset_n  => reset_n,
--
      s_clk    => mclk60,
      s_wr     => s1_wr,
      s_txe    => s1_txe,
      s_dbin   => int_mdatain,
--
--      d_clk    => hclk60_pll,
      d_clk    => mclk60,
      d_rd     => s1_rd,
      d_rxf    => s1_rxf,
      d_dbout  => int_mdataout
   );


mrxf <= not mrxfn;
mtxe <= not mtxen;


xfer1 : hs245_sif 
  port map(                                 
    clk        => mclk60,
    reset_n    => reset_n,
--
    ext_txe    => mtxe,
    ext_rxf    => mrxf,
    ext_wr     => mwr,
    ext_rd     => mrd,
    ext_oe     => moe,
    ext_datain => mdatain,
    ext_dataout => mdataout,
--
    int_datain  => int_mdataout,
    int_rxf     => s1_rxf,
    int_rd      => s1_rd,
--
    int_dataout => int_mdatain,
    int_txe     => s1_txe,
    int_wr      => s1_wr

    );


sync2 : sync_fifo 
   port map(                                 
      reset_n  => reset_n,
--
--      s_clk    => hclk60_pll,
      s_clk    => hclk60,
      s_wr     => s2_wr,
      s_txe    => s2_txe,
      s_dbin   => int_hdatain,
--
      d_clk    => hclk60,
      d_rd     => s2_rd,
      d_rxf    => s2_rxf,
      d_dbout  => int_hdataout
   );



hrxf <= not hrxfn;
htxe <= not htxen;

xfer2 : hs245_sif 
  port map(                                 
--    clk        => hclk60_pll,
    clk        => hclk60,
    reset_n    => reset_n,
--
    ext_txe    => htxe,
    ext_rxf    => hrxf,
    ext_wr     => hwr,
    ext_rd     => hrd,
    ext_oe     => hoe,
    ext_datain => hdatain,
    ext_dataout => hdataout,
--
    int_datain  => int_hdataout,
    int_rxf     => s2_rxf,
    int_rd      => s2_rd,
--
    int_dataout => int_hdatain,
    int_txe     => s2_txe,
    int_wr      => s2_wr

    );

end generate GENLOOP;



moen <= not moe;
mrdn <= not mrd;
mwrn <= not mwr;

msndimm <= '1';


hoen <= not hoe;
hrdn <= not hrd;
hwrn <= not hwr;

hsndimm <= '1';

--------------------------------------------------------------------------------
-- Create PLL
--------------------------------------------------------------------------------

--pll2_hs : sync_pll
--	port map
--	(
--		inclk0	=>	hclk60,
--		c0		=> hclk60_pll,
--		locked	=> pll2_lock
--	);


--========================================================
--== Create buffers for all Bidi I/Os for External data bus
--========================================================


hdataen <= not hoe;


GEN_EXT_DATABUS:

for I in 0 to 7 generate

hdbus_I : process(hdataout,hdataen)
begin
if (hdataen='1') then
   hdata(I) <= hdataout(I);
else
   hdata(I) <= 'Z';
end if;
end process hdbus_I;

hdatain(i) <= hdata(i);

end generate GEN_EXT_DATABUS;



 
end rtl;

