-------------------------------------------------------------------------------
-- Title      : fifo block to buffer data between two clock domains
-------------------------------------------------------------------------------
-- File       : sync_fifo.vhd
-- Author     : AJ DOUGAN 
-- Company    : Future Technology Devices International
-- Created    : 2009-10-29
-- Last update: 2009-10-29
-- Platform   : 
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
--
-- Description: Fifo to buffer between 2 different clock domains. This uses
--              a single buffer to perform the transfer. To get higher performance
--              double buffers should be used.
--
-------------------------------------------------------------------------------
-- Copyright (c) 2009 
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author  Description
-- 2009-10-29  1.0      AJD     Created
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

entity sync_fifo is
   generic ( use_register_mem : integer := 1);
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
   end sync_fifo;

architecture rtl of sync_fifo is


component dpram_512
	PORT
	(
		data		: IN STD_LOGIC_VECTOR (7 DOWNTO 0);
		rdaddress		: IN STD_LOGIC_VECTOR (8 DOWNTO 0);
		rdclock		: IN STD_LOGIC ;
		wraddress		: IN STD_LOGIC_VECTOR (8 DOWNTO 0);
		wrclock		: IN STD_LOGIC ;
		wren		: IN STD_LOGIC  := '1';
		q		: OUT STD_LOGIC_VECTOR (7 DOWNTO 0)
	);
end component;

component dpram4 
        port(
        clk_wr  : in std_logic;
        reset_n : in std_logic;
        data_wr : in std_logic_vector(7 downto 0);
        add_wr  : in std_logic_vector(1 downto 0);
        wr      : in std_logic;
        clk_rd  : in std_logic;
        add_rd  : in std_logic_vector(1 downto 0);
        data_rd : out std_logic_vector(7 downto 0)
        );
end component;

component dncntlg 
   generic(
      WIDTH	: integer
   );
   port(
      clk           : in  std_logic;
      enable        : in  std_logic;
      d             : in  std_logic_vector(WIDTH-1 downto 0);
      q             : out std_logic_vector(WIDTH-1 downto 0);
      load          : in  std_logic;
      async_reset_n : in  std_logic;
      sync_reset    : in  std_logic
   );
end component;

component upcntg
   generic(
      WIDTH	: integer
   );
   port(
      clk           : in  std_logic;
      enable        : in  std_logic;
      q             : out std_logic_vector(WIDTH-1 downto 0);
      async_reset_n : in  std_logic;
      sync_reset    : in  std_logic
   );
end component;

component syncflop 
   generic ( G_NFLOPS : integer := 2 ) ;
   port (
      clk            :  in     std_logic;
      reset_n        :  in     std_logic;
      din            :  in     std_logic;
      dout           :  out    std_logic
   );
end component;

  -- State machine version
  

  type fstat is (
    FSIDLE,FSFULLA1,FSFULLA2);

  signal fstate : fstat;

  signal s_reset_n : std_logic;
  signal s_reset_n_ip : std_logic;

  signal d_reset_n : std_logic;
  signal d_reset_n_ip : std_logic;


  signal d_address   : std_logic_vector(8 downto 0);
  signal s_address   : std_logic_vector(8 downto 0);

  signal ram_a_wren      : std_logic;
  signal ram_a_q         : std_logic_vector(7 downto 0);

  signal s_incadd    : std_logic;
  signal d_incadd    : std_logic;
  signal s_bytecnt   : std_logic_vector(9 downto 0);
  signal d_bytecnt   : std_logic_vector(9 downto 0);
  signal ram_a_writemode : std_logic;

  signal s_full      : std_logic;
  signal s_notempty  : std_logic;

  signal d_notempty  : std_logic;
					-- signal d_empty     : std_logic;

  signal ram_s_fullup    : std_logic;
  signal ram_s_fullup_d    : std_logic;
  signal ram_d_fullup    : std_logic;

  signal s_ram_reset : std_logic;
  signal s_rstcnt : std_logic;

  signal d_ram_reset : std_logic;

  signal tie_low         : std_logic;

  signal int_txe_ram_a : std_logic;
  signal int_rxf_ram_a : std_logic;

  signal fill_pipe : std_logic;
  signal pipe_full : std_logic;
  signal int_d_dbout,data_hold_reg : std_logic_vector(7 downto 0);
  signal d_rd_del : std_logic;
  signal int_d_rxf : std_logic;

  signal ram_d_fullup_s : std_logic;

  signal pipe_has_data : std_logic;
  signal pipe_has_data_s : std_logic;
  
begin

tie_low <= '0';

--===============================================
--== Create reset for source domain
--===============================================

srpp : process(s_clk,reset_n)
begin
if (reset_n = '0') then
   s_reset_n_ip <= '0';
   s_reset_n    <= '0';
