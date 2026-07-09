#include <Algorithms/Grover.hpp>
#include <QuantumCircuit.hpp>

void apply_multi_control_phase_flip(QuantumCircuit &qc, std::vector<size_t> qubits)
{
	switch (qubits.size())
	{
		case 0:
			return;

		case 1:
			qc.add_gate(GateKind::Z, {qubits[0]});
			return;

		default:
			qc.add_gate(GateKind::MCP, qubits, {std::numbers::pi});
			return;
	}
}

QuantumCircuit phase_oracle_builder(const std::string& marked_bitstring)
{
	QuantumCircuit qc;

	const size_t n = marked_bitstring.size();

	std::vector<size_t> zero_qubits;

	for (size_t pos = 0; pos != n; ++pos)
	{
		const char bit = marked_bitstring[pos];

		if (bit != '0' && bit != '1')
			throw std::invalid_argument("marked_bitstring must contain only 0 or 1");

		// String is displayed as |q(n-1)...q0>
		const size_t q = n - 1 - pos;

		if (bit == '0')
			zero_qubits.push_back(q);
	}

	for (size_t q : zero_qubits)
		qc.add_gate(GateKind::X, {q});

	std::vector<size_t> qubits(n);
	std::iota(qubits.begin(), qubits.end(), size_t{0});

	apply_multi_control_phase_flip(qc, qubits);

	for (size_t q : zero_qubits)
		qc.add_gate(GateKind::X, {q});

	return qc;
}

QuantumCircuit get_diffuser_circuit(size_t qubit_count)
{
	QuantumCircuit qc;

	for (size_t q = 0; q != qubit_count; ++q)
		qc.add_gate(GateKind::H, {q});

	for (size_t q = 0; q != qubit_count; ++q)
		qc.add_gate(GateKind::X, {q});

	std::vector<size_t> all_qubits;
	all_qubits.reserve(qubit_count);

	for (size_t q = 0; q != qubit_count; ++q)
		all_qubits.push_back(q);

	// Phase flip |111...111>.
	qc.add_gate(GateKind::MCP, all_qubits, {std::numbers::pi});

	for (size_t q = 0; q != qubit_count; ++q)
		qc.add_gate(GateKind::X, {q});

	for (size_t q = 0; q != qubit_count; ++q)
		qc.add_gate(GateKind::H, {q});

	return qc;
}

size_t grover_iteration_count(size_t qubit_count)
{
	const double state_count = static_cast<double>(size_t{1} << qubit_count);

	return std::max<size_t>(
		1,
		static_cast<size_t>(
			std::floor((std::numbers::pi / 4.0) * std::sqrt(state_count))
		)
	);
}

QuantumCircuit build_grover_algo_circuit(size_t qubit_count, const QuantumCircuit &oracle)
{
	static const QuantumCircuit diffuser_qc = get_diffuser_circuit(qubit_count);

	QuantumCircuit grover_qc;

	for (size_t q = 0; q != qubit_count; q++)
		grover_qc.add_gate(GateKind::H, {q});

	const size_t iterations = grover_iteration_count(qubit_count);

	for (size_t i = 0; i != iterations; i++)
	{
		grover_qc += oracle;
		grover_qc += diffuser_qc;
	}

	return grover_qc;
}

