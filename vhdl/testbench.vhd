library std;
use std.textio.all;           

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_textio.all;
use ieee.numeric_std.all;
 
entity fifo_stim is
  generic(
	clockperiod		: time := 17 ns;
    id 				: string := "fifo"
  );
  port(
	rst      		: in  	std_logic;
	data     		: inout std_logic_vector(7 downto 0);
	clk      		: out 	std_logic;
	rxfn     		: out 	std_logic;
	txen     		: out 	std_logic;
	rdn      		: in  	std_logic;
	wrn      		: in  	std_logic;
	oen      		: in  	std_logic
  );
end fifo_stim;

architecture proc of fifo_stim is

signal clki 		: std_logic := '0';
signal datai  		: std_logic_vector(7 downto 0) := "00000001";

begin

clk_out:  clk <= clki;
data_out: data <= datai when oen = '0' else (others => 'Z');

clk_proc: process
begin
    wait for clockperiod/2;
    clki <= not clki;
end process clk_proc;

stim_proc: process

variable my_line : line;

procedure writebyte(byte: in std_logic_vector(7 downto 0)) is
begin
    rxfn <= '0';
	datai <= byte;
	wait until rising_edge(clk);
	while rdn /= '0' loop
		wait until rising_edge(clk);
	end loop;
	write(my_line, id);
	write(my_line, string'(" write "));
	write(my_line, data);
	writeline(output, my_line);
    rxfn <= '1';
end writebyte;

procedure readbyte(byte: out std_logic_vector(7 downto 0)) is

begin
    txen <= '0';
	wait until rising_edge(clk);
	while wrn /= '0' loop
		wait until rising_edge(clk);
	end loop;
	write(my_line, id);
	write(my_line, string'(" read "));
	write(my_line, data);
	byte:= data;
	writeline(output, my_line);
    txen <= '1';
end readbyte;

constant syncA: std_logic_vector(7 downto 0) := "10101000";
constant syncB: std_logic_vector(7 downto 0) := "10111000";
constant loop4: std_logic_vector(7 downto 0) := "10101100";
constant linvn: std_logic_vector(7 downto 0) := "10111110";
variable rbyte: std_logic_vector(7 downto 0);

begin
    datai <= (others => '0'); rxfn <= '1'; txen <= '1';

    report "Test synchronization sequence";
    writebyte(syncA); readbyte(rbyte); assert rbyte=syncA report "read mismatch" severity warning;
    writebyte(syncA); readbyte(rbyte); assert rbyte=syncA report "read mismatch" severity warning;
    writebyte(syncA); writebyte(syncA); writebyte(syncA); writebyte(syncA);
    readbyte(rbyte);  readbyte(rbyte);  readbyte(rbyte);  readbyte(rbyte);
    writebyte(syncB); readbyte(rbyte); assert rbyte=syncB report "read mismatch" severity warning;
    report "Test loopback (4 bytes)";
    writebyte(loop4); readbyte(rbyte); assert rbyte=loop4 report "read mismatch" severity warning;
    writebyte("00000000"); writebyte("00000001"); 
    writebyte("00000010"); writebyte("00000011");
    readbyte(rbyte); assert rbyte="00000000" report "read mismatch" severity warning;
    readbyte(rbyte); assert rbyte="00000001" report "read mismatch" severity warning;
    readbyte(rbyte); assert rbyte="00000010" report "read mismatch" severity warning;
    readbyte(rbyte); assert rbyte="00000011" report "read mismatch" severity warning;
    report "Test loopback invert n (8 bytes)";
    writebyte(linvn); writebyte("00001000"); -- 8 bytes
    readbyte(rbyte); assert rbyte=linvn report "read mismatch" severity warning;
    readbyte(rbyte); assert rbyte="00001000" report "read mismatch" severity warning; 
    writebyte("00000000"); writebyte("00000001"); 
    writebyte("00000010"); writebyte("00000011");
    readbyte(rbyte); assert rbyte="11111111" report "read mismatch" severity warning;
    readbyte(rbyte); assert rbyte="11111110" report "read mismatch" severity warning;
    readbyte(rbyte); assert rbyte="11111101" report "read mismatch" severity warning;
    readbyte(rbyte); assert rbyte="11111100" report "read mismatch" severity warning;
    writebyte("00000100"); writebyte("00000101"); 
    writebyte("00000110"); writebyte("00000111");
    readbyte(rbyte); assert rbyte="11111011" report "read mismatch" severity warning;
    readbyte(rbyte); assert rbyte="11111010" report "read mismatch" severity warning;
    readbyte(rbyte); assert rbyte="11111001" report "read mismatch" severity warning;
    readbyte(rbyte); assert rbyte="11111000" report "read mismatch" severity warning;
    report "1-byte commands";
    writebyte("00001000"); 
    writebyte("00010000"); 
    writebyte("00100000"); 
    writebyte("01000000"); 
    writebyte("10000000"); 
    report "2,3,4,5-byte commands";
    writebyte("00001001"); writebyte("00000000");
    writebyte("00010010"); writebyte("00000000"); writebyte("00000001");
    writebyte("00100011"); writebyte("00000000"); writebyte("00000001"); writebyte("00000010");
    writebyte("01000100"); writebyte("00000000"); writebyte("00000001"); writebyte("00000010"); writebyte("00000011"); 
    writebyte("10000110"); writebyte("00000000"); writebyte("00000001"); writebyte("00000010"); writebyte("00000011"); writebyte("00000100"); 
    report "Test loopback (4 bytes)";
    writebyte(loop4); readbyte(rbyte); assert rbyte=loop4 report "read mismatch" severity warning;
    writebyte("00000000"); writebyte("00000001"); 
    writebyte("00000010"); writebyte("00000011");
    readbyte(rbyte); assert rbyte="00000000" report "read mismatch" severity warning;
    readbyte(rbyte); assert rbyte="00000001" report "read mismatch" severity warning;
    readbyte(rbyte); assert rbyte="00000010" report "read mismatch" severity warning;
    readbyte(rbyte); assert rbyte="00000011" report "read mismatch" severity warning;
    
    wait;
end process stim_proc;

end proc;

library ieee;
use ieee.std_logic_1164.all;

entity testbench is
end testbench;

architecture beh of testbench is

signal rst: std_logic;
signal clk50: std_logic := '0';
signal mdata,mdatai: std_logic_vector(7 downto 0);
signal mclk60,mrxfn,mtxen,mrdn,mwrn,moen,msndimm : std_logic;

component ftdi_morphic 
  port (
	clk50     		: in  std_logic;       -- 50MHz clock input unused
	rst       		: in  std_logic;       -- Active high reset via BD7

	mdata     		: inout std_logic_vector(7 downto 0);   -- Port A Data Bus
	mclk60    		: in std_logic;
	mrxfn     		: in std_logic;
	mtxen     		: in std_logic;
	mrdn      		: out std_logic;
	mwrn      : out std_logic;
	moen      : out std_logic;
	msndimm   : out std_logic;
	io				: inout std_logic_vector(79 downto 0)
  );
end component;

component fifo_stim is
  generic(
	clockperiod : time := 17 ns;
	id : string := "fifo"
  );
  port(
	rst      : in  std_logic;
	data     : inout std_logic_vector(7 downto 0);
	clk      : out std_logic;
	rxfn     : out std_logic;
	txen     : out std_logic;
	rdn      : in  std_logic;
	wrn      : in  std_logic;
	oen      : in  std_logic
  );
end component;


begin
i_fifo : ftdi_morphic 
   port map(                                 
      clk50=>clk50, 
      rst=>rst, 
      mdata=>mdata,
      mclk60=>mclk60,
      mrxfn=>mrxfn,
      mtxen=>mtxen,
      mrdn=>mrdn,
      mwrn=>mwrn,
      moen=>moen,
      msndimm=>msndimm
    );

i_mfifo_stim: fifo_stim
    generic map (
        clockperiod => 17 ns,
        id => "mfifo"
    )
    port map (
      rst=>rst,
      data=>mdata,
      clk=>mclk60,
      rxfn=>mrxfn,
      txen=>mtxen,
      rdn=>mrdn,
      wrn=>mwrn,
      oen=>moen
    );

    
rst_proc: process
begin
    rst <= '1';
    wait for 10 us;
    rst <= '0';
    wait;
end process rst_proc;

clk50_proc: process
begin
    wait for 10 ns;
    clk50 <= not clk50;
end process clk50_proc;

end beh;


