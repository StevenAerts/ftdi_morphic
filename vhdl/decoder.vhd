library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity decoder is
    port (
        clk50               : in    std_logic;
        rst                 : in    std_logic;

        dec_din             : in    std_logic_vector(7 downto 0);
        dec_din_rdy         : in    std_logic;
        dec_din_rd          : out   std_logic;

        dec_cmd             : out   std_logic_vector(4 downto 0);
        dec_cmd_new         : out   std_logic;
        
        app_din             : out   std_logic_vector(7 downto 0);
        app_din_rdy         : out   std_logic;
        app_din_rd          : in    std_logic;
        
        dec_iscmd           : out   std_logic;
        dec_islen           : out   std_logic;
        dec_isdata          : out   std_logic;
        dec_islast          : out   std_logic
    );
end decoder;

architecture rtl of decoder is

    signal dec_cmd_int      : std_logic_vector(4 downto 0);
    signal dec_cmd_next     : std_logic_vector(4 downto 0);
    signal dec_cmd_new_next : std_logic;
    signal dec_iscmd_int    : std_logic;
    signal dec_iscmd_next   : std_logic;
    signal dec_islen_int    : std_logic;
    signal dec_islen_next   : std_logic;
    signal app_din_int      : std_logic_vector(7 downto 0);
    signal app_din_next     : std_logic_vector(7 downto 0);
    signal app_din_rdy_int  : std_logic;
    signal app_din_rdy_next : std_logic;
  
    signal bytes_left       : integer range 0 to 65535;
    signal bytes_left_next  : integer range 0 to 65535;
    signal need_len_msb     : std_logic;
    signal need_len_msb_next: std_logic;
    signal need_len_lsb     : std_logic;
    signal need_len_lsb_next: std_logic;

    function to_stdlogic(inp : boolean) return std_logic is
    begin
        if inp then return '1'; else return '0'; end if;
    end to_stdlogic;
  
begin


out_asn: block
begin

    app_din         <= app_din_int;
    app_din_rdy     <= app_din_rdy_int;
    dec_cmd         <= dec_cmd_int;

    dec_iscmd       <= dec_iscmd_int;
    dec_islen       <= dec_islen_int;
    dec_isdata      <= not (dec_iscmd_int or dec_islen_int);
    dec_islast      <= '0'; --todo not (need_len_msb or need_len_lsb) and to_stdlogic(bytes_left = 0);

end block out_asn;

reg_proc: process(clk50)
begin
    if rising_edge(clk50) then
        if rst = '1' then
            dec_cmd_int         <= (others => '0');
            dec_cmd_new         <= '0';
            dec_iscmd_int       <= '0';
            dec_islen_int       <= '0';
            app_din_int         <= (others => '0');
            app_din_rdy_int     <= '0';
            bytes_left          <=  0;
            need_len_msb        <= '0';
            need_len_lsb        <= '0';
        else
            dec_cmd_int         <= dec_cmd_next;
            dec_cmd_new         <= dec_cmd_new_next;
            dec_iscmd_int       <= dec_iscmd_next;
            dec_islen_int       <= dec_islen_next;
            app_din_int         <= app_din_next;
            app_din_rdy_int     <= app_din_rdy_next;
            bytes_left          <= bytes_left_next;
            need_len_msb        <= need_len_msb_next;
            need_len_lsb        <= need_len_lsb_next;
        end if;
    end if;
end process reg_proc;

next_proc: process( dec_din, dec_cmd_int, dec_din_rdy, app_din_int, app_din_rdy_int, app_din_rd, 
    dec_iscmd_int, dec_islen_int, bytes_left, need_len_msb, need_len_lsb)
    
begin

    dec_din_rd <= '0';
    dec_cmd_next <= dec_cmd_int;
    dec_cmd_new_next <= '0';
    dec_iscmd_next <= dec_iscmd_int;
    dec_islen_next <= dec_islen_int;
    app_din_next <= app_din_int;
    app_din_rdy_next <= app_din_rdy_int;

    bytes_left_next <= bytes_left;
    need_len_msb_next <= need_len_msb;
    need_len_lsb_next <= need_len_lsb;

    if app_din_rd = '1' then
    
        dec_iscmd_next <= '0';
        dec_islen_next <= need_len_msb or need_len_lsb;
        app_din_rdy_next <= '0';
    
    elsif app_din_rdy_int = '1' then
    
        NULL;
    
    elsif dec_din_rdy = '1' then
        
        dec_din_rd <= '1';
        app_din_rdy_next <= '1';
        app_din_next <= dec_din;
        
        if need_len_msb = '1' then
        
            need_len_msb_next <= '0';
            bytes_left_next <= to_integer(unsigned(dec_din&"00000000"));
        
        elsif need_len_lsb = '1' then
        
            need_len_lsb_next <= '0';
            bytes_left_next <= to_integer(to_unsigned(bytes_left,16)
                (15 downto 7) & unsigned(dec_din));

        elsif bytes_left > 0 then
        
            bytes_left_next <= bytes_left - 1;
            
        else
            
            dec_cmd_next <= dec_din(7 downto 3);
            dec_cmd_new_next <= '1';
            dec_iscmd_next  <= '1';
            
            if dec_din(2 downto 0) = "111" then
            
                need_len_msb_next <= '1';
                need_len_lsb_next <= '1';
            
            elsif dec_din(2 downto 0) = "110" then
            
                need_len_lsb_next <= '1';
            
            else
            
                bytes_left_next <= to_integer(unsigned(dec_din(2 downto 0)));
            
            end if;
            
        end if;
        
    end if;
    
end process;

end rtl;

