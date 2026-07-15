#include <QuantumCircuit.hpp>
#include <algorithm>
#include <bit>
#include <iostream>
#include <numeric>
#include <set>
#include <stdexcept>

QuantumCircuit build_simon_qc_pass(const QuantumCircuit &oracle, size_t oracle_size)
{
	QuantumCircuit simon_qc;

	std::vector<size_t> input_register(oracle_size);
	std::iota(input_register.begin(), input_register.end(), 0);

	for (size_t i : input_register)
		simon_qc.add_gate(GateKind::H, {i});

	simon_qc += oracle;

	for (size_t i : input_register)
		simon_qc.add_gate(GateKind::H, {i});

	simon_qc.add_measure(MeasureKind::MULTIPLE_QUBIT, input_register);

	return simon_qc;
}

inline size_t pivot_index(size_t x) { return std::bit_width(x) - 1; }

size_t gaussian_bit_elimination(const std::vector<size_t>& pivot_points, size_t value)
{
	while (value != 0)
	{
		size_t pivot_idx = pivot_index(value);

		if (pivot_points[pivot_idx] == 0)
			return value;

		value ^= pivot_points[pivot_idx];
	}

	return 0;
}

size_t solve_simon_algo(const QuantumCircuit &oracle, size_t oracle_size)
{
	if(oracle_size < 2)
		throw std::invalid_argument("Simon algorithm need to have an oracle_size >= 2");
	
	QuantumCircuit simon_qc = build_simon_qc_pass(oracle, oracle_size);

	std::vector<size_t> pivot_points(oracle_size, 0);

	size_t rank = 0;

	while (rank != oracle_size - 1)
	{
		StateVector state(oracle_size * 2);
		std::vector<MeasureResult> measure;
		simon_qc.apply(state, measure);
	
		size_t result = measure[0].second;

		if (result == 0)
			continue;

		size_t reduce = gaussian_bit_elimination(pivot_points, result);
		if (reduce == 0)
			continue;
		
		pivot_points[pivot_index(reduce)] = reduce;
		rank++;
	}

	size_t solution = 0;

	for (size_t i = 0; i != pivot_points.size(); ++i)
	{
		const size_t row = pivot_points[i];
		const size_t pivot = std::bit_floor(row);
		const size_t indexes = row ^ pivot;

		const bool pivot_value = (std::popcount(indexes & solution) & 1u) != 0;

		if (pivot_value)
			solution |= pivot;
		else
			solution &= ~pivot;
	}

	return solution;
}

QuantumCircuit build_simon_oracle_101()
{
	QuantumCircuit qc;

	qc.add_gate(GateKind::CNOT, {0, 3});
	qc.add_gate(GateKind::CNOT, {2, 3});
	qc.add_gate(GateKind::CNOT, {1, 4});

	return qc;
}

QuantumCircuit build_simon_oracle_1011()
{
	QuantumCircuit qc;

	qc.add_gate(GateKind::CNOT, {0, 4});
	qc.add_gate(GateKind::CNOT, {3, 4});

	qc.add_gate(GateKind::CNOT, {1, 5});
	qc.add_gate(GateKind::CNOT, {3, 5});

	qc.add_gate(GateKind::CNOT, {2, 6});

	return qc;
}

QuantumCircuit build_simon_oracle_10101()
{
	QuantumCircuit qc;

	qc.add_gate(GateKind::CNOT, {0, 5});
	qc.add_gate(GateKind::CNOT, {4, 5});

	qc.add_gate(GateKind::CNOT, {1, 6});

	qc.add_gate(GateKind::CNOT, {2, 7});
	qc.add_gate(GateKind::CNOT, {4, 7});

	qc.add_gate(GateKind::CNOT, {3, 8});

	return qc;
}
