#include "mysim.h"
struct and_gate_Inputs {
	Signal<1> B;
	Signal<1> A;

	template <typename T> void dump(T &out) {
		out("B", B);
		out("A", A);
	}
};

struct and_gate_Outputs {
	Signal<1> Y;

	template <typename T> void dump(T &out) {
		out("Y", Y);
	}
};

struct and_gate_State {

	template <typename T> void dump(T &out) {
	}
};

void and_gate(and_gate_Inputs const &input, and_gate_Outputs &output, and_gate_State const &current_state, and_gate_State &next_state)
{
	Signal<1> B = input.B;
	Signal<1> A = input.A;
	Signal<1> $and$tests_functional_backend_and_v_9$1$_Y = $and(A, B); //
	output.Y = $and$tests_functional_backend_and_v_9$1$_Y;
}
