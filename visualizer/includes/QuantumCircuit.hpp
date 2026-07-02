#pragma once

#include <IGate.hpp>
#include <Gate.hpp>
#include <StateVector.hpp>
#include <ostream>
#include <vector>

#define GATE_CASE(name) case GateKind::name: return Gate::name()

inline const IGate &kind_to_gate(GateKind kind)
{
	switch (kind)
	{
		GATE_CASE(I);
		GATE_CASE(X);
		GATE_CASE(Y);
		GATE_CASE(Z);
		GATE_CASE(H);
		GATE_CASE(S);
		GATE_CASE(Sdg);
		GATE_CASE(T);
		GATE_CASE(Tdg);
		GATE_CASE(P);
		GATE_CASE(RZ);
		GATE_CASE(RX);
		GATE_CASE(RY);
		GATE_CASE(CX);
		GATE_CASE(CZ);
		GATE_CASE(CP);
		GATE_CASE(CRZ);
		GATE_CASE(CRX);
		GATE_CASE(CRY);
		GATE_CASE(SWAP);
		GATE_CASE(ISWAP);
		GATE_CASE(RZZ);
		GATE_CASE(CCX);
		GATE_CASE(CCZ);
		GATE_CASE(CSWAP);
		GATE_CASE(MCX);
		GATE_CASE(MCZ);
		GATE_CASE(MCP);
		GATE_CASE(RXX);
		GATE_CASE(RYY);
		GATE_CASE(RZX);
		GATE_CASE(ECR);
		GATE_CASE(U);
		GATE_CASE(SX);
		GATE_CASE(CH);
		GATE_CASE(SQRTSWAP);
	}
}

struct CircuitOperation
{
	GateKind kind;

	std::vector<size_t> qubits;
	std::vector<double> params;
};

template<typename K>
inline std::ostream &operator<<(std::ostream &os, const std::vector<K> &vec)
{
	os << "{";
	for (size_t i = 0; i != vec.size(); i++)
	{
		os << vec[i];
		if (i != vec.size()-1)
			os << ", ";
	}
	os << "}";
	return os;
}

inline std::ostream &operator<<(std::ostream &os, const CircuitOperation &op)
{
	os << "{Gate: " << kind_to_gate(op.kind).name() << ", Qubit(s): " << op.qubits << ", Param(s): " << op.params << "}";
	return os;
}

class QuantumCircuit
{
	public:
		QuantumCircuit() = default;

		QuantumCircuit& add_gate(GateKind gate,
								 std::initializer_list<size_t> qubits,
								 std::initializer_list<double> angular_params = {})
		{
			circuit.emplace_back(gate, qubits, angular_params);
			return *this;
		}

		QuantumCircuit& add_gate(GateKind gate,
								 std::vector<size_t> qubits,
								 std::vector<double> angular_params = {})
		{
			circuit.emplace_back(gate, std::move(qubits), std::move(angular_params));
			return *this;
		}

		QuantumCircuit& add_gate(GateKind gate,
								 std::vector<size_t> qubits,
								 std::initializer_list<double> angular_params = {})
		{
			circuit.emplace_back(gate, std::move(qubits), angular_params);
			return *this;
		}

		void apply(StateVector& sv) const
		{
			for (const CircuitOperation& op : circuit)
				sv.gate(kind_to_gate(op.kind), op.qubits, op.params);
		}

		QuantumCircuit &operator+=(const QuantumCircuit &other)
		{
			circuit.reserve(circuit.size() + other.circuit.size());
			circuit.insert(circuit.end(), other.circuit.begin(), other.circuit.end());
			return *this;
		}

		QuantumCircuit operator+(const QuantumCircuit &other) const
		{
			QuantumCircuit result = *this;
			result += other;
			return result;
		}

		size_t size() const { return circuit.size(); }

		const std::vector<CircuitOperation> &get_raw_circuit() const { return circuit; }

		friend std::ostream &operator<<(std::ostream &os, const QuantumCircuit &qc);

	private:
		std::vector<CircuitOperation> circuit;
};

inline std::ostream &operator<<(std::ostream &os, const QuantumCircuit &qc)
{
	os << "QuantumCircuit: \n";
	for (const CircuitOperation &op : qc.circuit)
		os << op << "\n";
	os << "\n";
	return os;
}
