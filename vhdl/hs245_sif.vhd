-------------------------------------------------------------------------------
-- Title      : Fast Serial Interface for testing FT2232H
-------------------------------------------------------------------------------
-- File       : hs245_sif.vhd
-- Author     : AJ DOUGAN 
-- Company    : FTDI
-- Created    : 2010-June-15
-- Last update: 2010-June-15
-- Platform   : 
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: 245 sync Interface 
-------------------------------------------------------------------------------
-- Copyright (c) 2010 
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author  Description
-- 2010-June-15  1.0      AJD     Created
-------------------------------------------------------------------------------

LIBRARY IEEE;
USE IEEE.std_logic_1164.ALL;

entity hs245_sif is
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
end hs245_sif;

architecture rtl of hs245_sif is

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

  -- State machine version
  

type fsstat is (fsidle,fsread,fswrite);

signal fsstate : fsstat;

signal enbyte_cnt : std_logic;
signal rst_cnt    : std_logic;
signal byte_cnt   : std_logic_vector(1 downto 0);
signal cnt_limit  : std_logic;
signal en_ext_to_int : std_logic;
signal en_int_to_ext : std_logic;
signal can_read_ext  : std_logic;
signal can_write_ext : std_logic;
signal int_oe        : std_logic;
signal last_was_read : std_logic;

signal tx_hold_reg   : std_logic_vector(7 downto 0);

signal sel_hold,hold_wr : std_logic;

begin

--===============================================
--== connect data buffers
--===============================================


rcpp : process(reset_n,clk)
begin
if (reset_n='0') then
   tx_hold_reg <= (others=>'0');
elsif rising_edge(clk) then
   if (en_int_to_ext='1') then
      tx_hold_reg <= int_datain;
   end if;
end if;
end process rcpp;

mxext : process(int_datain,tx_hold_reg,sel_hold)
begin
if (sel_hold='1') then
   ext_dataout <= tx_hold_reg;
else
   ext_dataout <= int_datain;
end if;
end process mxext;

int_dataout <= ext_datain;


--===============================================
--== byte counter
--===============================================

enbyte_cnt <= ((en_ext_to_int and ext_rxf) or (en_int_to_ext and ext_txe)) 
              and not(byte_cnt(1) and byte_cnt(0)); 


bcnt1 : upcntg
   generic map(
      WIDTH	=> 2
   )
   port map(
      clk           => clk,
      enable        => enbyte_cnt,
      q             => byte_cnt,
      async_reset_n => reset_n,
      sync_reset    => rst_cnt
   );

cnt_limit <= (byte_cnt(1) and byte_cnt(0));-- or
             --(byte_cnt(1) and not byte_cnt(0) and enbyte_cnt);

--===============================================
--== State machine
--===============================================

can_read_ext  <= ext_rxf and int_txe;
can_write_ext <= ext_txe and int_rxf;


 stmpp : PROCESS(clk, reset_n)
 BEGIN
 IF (reset_n='0') then
    fsstate   <= fsidle;
    rst_cnt   <= '0';
    en_ext_to_int <= '0';
    en_int_to_ext <= '0';
    last_was_read <= '0';
    int_oe        <= '0';
    sel_hold      <= '0';
    hold_wr       <= '0';
 ELSIF ((clk'event) and (clk = '1')) THEN
    CASE fsstate IS
--
       WHEN fsidle =>
         if (rst_cnt='1') then
            if (sel_hold='1') then
               if (ext_txe='1') then
                  hold_wr       <= not hold_wr;
               else
                  hold_wr       <= '0';
               end if; 
               fsstate   <= fsidle;
               sel_hold      <= sel_hold and not hold_wr;
            else
               if (can_read_ext='1') and (can_write_ext='1') then
                  if (last_was_read='1') then
                     fsstate   <= fswrite;
                     int_oe        <= '0';
                  else
                     fsstate   <= fsread;
                     int_oe        <= '1';
                  end if;   
               elsif (can_read_ext='1') and (can_write_ext='0') then
                  fsstate   <= fsread;
                  int_oe        <= '1';
               elsif (can_read_ext='0') and (can_write_ext='1') then
                  fsstate   <= fswrite;
                  int_oe        <= '0';
               else
                  fsstate   <= fsidle;
                  int_oe        <= '0';
               end if;
               hold_wr       <= '0';
               sel_hold      <= sel_hold;
            end if;
         else   
            fsstate   <= fsidle;
            int_oe        <= '0';
            hold_wr       <= '0';
            sel_hold      <= sel_hold;
         end if;
         rst_cnt   <= '1';
         en_ext_to_int  <= '0';
         en_int_to_ext  <= '0';
         last_was_read <= last_was_read;
--         sel_hold      <= sel_hold;
--         int_oe        <= '0';
--         hold_wr       <= '0';
--
       WHEN fsread =>
         if (can_read_ext='1') and (cnt_limit='0') then
            fsstate   <= fsread;
            en_ext_to_int  <= '1';
            int_oe        <= '1';
         else
            fsstate   <= fsidle;
            en_ext_to_int  <= '0';
            int_oe        <= '0';
         end if;
         rst_cnt   <= '0';
--         en_ext_to_int  <= '0';
         en_int_to_ext  <= '0';
         last_was_read <= '1';
         sel_hold      <= '0';
--         int_oe        <= '0';
         hold_wr       <= '0';
--
       WHEN fswrite =>
         if (can_write_ext='1') and (cnt_limit='0') then
            fsstate   <= fswrite;
            en_int_to_ext  <= '1';
         else
            fsstate   <= fsidle;
            en_int_to_ext  <= '0';
         end if;
         rst_cnt   <= '0';
         en_ext_to_int  <= '0';
--         en_int_to_ext  <= '0';
         last_was_read <= '0';
         sel_hold      <= en_int_to_ext and int_rxf and not ext_txe;
         int_oe        <= '0';
         hold_wr       <= '0';
--
      WHEN others =>
         fsstate   <= fsidle;
         rst_cnt   <= '0';
         en_ext_to_int  <= '0';
         en_int_to_ext  <= '0';
         last_was_read <= '0';
         sel_hold      <= '0';
         int_oe        <= '0';
         hold_wr       <= '0';
   END CASE;
END IF;
END PROCESS stmpp;

  -----------------------------------------------------------------------------
  -- Assign outputs
  -----------------------------------------------------------------------------
ext_rd <= en_ext_to_int;
int_wr <= en_ext_to_int and ext_rxf;

ext_wr <= (en_int_to_ext and int_rxf) or hold_wr;
int_rd <= en_int_to_ext;

ext_oe <= int_oe;


end rtl;










