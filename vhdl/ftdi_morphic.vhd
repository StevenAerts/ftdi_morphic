library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity ftdi_morphic is

	port (
		clk50     			: in  	std_logic;
		rst       			: in  	std_logic;

		mdata     			: inout std_logic_vector(7 downto 0);
		mclk60    			: in 	std_logic;
		mrxfn     			: in 	std_logic;
		mtxen     			: in 	std_logic;
		mrdn      			: out 	std_logic;
		mwrn      			: out 	std_logic;
		moen      			: out 	std_logic;
		msndimm   			: out 	std_logic;
		
		io        			: inout std_logic_vector(79 downto 0)
	);
end ftdi_morphic;


architecture rtl of ftdi_morphic is


component sync_fifo 

	port(                                 
		reset_n   			: in  	std_logic;

		s_clk    			: in  	std_logic;
		s_wr     			: in  	std_logic;
		s_txe    			: out 	std_logic;
		s_dbin   			: in  	std_logic_vector(7 downto 0);

		d_clk    			: in  	std_logic;
		d_rd     			: in  	std_logic;
		d_rxf    			: out 	std_logic;
		d_dbout  			: out 	std_logic_vector(7 downto 0)
	);
end component;


component hs245_sif 

	port(                                 
		clk        			: in  	std_logic;
		reset_n    			: in  	std_logic;
		
		ext_txe    			: in	std_logic;
		ext_rxf    			: in  	std_logic;
		ext_wr     			: out 	std_logic;
		ext_rd     			: out 	std_logic;
		ext_oe     			: out 	std_logic;
		ext_datain 			: in  	std_logic_vector(7 downto 0);
		ext_dataout 		: out  	std_logic_vector(7 downto 0);  
		
		int_datain  		: in  	std_logic_vector(7 downto 0);
		int_rxf     		: in  	std_logic;                   
		int_rd      		: out 	std_logic;                   
		
		int_dataout 		: out 	std_logic_vector(7 downto 0);
		int_txe     		: in  	std_logic;
		int_wr      		: out 	std_logic 
	);
end component;


component decoder is

    port (
        clk50               : in    std_logic;
        rst                 : in    std_logic;

        dec_din             : in    std_logic_vector(7 downto 0);
        dec_din_rdy         : in    std_logic;
        dec_din_rd          : out   std_logic;

        dec_cmd             : out   std_logic_vector(4 downto 0);
        dec_cmd_new         : out   std_logic;
        
        dec_iscmd           : out   std_logic;
        dec_islen           : out   std_logic;
        dec_isdata          : out   std_logic;
        dec_islast          : out   std_logic;
        
        app_din            	: out   std_logic_vector(7 downto 0);
        app_din_rdy        	: out   std_logic;
        app_din_rd         	: in    std_logic
    );
end component;


component application is

	port (
		clk50         		: in  	std_logic;
		rst           		: in  	std_logic;

        dec_cmd             : in    std_logic_vector(4 downto 0);
        dec_cmd_new         : in    std_logic;
        
        dec_iscmd           : in    std_logic;
        dec_islen           : in    std_logic;
        dec_isdata          : in    std_logic;
        dec_islast          : in    std_logic;

		app_din       		: in    std_logic_vector(7 downto 0);
		app_din_rdy   		: in    std_logic;
		app_din_rd    		: out   std_logic;

		app_dout      		: out   std_logic_vector(7 downto 0);
		app_dout_rdy  		: in    std_logic;
		app_dout_wr   		: out   std_logic;

		io            		: inout std_logic_vector(79 downto 0)
	);
