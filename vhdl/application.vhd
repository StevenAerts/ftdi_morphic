library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity application is
   port (-- Inputs
	  clk50     	: in  std_logic;       -- 50MHz clock input
      rst       	: in  std_logic;       -- Active high reset

      app_din   	: in 	std_logic_vector(7 downto 0);
      app_din_rdy	: in	std_logic;
      app_din_rd	: out	std_logic;

      app_dout  	: out 	std_logic_vector(7 downto 0);
      app_dout_rdy	: in	std_logic;
      app_dout_wr	: out	std_logic;
      
      dec_next	: out	std_logic;
      dec_islen : in	std_logic;
      dec_isdata: in	std_logic;
	  cmd		: in	std_logic_vector(4 downto 0);
	  cmd_new	: in	std_logic;
	  
	  io		: inout std_logic_vector(79 downto 0)
);
end application;

architecture rtl of application is

  signal io_in, io_out, io_out_next, io_oe, io_oe_next, 
		 io_fn, io_fn_en, io_fn_en_next: std_logic_vector(79 downto 0);

  signal led_send, led_byte_rd, led_pulse	: std_logic;
  signal led_byte, led_byte_next: std_logic_vector(7 downto 0);
  signal i_payload, i_payload_next: integer range 0 to 7;
  
  signal app_din_rd_int: std_logic;
  
component led_driver is
   port (
	  clk50     : in  std_logic;       -- 50MHz clock input
      rst       : in  std_logic;       -- Active high reset

	  led_send	: in  std_logic;
	  led_byte	: in  std_logic_vector(7 downto 0);
	  led_byte_rd	: out std_logic;
	  led_pulse	: out std_logic
  );
end component;

begin

leds: led_driver
   port map (
	  clk50     => clk50,
      rst       => rst,

	  led_send	=> led_send,
	  led_byte	=> led_byte,
	  led_byte_rd => led_byte_rd,
	  led_pulse	=> led_pulse
  );

iofn_map: io_fn <= (
	0 => led_pulse, 
	others => '0');

app_din_rd <= app_din_rd_int;
	
reg_proc: process(clk50)
begin
  if rising_edge(clk50) then
	io_in <= io;
	if rst = '1' then
		io_out <= (others => '0');
		io_oe <= (0 => '1', others => '0');
		io_fn_en <= (0 => '1', others => '0');
		led_byte <= (others => '0');
		i_payload <= 0;
	else
		io_out <= io_out_next;
		io_oe <= io_oe_next;
		io_fn_en <= io_fn_en_next;
		led_byte <= led_byte_next;
		i_payload <= i_payload_next;
	end if;
  end if;
end process reg_proc;

cmd_proc: process(cmd, cmd_new, dec_islen, dec_isdata, i_payload, 
	app_din, app_din_rdy, app_din_rd_int, app_dout_rdy,
	io_in, io_out, io_oe,
	led_byte, led_byte_rd
	)

--variable iosel : integer range io'range;

begin
  app_dout <= (others => '0');
  app_din_rd_int <= '0'; 
  app_dout_wr <= '0';
  dec_next <= '0';
  io_out_next <= io_out;
  io_oe_next <= io_oe;
  io_fn_en_next <= io_fn_en;
  led_send <= '0';
  led_byte_next <= led_byte;
  i_payload_next <= i_payload;
	
	if dec_isdata = '0' then
		i_payload_next <= 0;
	elsif app_din_rd_int = '1' and i_payload < 7 then
		i_payload_next <= i_payload + 1;
	end if;
	  case cmd is
		--when "00000" => -- I/O config
		--	if dec_isdata then
		--		if to_integer(unsigned(app_din)) <= io'high then
		--		io_fn_en_next
		--	end if;
		when "10000" => -- leds
		  if cmd_new = '0' and dec_islen = '0' then
			  led_send <= '1';
          end if;
		  if app_din_rdy = '1' then
			if cmd_new = '1' or dec_islen = '1' then
			  dec_next <= '1'; 
			  app_din_rd_int <= '1';
			else
			  if led_byte_rd = '1' then
				  dec_next <= '1'; 
				  app_din_rd_int <= '1';
				  led_byte_next <= app_din;
			  end if;
			end if;
		  end if;
		  
	    when "10101" => -- loopback
		  if app_din_rdy = '1' and app_dout_rdy = '1' then
			dec_next <= '1'; 
			app_din_rd_int <= '1';
			app_dout_wr <= '1'; 
			app_dout <= app_din;
		  end if;
	    when "10111" => -- loopback invert
		  if app_din_rdy = '1' and app_dout_rdy = '1' then
			dec_next <= '1'; 
			app_din_rd_int <= '1';
			app_dout_wr <= '1'; 
			if cmd_new = '1' or dec_islen = '1' then
				app_dout <= app_din;
			else
				app_dout <= not app_din;
			end if;
		  end if;
		when others => 
		  if app_din_rdy = '1' then --and app_dout_rdy = '1' then
			dec_next <= '1'; 
			app_din_rd_int <= '1';
			--app_dout_wr <= '1'; 
			--app_dout <= "11100000";
		  end if;
	  end case;
  --end if;
end process cmd_proc;

iogen_block : block
  begin
	iogen: for i in io'range generate
		io(i) <= 'Z' when io_oe(i) = '0' else
		         (io_fn(i) and io_fn_en(i)) or (io_out(i) and not io_fn_en(i));
	end generate iogen;
end block iogen_block;

end rtl;
