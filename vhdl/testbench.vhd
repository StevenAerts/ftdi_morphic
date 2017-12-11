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

procedure writebytes(bytes: in integer_vector) is
begin
    rxfn <= '0';
	for ibyte in bytes'range loop
        datai <= std_logic_vector(to_unsigned(bytes(ibyte), datai'length));
        wait until rising_edge(clk);
        while rdn /= '0' loop
            wait until rising_edge(clk);
        end loop;
        write(my_line, id);
        write(my_line, string'(" write "));
        write(my_line, data);
        writeline(output, my_line);
    end loop;
    rxfn <= '1';
end writebytes;

procedure readbytes(bytes: out integer_vector) is

begin
    txen <= '0';
	for ibyte in bytes'range loop
        wait until rising_edge(clk);
        while wrn /= '0' loop
            wait until rising_edge(clk);
        end loop;
        write(my_line, id);
        write(my_line, string'(" read "));
        write(my_line, data);
        bytes(ibyte) := to_integer(unsigned(data));
        writeline(output, my_line);
    end loop;
    txen <= '1';
end readbytes;

variable bytes: integer_vector(3 downto 0) := (0,1,2,3);

begin
    datai <= (others => '0'); rxfn <= '1'; txen <= '1';

    writebytes(bytes);
    readbytes(bytes);
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