end component;

	signal reset_n 			: std_logic;

	signal mrxf, mrd		: std_logic;
	signal mtxe, moe, mwr	: std_logic;
	signal mdatain, mdataout: std_logic_vector(7 downto 0);

	signal s1_din, s2_dout 	: std_logic_vector(7 downto 0);
	signal s1_wr,s1_txe		: std_logic;
	signal s2_rd,s2_rxf 	: std_logic;

	signal app_din, app_dout: std_logic_vector(7 downto 0);
	signal app_din_rd		: std_logic;
	signal app_din_rdy		: std_logic;
	signal app_dout_wr		: std_logic;
	signal app_dout_rdy		: std_logic;

	signal dec_cmd			: std_logic_vector(4 downto 0);
	signal dec_cmd_new		: std_logic;
	
	signal dec_iscmd		: std_logic;
	signal dec_islen		: std_logic;
	signal dec_isdata		: std_logic;
	signal dec_islast 		: std_logic;
	
	signal dec_din			: std_logic_vector(7 downto 0);
	signal dec_din_rdy		: std_logic;
	signal dec_din_rd		: std_logic;
  
begin


local_asn: block
begin
  
	mdatain 	<= mdata;
	mdata 		<= mdataout when moe='0' else (others => 'Z');
	
	reset_n 	<= not rst;
	mrxf 		<= not mrxfn;
	mtxe 		<= not mtxen;
	moen 		<= not moe;
	mrdn 		<= not mrd;
	mwrn 		<= not mwr;
	
	msndimm 	<= '1';

end block local_asn;


sync1 : sync_fifo
 
	port map(                                 
		reset_n  			=> reset_n,

		s_clk    			=> mclk60,
		s_wr     			=> s1_wr,
		s_txe    			=> s1_txe,
		s_dbin   			=> s1_din,

		d_clk    			=> clk50,
		d_rd     			=> dec_din_rd,
		d_rxf    			=> dec_din_rdy,
		d_dbout  			=> dec_din
	);


sync2 : sync_fifo 

	port map(                                 
		reset_n  			=> reset_n,

		s_clk    			=> clk50,
		s_wr     			=> app_dout_wr,
		s_txe    			=> app_dout_rdy,
		s_dbin   			=> app_dout,

		d_clk    			=> mclk60,
		d_rd     			=> s2_rd,
		d_rxf    			=> s2_rxf,
		d_dbout  			=> s2_dout
	);


xfer1 : hs245_sif 

	port map(                                 
		clk        			=> mclk60,
		reset_n    			=> reset_n,

		ext_txe    			=> mtxe,
		ext_rxf    			=> mrxf,
		ext_wr     			=> mwr,
		ext_rd     			=> mrd,
		ext_oe     			=> moe,
		ext_datain 			=> mdatain,
		ext_dataout 		=> mdataout,

		int_datain  		=> s2_dout,
		int_rxf     		=> s2_rxf,
		int_rd      		=> s2_rd,
		int_dataout 		=> s1_din,
		int_txe     		=> s1_txe,
		int_wr      		=> s1_wr
	);


dec: decoder

    port map(
        clk50               => clk50,
        rst                 => rst,

		dec_din     		=> dec_din,
		dec_din_rdy  		=> dec_din_rdy,
		dec_din_rd			=> dec_din_rd,
      
        dec_cmd             => dec_cmd,
        dec_cmd_new         => dec_cmd_new,
        
        dec_iscmd           => dec_iscmd,
        dec_islen           => dec_islen,
        dec_isdata          => dec_isdata,
        dec_islast          => dec_islast,

        app_din            	=> app_din,
        app_din_rdy        	=> app_din_rdy,
        app_din_rd         	=> app_din_rd
	);


app: application

	port map (
		clk50         		=> clk50,
		rst           		=> rst,

        dec_cmd             => dec_cmd,
        dec_cmd_new         => dec_cmd_new,
        
        dec_iscmd           => dec_iscmd,
        dec_islen           => dec_islen,
        dec_isdata          => dec_isdata,
        dec_islast          => dec_islast,

		app_din       		=> app_din,
		app_din_rdy   		=> app_din_rdy,
		app_din_rd    		=> app_din_rd,

		app_dout      		=> app_dout,
		app_dout_rdy  		=> app_dout_rdy,
		app_dout_wr   		=> app_dout_wr,

		io            		=> io
	);

end rtl;

