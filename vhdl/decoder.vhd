library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity decoder is
   port (-- Inputs
	  clk50     : in  std_logic;       -- 50MHz clock input
      rst       : in  std_logic;       -- Active high reset

      decin     : in 	std_logic_vector(7 downto 0);
      dec_next	: in	std_logic;
      
      dec_islen : out	std_logic;
      dec_isdata: out	std_logic;

	  cmd		: out	std_logic_vector(4 downto 0);
	  cmd_new	: out	std_logic
);
end decoder;

architecture rtl of decoder is

  signal bytes_left, bytes_left_next: integer range 0 to 65535;
  signal cmd_saved, cmd_saved_next: std_logic_vector(4 downto 0);
  signal need_bytes_msb, need_bytes_msb_next : std_logic;
  signal need_bytes_lsb, need_bytes_lsb_next : std_logic;
  signal cmd_new_int: std_logic;
  
begin

out_asn: block
begin
	cmd <= cmd_saved_next;
	cmd_new <= cmd_new_int;
	dec_islen <= need_bytes_msb or need_bytes_lsb;
	dec_isdata <= not (cmd_new_int or need_bytes_msb or need_bytes_lsb);
end block out_asn;

reg_proc: process(clk50)
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
  end if;
end process reg_proc;

next_proc: process(decin, dec_next, bytes_left, cmd_saved, 
	need_bytes_msb, need_bytes_lsb)
begin
  bytes_left_next <= bytes_left;
  cmd_saved_next <= cmd_saved;
  need_bytes_msb_next <= need_bytes_msb;
  need_bytes_lsb_next <= need_bytes_lsb;
  cmd_new_int <= '0';
  if dec_next = '1' then
    if need_bytes_msb = '1' then
	  need_bytes_msb_next <= '0';
	  bytes_left_next <= to_integer(unsigned(decin&"00000000"));
	elsif need_bytes_lsb = '1' then
	  need_bytes_lsb_next <= '0';
	  bytes_left_next <= to_integer(to_unsigned(bytes_left,16)(15 downto 7)&unsigned(decin));
	elsif bytes_left > 0 then
	  bytes_left_next <= bytes_left - 1;
    else
	  cmd_saved_next <= decin(7 downto 3);
	  cmd_new_int <= '1';
	  if decin(2 downto 0) = "111" then
		need_bytes_msb_next <= '1';
		need_bytes_lsb_next <= '1';
	  elsif decin(2 downto 0) = "110" then
		need_bytes_lsb_next <= '1';
	  else
		bytes_left_next <= to_integer(unsigned(decin(2 downto 0)));
	  end if;
    end if;
  end if;
end process;

end rtl;