elsif rising_edge(s_clk) then
   s_reset_n_ip <= '1';
   s_reset_n    <= s_reset_n_ip;
end if;
end process srpp;

--===============================================
--== Create reset for destination domain
--===============================================

srdpp : process(d_clk,reset_n)
begin
if (reset_n = '0') then
   d_reset_n_ip <= '0';
   d_reset_n    <= '0';
elsif rising_edge(d_clk) then
   d_reset_n_ip <= '1';
   d_reset_n    <= d_reset_n_ip;
end if;
end process srdpp;

--===============================================
--== Bring in RAMS
--===============================================
SRAMEN : if use_register_mem = 0 generate

ram_a_wren <= s_wr and ram_a_writemode and not ram_s_fullup;

ram_a : dpram_512
	port map
	(
		data		=> s_dbin,
		rdaddress	=> d_address,
		rdclock	=> d_clk,
		wraddress	=> s_address,
		wrclock	=> s_clk,
		wren		=> ram_a_wren,
		q		=> ram_a_q
	);


s_full <= s_bytecnt(9) or
             (s_bytecnt(8) and 
              s_bytecnt(7) and 
              s_bytecnt(6) and 
              s_bytecnt(5) and 
              s_bytecnt(4) and 
              s_bytecnt(3) and 
              s_bytecnt(2) and 
              s_bytecnt(1) and 
              s_bytecnt(0) and 
              s_incadd); 

s_notempty <= s_bytecnt(9) or
              s_bytecnt(8) or 
              s_bytecnt(7) or 
              s_bytecnt(6) or 
              s_bytecnt(5) or 
              s_bytecnt(4) or 
              s_bytecnt(3) or 
              s_bytecnt(2) or 
              s_bytecnt(1) or 
              s_bytecnt(0); 

d_notempty <= d_bytecnt(9) or
              d_bytecnt(8) or 
              d_bytecnt(7) or 
              d_bytecnt(6) or 
              d_bytecnt(5) or 
              d_bytecnt(4) or 
              d_bytecnt(3) or 
              d_bytecnt(2) or 
              d_bytecnt(1) or 
              d_bytecnt(0); 


end generate SRAMEN;

RRAMEN : if use_register_mem = 1 generate

ram_a_wren <= s_wr and ram_a_writemode and not ram_s_fullup;


ram_a : dpram4 
        port map(
        clk_wr  => s_clk,
        reset_n => s_reset_n,
        data_wr => s_dbin,
        add_wr  => s_address(1 downto 0),
        wr      => ram_a_wren,
        clk_rd  => d_clk,
        add_rd  => d_address(1 downto 0),
        data_rd => ram_a_q
        );

s_full <= s_bytecnt(2) or
             (s_bytecnt(1) and 
              s_bytecnt(0) and 
              s_incadd); 

s_notempty <= s_bytecnt(2) or 
              s_bytecnt(1) or 
              s_bytecnt(0); 

d_notempty <= d_bytecnt(2) or 
              d_bytecnt(1) or 
              d_bytecnt(0); 

end generate RRAMEN;


				-- d_empty <= not d_notempty;


--====================================
--== Bring in input address counters
--====================================

-- address counter

s_incadd <= ram_a_wren;

ram_a_s_addcnt : upcntg
   generic map(
      WIDTH	=> 9
   )
   port map(
      clk           => s_clk,
      enable        => s_incadd,
      q             => s_address,
      async_reset_n => s_reset_n,
      sync_reset    => s_ram_reset
   );

ram_a_s_fullcnt : upcntg
   generic map(
      WIDTH	=> 10
   )
   port map(
      clk           => s_clk,
      enable        => s_incadd,
      q             => s_bytecnt,
      async_reset_n => s_reset_n,
      sync_reset    => s_rstcnt
   );


ram_a_txpp : process(s_reset_n,s_clk)
begin
if (s_reset_n='0') then
   ram_s_fullup <= '0';
elsif rising_edge(s_clk) then
   ram_s_fullup <= s_full or (s_notempty and not ram_a_wren) or
                   (ram_s_fullup and ram_a_writemode);
end if;
end process ram_a_txpp;



--====================================
--== Bring in output address counters
--====================================

d_incadd <= (d_rd or fill_pipe) and int_rxf_ram_a;

ram_a_d_addcnt : upcntg
   generic map(
      WIDTH	=> 9
   )
   port map(
      clk           => d_clk,
      enable        => d_incadd,
      q             => d_address,
      async_reset_n => d_reset_n,
      sync_reset    => d_ram_reset
   );

ram_a_d_fullcnt : dncntlg 
   generic map(
      WIDTH	=> 10
   )
   port map(
      clk           => d_clk,
      enable        => d_incadd,
      d             => s_bytecnt,
      q             => d_bytecnt,
      load          => d_ram_reset,
      async_reset_n => d_reset_n,
      sync_reset    => tie_low
   );



