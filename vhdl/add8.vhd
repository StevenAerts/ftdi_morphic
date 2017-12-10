-----------------------------------------------------------------------------
-- Title           : 8 bit add 1 full adder with limits
-- Project         : ft232h
-----------------------------------------------------------------------------
-- File            : add8.vhd
-- Author          : A.J. Dougan
-- Company         : Future Technology Devices International
-- Date Created    : 14-11-2007
-----------------------------------------------------------------------------
-- Description     : This is an 8 bit adder .
-----------------------------------------------------------------------------
-- Known issues and omissions: 
--                 
--                 None
-----------------------------------------------------------------------------
-- Copyright 2007 FTDI Ltd. All rights reserved
-----------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

entity add8 is
   port(numin : in std_logic_vector(7 downto 0);
        result : out std_logic_vector(7 downto 0)
        );
end add8;

architecture rtl of add8 is

component fadd1 
        port(
        a : in std_logic;
        b : in std_logic;
        cin : in std_logic;
        d : out std_logic;
        cout : out std_logic
        );
end component;

signal co : std_logic_vector(7 downto 0);
signal b : std_logic_vector(7 downto 0);
signal carryin0 : std_logic;
signal res : std_logic_vector(7 downto 0);


begin


carryin0 <= '1';

b <= (others=>'0');

--=======================================================
--==  create the arithmetic unit                  
--=======================================================


-- create the adder
i1_fadd1 : fadd1 port map(
        a => numin(0),
        b => b(0),
        cin => carryin0,
        d => res(0),
        cout => co(0)
        );

i2_fadd1 : fadd1 port map(
        a => numin(1),
        b => b(1),
        cin => co(0),
        d => res(1),
        cout => co(1)
        );

i3_fadd1 : fadd1 port map(
        a => numin(2),
        b => b(2),
        cin => co(1),
        d => res(2),
        cout => co(2)
        );

i4_fadd1 : fadd1 port map(
        a => numin(3),
        b => b(3),
        cin => co(2),
        d => res(3),
        cout => co(3)
        );

i5_fadd1 : fadd1 port map(
        a => numin(4),
        b => b(4),
        cin => co(3),
        d => res(4),
        cout => co(4)
        );

i6_fadd1 : fadd1 port map(
        a => numin(5),
        b => b(5),
        cin => co(4),
        d => res(5),
        cout => co(5)
        );

i7_fadd1 : fadd1 port map(
        a => numin(6),
        b => b(6),
        cin => co(5),
        d => res(6),
        cout => co(6)
        );

i8_fadd1 : fadd1 port map(
        a => numin(7),
        b => b(7),
        cin => co(6),
        d => res(7),
        cout => co(7)
        );



-- assign outputs

result <= res;


end rtl;

