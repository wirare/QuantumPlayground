#pragma once

#include <QuantumCircuit.hpp>

void apply_multi_control_phase_flip(QuantumCircuit &qc, std::vector<size_t> qubits);
QuantumCircuit phase_oracle_builder(const std::string& marked_bitstring);
QuantumCircuit get_diffuser_circuit(size_t qubit_count);
size_t grover_iteration_count(size_t qubit_count);
QuantumCircuit build_grover_algo_circuit(size_t qubit_count, const QuantumCircuit &oracle);
