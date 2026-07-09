#include <QuantumCircuit.hpp>
#include <numeric>
#include <stdexcept>

QuantumCircuit build_simon_algo_circuit(size_t qubit_count, const QuantumCircuit &oracle)
{
	QuantumCircuit simon_qc;
	
	if(qubit_count <= 3 || qubit_count % 2 != 0)
		throw std::invalid_argument("Simon algorithm need to have an even qubit_count >= 4");

	std::vector<size_t> input_register;
	std::vector<size_t> output_register;

	std::iota(input_register.begin(), input_register.end(), 0);
	std::iota(output_register.begin(), output_register.end(), qubit_count/2);

	for (size_t i = 0; i != qubit_count/2; i++)
		simon_qc.add_gate(GateKind::H, {i});

	simon_qc += oracle;

	for (size_t i = 0; i != qubit_count/2; i++)
		simon_qc.add_gate(GateKind::H, {i});
}
