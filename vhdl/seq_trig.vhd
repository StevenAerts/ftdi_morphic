-----------------------------------------------------------------------------
-- Title           : Sequential trigger block for debug
-- Project         : ft232h
-----------------------------------------------------------------------------
-- File            : seq_trig.vhd
-- Author          : A.J. Dougan
-- Company         : Future Technology Devices International
-- Date Created    : 14-11-2007
-----------------------------------------------------------------------------
-- Description     : This monitors a bus and triggers if the present value does
--                   not match the previous value + 1
-----------------------------------------------------------------------------
-- Known issues and omissions: 
--                 
--                 None
-----------------------------------------------------------------------------
-- Copyright 2007 FTDI Ltd. All rights reserved
-----------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

entity seq_trig is
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
end seq_trig;




architecture rtl of seq_trig is

component upcntg 
   generic(
   width:    integer
);
   port(
      clk           : in  std_logic;
      enable        : in  std_logic;
      q             : out std_logic_vector(width-1 downto 0);
      async_reset_n : in  std_logic;
      sync_reset    : in  std_logic
);
   end component;


component add8 
   port(numin : in std_logic_vector(7 downto 0);
        result : out std_logic_vector(7 downto 0)
        );
   end component;


signal old_data : std_logic_vector(7 downto 0);
signal current_data : std_logic_vector(7 downto 0);
signal enable       : std_logic;
signal int_trigger  : std_logic;
signal chk_data : std_logic_vector(7 downto 0);

begin

--===========================================================
--== starting signal
--===========================================================

regpp : process(clk,reset_n)
begin
if (reset_n='0') then
   enable       <= '0';
   current_data <= (others=>'0');
   old_data     <= (others=>'0');
   trigger      <= '0';
elsif rising_edge(clk) then
   if (strobe='1') then
      enable       <= '1';
      current_data <= mon_bus;
      old_data <= current_data;
   end if;
   trigger <= int_trigger;
end if;
end process regpp;

adder1 : add8 
   port map(
        numin  => old_data,
        result => chk_data
        );

int_trigger <= '1' when (enable='1') and (chk_data /= current_data) else '0';
        
end rtl;