ram_d_txpp : process(d_reset_n,d_clk)
begin
if (d_reset_n='0') then
   ram_d_fullup <= '0';
elsif rising_edge(d_clk) then
--   ram_d_fullup <= ram_s_fullup_d or 
--                   (ram_d_fullup and not (not d_notempty and d_rd));
--   ram_d_fullup <= (ram_s_fullup_d and d_ram_reset) or (ram_d_fullup and not (d_empty and d_rd));
   ram_d_fullup <= (ram_s_fullup_d) or (ram_d_fullup and d_notempty);
end if;
end process ram_d_txpp;

int_rxf_ram_a <= ram_d_fullup and not ram_a_writemode and d_notempty;


--===============================================
--== sync back to source clock domain
--===============================================

sync_ram_a_1 : syncflop 
   port map(
      clk       => d_clk,
      reset_n   => d_reset_n,
      din       => ram_a_writemode,
      dout      => d_ram_reset
   );

sync_ram_a_2 : syncflop 
   port map(
      clk       => d_clk,
      reset_n   => d_reset_n,
      din       => ram_s_fullup,
      dout      => ram_s_fullup_d
   );


sync_ram_a_3 : syncflop 
   port map(
      clk       => s_clk,
      reset_n   => s_reset_n,
      din       => ram_d_fullup,
      dout      => ram_d_fullup_s
   );


sync_ram_a_4 : syncflop 
   port map(
      clk       => s_clk,
      reset_n   => s_reset_n,
      din       => pipe_has_data,
      dout      => pipe_has_data_s
   );

--===============================================
--== State machine
--===============================================

stmpp : process(s_clk, s_reset_n)
begin
if (s_reset_n='0') then
   fstate   <= FSIDLE;
   ram_a_writemode <= '0';
   int_txe_ram_a <= '0';
   s_ram_reset <= '0';
   s_rstcnt <= '0';

elsif ((s_clk'event) AND (s_clk = '1')) then
   case fstate is
--
   when FSIDLE =>
      if (ram_s_fullup='1') then
         fstate <= FSFULLA1;
         ram_a_writemode <= '0';
      else
         fstate <= FSIDLE;
         ram_a_writemode <= '1';
      end if;   
      int_txe_ram_a <= not ram_s_fullup;
      s_ram_reset <= '0';
      s_rstcnt <= '0';
--
   when FSFULLA1 =>
      if (ram_d_fullup_s='0') then
         fstate <= FSFULLA1;
      else
         fstate <= FSFULLA2;
      end if;   
      ram_a_writemode <= '0';
      s_ram_reset <= '0';
      s_rstcnt <= '0';
      int_txe_ram_a <= '0';
--
   when FSFULLA2 =>
      if (ram_d_fullup_s='0') and (pipe_has_data_s='0') then
         fstate <= FSIDLE;
         ram_a_writemode <= '1';
         s_ram_reset <= '1';
         s_rstcnt <= '1';
      else
         fstate <= FSFULLA2;
         ram_a_writemode <= '0';
         s_ram_reset <= '0';
         s_rstcnt <= '1';
      end if;   
      int_txe_ram_a <= '0';
--
   when others =>
      fstate <= FSIDLE;
      ram_a_writemode <= '0';
      int_txe_ram_a <= '0';
      s_ram_reset <= '0';
      s_rstcnt <= '0';

   end case;
end if;
end process stmpp;


  -----------------------------------------------------------------------------
  -- pipline data out
  -----------------------------------------------------------------------------
int_d_dbout <= ram_a_q;

plcpp : process(d_reset_n,d_clk)
begin
if (d_reset_n='0') then
   fill_pipe <= '0';
   pipe_full <= '0';
   data_hold_reg <= (others => '0');
   d_rd_del <= '0';
   pipe_has_data <= '0';
elsif rising_edge(d_clk) then
   fill_pipe <= int_d_rxf and not pipe_full;
   pipe_full <= int_d_rxf;
   if (fill_pipe='1') or (d_rd = '1') or (d_rd_del='1') then
      data_hold_reg <= int_d_dbout;
   end if;
   d_rd_del <= d_rd;
   pipe_has_data <= (d_incadd) or (pipe_has_data and not d_rd); 
end if;
end process plcpp;

int_d_rxf <= int_rxf_ram_a;

mxdop : process(int_d_dbout,d_rd_del,data_hold_reg)
begin
if (d_rd_del='1') then
   d_dbout <= int_d_dbout;
else
   d_dbout <= data_hold_reg;
end if;
end process mxdop;

  -----------------------------------------------------------------------------
  -- Assign outputs
  -----------------------------------------------------------------------------

s_txe <= int_txe_ram_a and not ram_s_fullup;

d_rxf <= (pipe_full and int_d_rxf and not fill_pipe) or pipe_has_data;

end rtl;










