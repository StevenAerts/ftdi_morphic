library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity application is

    port (
        clk50               : in    std_logic;
        rst                 : in    std_logic;

        app_din             : in    std_logic_vector(7 downto 0);
        app_din_rdy         : in    std_logic;
        app_din_rd          : out   std_logic;

        app_dout            : out   std_logic_vector(7 downto 0);
        app_dout_rdy        : in    std_logic;
        app_dout_wr         : out   std_logic;

        io                  : inout std_logic_vector(79 downto 0)
    );
end application;

architecture rtl of application is

    signal io_in            : std_logic_vector(io'range);
    
    signal io_out           : std_logic_vector(io'range);
    signal io_out_next      : std_logic_vector(io'range);
    signal io_oe            : std_logic_vector(io'range);
    signal io_oe_next       : std_logic_vector(io'range);
    
    signal io_fn            : std_logic_vector(io'range);
    signal io_fn_en         : std_logic_vector(io'range);
    signal io_fn_en_next    : std_logic_vector(io'range);
    

    signal led_send         : std_logic;
    signal led_pulse        : std_logic;
    signal led_byte_rd      : std_logic;
    signal led_byte         : std_logic_vector(7 downto 0);
    signal led_byte_next    : std_logic_vector(7 downto 0);

    signal i_payload        : integer range 0 to 7;
    signal i_payload_next   : integer range 0 to 7;

    signal app_fw_din       : std_logic_vector(7 downto 0);
    signal app_fw_din_rdy   : std_logic;
    signal app_fw_din_rd    : std_logic;

    signal dec_din          : std_logic_vector(7 downto 0);
    signal dec_din_rdy      : std_logic;
    signal dec_din_rd       : std_logic;

    signal dec_cmd          : std_logic_vector(4 downto 0);
    signal dec_cmd_new      : std_logic;
        
    signal dec_iscmd        : std_logic;
    signal dec_islen        : std_logic;
    signal dec_isdata       : std_logic;
    signal dec_islast       : std_logic;

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
        
        app_din             : out   std_logic_vector(7 downto 0);
        app_din_rdy         : out   std_logic;
        app_din_rd          : in    std_logic
    );
end component;

  
component led_driver is

   port (

      clk50                 : in    std_logic;
      rst                   : in    std_logic;

      led_send              : in    std_logic;
      led_byte              : in    std_logic_vector(7 downto 0);
      led_byte_rd           : out   std_logic;
      led_pulse             : out   std_logic

  );

end component;

begin

leds: led_driver

    port map (
        clk50               => clk50,
        rst                 => rst,

        led_send            => led_send,
        led_byte            => led_byte,
        led_byte_rd         => led_byte_rd,
        led_pulse           => led_pulse
    );


dec: decoder

    port map(
        clk50               => clk50,
        rst                 => rst,

        dec_din             => app_din,
        dec_din_rdy         => app_din_rdy,
        dec_din_rd          => app_din_rd,
      
        dec_cmd             => dec_cmd,
        dec_cmd_new         => dec_cmd_new,
        
        dec_iscmd           => dec_iscmd,
        dec_islen           => dec_islen,
        dec_isdata          => dec_isdata,
        dec_islast          => dec_islast,

        app_din             => app_fw_din,
        app_din_rdy         => app_fw_din_rdy,
        app_din_rd          => app_fw_din_rd
    );

iofn_map: io_fn <= ( 0 => led_pulse, others => '0');

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

cmd_proc: process(

    dec_cmd, dec_cmd_new, 
    dec_iscmd, dec_islen, dec_isdata, dec_islast,
    i_payload, 
    app_fw_din, app_fw_din_rdy, app_fw_din_rd, app_dout_rdy,
    io_in, io_out, io_oe,
    led_byte, led_byte_rd
)

begin
    app_dout <= (others => '0');
    app_fw_din_rd <= '0'; 
    app_dout_wr <= '0';
    io_out_next <= io_out;
    io_oe_next <= io_oe;
    io_fn_en_next <= io_fn_en;
    led_send <= '0';
    led_byte_next <= led_byte;
    i_payload_next <= i_payload;
    
    if dec_isdata = '0' then
        i_payload_next <= 0;
    elsif app_fw_din_rd = '1' and i_payload < 7 then
        i_payload_next <= i_payload + 1;
    end if;
    
    case dec_cmd is
        --when "00000" => -- I/O config
        --  if dec_isdata then
        --      if to_integer(unsigned(app_fw_din)) <= io'high then
        --      io_fn_en_next
        --  end if;
        
        when "10000" => -- leds
        
            if dec_isdata = '1' then
                led_send <= '1';
            end if;
              
            if app_fw_din_rdy = '1' then
            
                if dec_isdata = '0' then
                
                    app_fw_din_rd <= '1';
                
                elsif led_byte_rd = '1' then
                
                    app_fw_din_rd <= '1';
                    led_byte_next <= app_fw_din;
                
                end if;
            end if;
          
        when "10101" => -- loopback
        
            if app_fw_din_rdy = '1' and app_dout_rdy = '1' then
          
                app_fw_din_rd <= '1';
                app_dout_wr <= '1'; 
                app_dout <= app_fw_din;
                
            end if;

        when "10111" => -- loopback invert
        
            if app_fw_din_rdy = '1' and app_dout_rdy = '1' then
            
                app_fw_din_rd <= '1';
                app_dout_wr <= '1'; 
                
                if dec_isdata = '1' then
                    app_dout <= not app_fw_din;
                else
                    app_dout <= app_fw_din;
                end if;
                
            end if;
            
        when others => 
        
            if app_fw_din_rdy = '1' then
            
                app_fw_din_rd <= '1';

            end if;
    end case;

end process cmd_proc;

iogen_block : block
begin
    iogen: for i in io'range generate
        io(i) <= 
            'Z' when io_oe(i) = '0' else
            (io_fn(i) and io_fn_en(i)) or (io_out(i) and not io_fn_en(i));
    end generate iogen;
end block iogen_block;

end rtl;
