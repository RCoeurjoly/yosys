library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

-- Entity declaration
entity Simple_Adder is
    Port ( A : in  STD_LOGIC_VECTOR (3 downto 0);
           B : in  STD_LOGIC_VECTOR (3 downto 0);
           Sum : out  STD_LOGIC_VECTOR (3 downto 0);
           Carry_Out : out  STD_LOGIC);
end Simple_Adder;

-- Architecture definition
architecture Behavioral of Simple_Adder is
begin
    process(A, B)
    variable temp_sum : STD_LOGIC_VECTOR (4 downto 0);
    begin
        -- Perform the addition
        temp_sum := ('0' & A) + ('0' & B);
        -- Assign the lower 4 bits to Sum
        Sum <= temp_sum(3 downto 0);
        -- Assign the MSB to Carry_Out
        Carry_Out <= temp_sum(4);
    end process;
end Behavioral;
