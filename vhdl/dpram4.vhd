-----------------------------------------------------------------------------
-- Title           : dual port ram from D types
-- Project         : ft232h
-----------------------------------------------------------------------------
-- File            : dpram4.vhd
-- Author          : A.J. Dougan
-- Company         : Future Technology Devices International
-- Date Created    : 14-11-2007
-----------------------------------------------------------------------------
-- Description     : This is a 4 byte dual port RAM
-----------------------------------------------------------------------------
-- Known issues and omissions: 
--                 
--                 None
-----------------------------------------------------------------------------
-- Copyright 2007 FTDI Ltd. All rights reserved
-----------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

entity dpram4 is
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
end dpram4;

architecture rtl of dpram4 is

signal reg1 : std_logic_vector(7 downto 0);
signal reg2 : std_logic_vector(7 downto 0);
signal reg3 : std_logic_vector(7 downto 0);
signal reg4 : std_logic_vector(7 downto 0);
signal read_add : std_logic_vector(1 downto 0);

begin

--=================================================
--== create memory registers
--=================================================

ckrp : process(reset_n,clk_wr)
begin
if (reset_n ='0') then
   reg1 <= (others=> '0');
   reg2 <= (others=> '0');
   reg3 <= (others=> '0');
   reg4 <= (others=> '0');
elsif rising_edge(clk_wr) then
   if ((add_wr="00") and (wr='1')) then reg1 <= data_wr; end if;
   if ((add_wr="01") and (wr='1')) then reg2 <= data_wr; end if;
   if ((add_wr="10") and (wr='1')) then reg3 <= data_wr; end if;
   if ((add_wr="11") and (wr='1')) then reg4 <= data_wr; end if;
end if;
end process ckrp;

--=================================================
--== mux output data
--=================================================

ckrrp : process(reset_n,clk_rd)
begin
if (reset_n ='0') then
   read_add <= (others=> '0');
elsif rising_edge(clk_rd) then
   read_add <= add_rd;
end if;
end process ckrrp;

mxop : process(read_add,reg1,reg2,reg3,reg4)
begin
case read_add is
   when "00" => data_rd <= reg1;
   when "01" => data_rd <= reg2;
   when "10" => data_rd <= reg3;
   when "11" => data_rd <= reg4;
   when others => data_rd <= reg1;
end case;
end process mxop;


end rtl;
