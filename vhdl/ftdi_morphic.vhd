library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

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

  signal bytes_left, bytes_left_next: integer range 0 to 65535;
  signal cmd_saved, cmd_saved_next: std_logic_vector(4 downto 0);
  signal need_bytes_msb, need_bytes_msb_next : std_logic;
  signal need_bytes_lsb, need_bytes_lsb_next : std_logic;
  
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

sync1 : sync_fifo 
   port map(                                 
      reset_n  => reset_n,

      s_clk    => mclk60,
      s_wr     => s1_wr,
      s_txe    => s1_txe,
      s_dbin   => int_mdatain,

      d_clk    => clk50,
      d_rd     => s1_rd,
      d_rxf    => s1_rxf,
      d_dbout  => int_adatain
   );

sync2 : sync_fifo 
   port map(                                 
      reset_n  => reset_n,

      s_clk    => clk50,
      s_wr     => s2_wr,
      s_txe    => s2_txe,
      s_dbin   => int_adataout,

      d_clk    => mclk60,
      d_rd     => s2_rd,
      d_rxf    => s2_rxf,
      d_dbout  => int_mdataout
   );
   
dummy_proc: process(clk50)
begin
  if rising_edge(clk50) then
	if rst = '1' then
	  bytes_left <= 0;
	  cmd_saved <= (others => '0');
	  need_bytes_msb <= '0';
	  need_bytes_lsb <= '0';
	else
	  bytes_left <= bytes_left_next;
	  cmd_saved  <= cmd_saved_next;
	  need_bytes_msb <= need_bytes_msb_next;
	  need_bytes_lsb <= need_bytes_lsb_next;
	end if;
	--io_out(0) <= io_in(1);
  end if;
end process dummy_proc;

decoder:process(int_adatain, s1_rxf, s2_txe, bytes_left, cmd_saved, 
	need_bytes_msb, need_bytes_lsb)
begin
  bytes_left_next <= bytes_left;
  cmd_saved_next <= cmd_saved;
  need_bytes_msb_next <= need_bytes_msb;
  need_bytes_lsb_next <= need_bytes_lsb;
  s1_rd <= '0'; s2_wr <= '0';
  int_adataout <= (others => '0');
  if s1_rxf = '1' and s2_txe = '1' then
	s1_rd <= '1';
    if need_bytes_msb = '1' then
	  need_bytes_msb_next <= '0';
	  bytes_left_next <= to_integer(unsigned(int_adatain&"00000000"));
	  case cmd_saved is
	    when "10101" | "10111" => 
			s2_wr <= '1'; 
			int_adataout <= int_adatain;
		when others => NULL;
	  end case;
	elsif need_bytes_lsb = '1' then
	  case cmd_saved is
	    when "10101" | "10111" => 
			s2_wr <= '1'; 
			int_adataout <= int_adatain;
		when others => NULL;
	  end case;
	  need_bytes_lsb_next <= '0';
	  bytes_left_next <= to_integer(to_unsigned(bytes_left,16)(15 downto 7)&unsigned(int_adatain));
	elsif bytes_left > 0 then
	  bytes_left_next <= bytes_left - 1;
	  case cmd_saved is
	    when "10101" => 
			s2_wr <= '1'; 
			int_adataout <= int_adatain;
	    when "10111" => 
			s2_wr <= '1'; 
			int_adataout <= not int_adatain;
		when others => NULL;
	  end case;
    else
	  cmd_saved_next <= int_adatain(7 downto 3);
	  case int_adatain(7 downto 3) is
	    when "10101" | "10111" => 
			s2_wr <= '1'; 
			int_adataout <= int_adatain;
		when others => NULL;
	  end case;
	  if int_adatain(2 downto 0) = "111" then
		need_bytes_msb_next <= '1';
		need_bytes_lsb_next <= '1';
	  elsif int_adatain(2 downto 0) = "110" then
		need_bytes_lsb_next <= '1';
	  else
		bytes_left_next <= to_integer(unsigned(int_adatain(2 downto 0)));
	  end if;
    end if;
  end if;
end process;

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

