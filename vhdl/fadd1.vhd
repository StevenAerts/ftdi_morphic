--===========================================================================
--== Code translated from original by TransVHDL Version 1.31
--===========================================================================
-----------------------------------------------------------------------------
-- Title           : 1 bit Full adder
-- Project         : ft4232h
-----------------------------------------------------------------------------
-- File            : fadd1.vhd
-- Author          : A.J. Dougan
-- Company         : Future Technology Devices International
-- Date Created    : 14-11-2007
-----------------------------------------------------------------------------
-- Description     : This is a 1 bit full adder
-----------------------------------------------------------------------------
-- Known issues and omissions: 
--                 
--                 None
-----------------------------------------------------------------------------
-- Copyright 2007 FTDI Ltd. All rights reserved
-----------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

entity fadd1 is
        port(
        a : in std_logic;
        b : in std_logic;
        cin : in std_logic;
        d : out std_logic;
        cout : out std_logic
        );
end fadd1;

architecture rtl of fadd1 is


begin

cout <= (a and b) or (a and cin) or (b and cin);

d <= (a and b and cin) or 
     (not a and not b and cin) or 
     (a and not b and not cin) or 
     (not a and b and not cin);


end rtl;
