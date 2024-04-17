// Definition of a 2-input AND gate using Verilog

module and_gate(
    input A,     // First input
    input B,     // Second input
    output Y     // Output
);

assign Y = A & B;  // Perform logical AND on inputs A and B

endmodule
