--===========================================================================
--== Code translated from original by TransVHDL Version 1.32
--===========================================================================
-----------------------------------------------------------------------------
-- Title           : generic n bit up counter with synchronous reset
-- Project         : ft4232h
-----------------------------------------------------------------------------
-- File            : upcntg.vhd
-- Author          : A.J. Dougan
-- Company         : Future Technology Devices International
-- Date Created    : 14-11-2007
-----------------------------------------------------------------------------
-- Description     : This is a generic n bit up counter with async and sync resets.
-----------------------------------------------------------------------------
-- Known issues and omissions: 
--                 
--                 None
-----------------------------------------------------------------------------
-- Copyright 2007 FTDI Ltd. All rights reserved
-----------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

entity upcntg is
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
end upcntg;


architecture rtl of upcntg is

signal int_count : std_logic_vector(WIDTH-1 downto 0);
signal count_inp : std_logic_vector(WIDTH-1 downto 0);
signal count_ands : std_logic_vector(WIDTH-2 downto 0);
       
begin

GEN_ANDS:
for I in 0 to WIDTH-2 generate

   FIRST_BIT_AND: if I=0 generate
      count_ands(I) <= int_count(I) and enable;
   end generate FIRST_BIT_AND;

   UPPER_BIT_AND: if I>0 generate
     count_ands(I) <= int_count(I) and count_ands(I-1);
   end generate UPPER_BIT_AND;
 
end generate GEN_ANDS;

     
-- create input multiplexer		  

-- first bit
count_inp(0) <= not int_count(0) when enable='1' else int_count(0);

-- upper bits
GEN_MUX:
for J in 1 to WIDTH-1 generate
   count_inp(J) <= not int_count(J) when count_ands(J-1)='1' else int_count(J);
end generate GEN_MUX;

-- create registered counter

cntgp : process (async_reset_n,clk)
begin
if (async_reset_n='0') then
   int_count <= (others=>'0');
elsif rising_edge(clk) then
   if (sync_reset='1') then
      int_count <= (others=>'0');
   else
      int_count <= count_inp;
   end if; 
end if; 
end process cntgp;

-- assign outputs

q <= int_count;

end rtl;
