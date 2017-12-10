-----------------------------------------------------------------------------
-- Title           : Sync flop
-- Project         : ft4232h
-----------------------------------------------------------------------------
-- File            : syncflop.vhd
-- Author          : A.J. Dougan
-- Company         : Future Technology Devices International
-- Date Created    : 29-10-2009
-----------------------------------------------------------------------------
-- Description     : This syncs signals from different clock domain
-----------------------------------------------------------------------------
-- Known issues and omissions: 
--                 
--                 None
-----------------------------------------------------------------------------
-- Copyright 2007 FTDI Ltd. All rights reserved
-----------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

entity syncflop is
   generic ( G_NFLOPS : integer := 2 ) ;
   port (
      clk            :  in     std_logic;
      reset_n        :  in     std_logic;
      din            :  in     std_logic;
      dout           :  out    std_logic
   );
   end syncflop;

architecture rtl of syncflop is

signal metastable  : std_logic_vector(G_NFLOPS-1 downto 0);

begin

drpp : process(reset_n,clk)
begin
GEN_REGS:
if (reset_n='0') then
  metastable <= (others=>'0');
elsif rising_edge(clk) then
  metastable(G_NFLOPS-1 downto 0) <= metastable(G_NFLOPS-2 downto 0) & din;
end if;
end process drpp;

dout <= metastable(G_NFLOPS-1);

end rtl;
