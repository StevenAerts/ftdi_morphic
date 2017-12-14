library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity led_driver is
   port (
	  clk50     : in  std_logic;       -- 50MHz clock input
      rst       : in  std_logic;       -- Active high reset

	  led_send	: in  std_logic;
	  led_byte	: in  std_logic_vector(7 downto 0);
	  led_byte_rd	: out std_logic;
	  led_pulse	: out std_logic
  );
end led_driver;

architecture rtl of led_driver is

signal	led_pulse_int, led_pulse_next		: std_logic;
signal	led_bit_cnt, led_bit_cnt_next 		: integer range 0 to 7;
signal	led_pulse_cnt, led_pulse_cnt_next 	: integer range 0 to 63;
signal  led_wait_cnt, led_wait_cnt_next 	: integer range 0 to 6;
signal  led_state, led_state_next 			: integer range 0 to 3;

begin

out_asn: led_pulse <= led_pulse_int;

reg_proc: process(clk50)
begin
  if rising_edge(clk50) then
	if rst = '1' then
		led_bit_cnt <= 0;
		led_pulse_cnt <= 0;
		led_wait_cnt <= 0;
		led_state <= 0;
		led_pulse_int <= '0';
	else
		led_bit_cnt <= led_bit_cnt_next;
		led_pulse_cnt <= led_pulse_cnt_next;
		led_wait_cnt <= led_wait_cnt_next;
		led_state <= led_state_next;
		led_pulse_int <= led_pulse_next;
	end if;
  end if;
end process reg_proc;

next_proc: process(led_send, led_byte, led_pulse_int,
	led_state, led_bit_cnt, led_pulse_cnt, led_wait_cnt)
begin
  led_pulse_cnt_next <= led_pulse_cnt;
  led_bit_cnt_next <= led_bit_cnt;
  led_wait_cnt_next <= led_wait_cnt;
  led_state_next <= led_state;
  led_pulse_next <= led_pulse_int;
  led_byte_rd <= '0';
  
  -- counters, 
  if led_pulse_cnt = 61 then
	led_pulse_cnt_next <= 0;
	if led_bit_cnt = 0 then
		led_bit_cnt_next <= 7;
		if led_wait_cnt > 0 then
			led_wait_cnt_next <= led_wait_cnt -1;
		end if;
	else
		led_bit_cnt_next <= led_bit_cnt - 1;
	end if;
  else
	led_pulse_cnt_next <= led_pulse_cnt + 1;
  end if;

  case led_state is
	when 0 => 
	  if led_send = '1' then
		led_state_next <= 1;
		led_pulse_cnt_next <= 61;
		led_bit_cnt_next <= 0;
		led_wait_cnt_next <= 0;
	  end if;
	when 1 =>
	  if led_send = '0' then
		led_state_next <= 2;
	  elsif led_pulse_cnt = 61 and led_bit_cnt = 0 then					
		led_byte_rd <= '1';
	  end if;
	when 2 =>
	  if led_bit_cnt = 0 and led_pulse_cnt = 39 then
		led_wait_cnt_next <= 6;
		led_state_next <= 3;
	  end if;
	when 3 =>
	  if led_wait_cnt = 0 then
		led_state_next <= 0;
	  end if;
	when others =>
		led_state_next <= 0;
  end case;
   
  if (led_state = 1) or (led_state = 2) then 
	if led_pulse_cnt = 61 then
		led_pulse_next <= '1';
	end if;
	if led_byte(led_bit_cnt) = '0' and led_pulse_cnt = 19 then
		led_pulse_next <= '0';
	end if;
	if led_byte(led_bit_cnt) = '1' and led_pulse_cnt = 39 then
		led_pulse_next <= '0';
	end if;
  end if; 
  
end process next_proc;

end rtl;
