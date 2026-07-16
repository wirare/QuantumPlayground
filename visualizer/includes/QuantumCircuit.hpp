#pragma once

#include <IGate.hpp>
#include <Gate.hpp>
#include <StateVector.hpp>
#include <algorithm>
#include <format>
#include <ostream>
#include <stdexcept>
#include <string>
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
		GATE_CASE(CNOT);
	}
}

enum class MeasureKind
{
	NO_MEASURE,
	SINGLE_QUBIT,
	MULTIPLE_QUBIT,
	ALL_QUBIT,
};

struct CircuitOperation
{
	GateKind kind = GateKind::I;

	std::vector<size_t> qubits = {};
	std::vector<double> params = {};
	
	bool is_measure = false;
	MeasureKind m_kind = MeasureKind::NO_MEASURE;
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
	os << kind_to_gate(op.kind).name();

	if (op.params.size() != 0)
	{
		os << "[";
		for (size_t i = 0; i != op.params.size(); i++)
		{
			os << std::format("{:.4g}", op.params[i]);
			if (i != op.params.size() - 1)
				os << ", ";
		}
		os << "]";
	}
	
	os << "(";
	for (size_t i = 0; i != op.qubits.size(); i++)
	{
		os << "q" << std::to_string(op.qubits[i]);
		if (i != op.qubits.size() - 1)
			os << ", ";
	}
	os << ")";

	return os;
}

using MeasureResult = std::pair<std::vector<size_t>, size_t>;

class QuantumCircuit
{
	public:
		QuantumCircuit() = default;

		QuantumCircuit(const std::vector<CircuitOperation> &ops): circuit(ops) {};

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

		QuantumCircuit& add_gate(const CircuitOperation &op)
		{
			circuit.emplace_back(op);
			return *this;
		}

		QuantumCircuit& add_measure(MeasureKind measure, std::vector<size_t> qubits = {})
		{
			switch (measure)
			{
				case MeasureKind::NO_MEASURE: break;
				case MeasureKind::SINGLE_QUBIT:
				{
					if (qubits.size() != 1)
						throw std::invalid_argument("Single qubits measure need 1 qubit");
					circuit.emplace_back(CircuitOperation{.qubits = qubits, .is_measure = true, .m_kind = MeasureKind::SINGLE_QUBIT});	
					break;
				}
				case MeasureKind::MULTIPLE_QUBIT:
				{
					if (qubits.empty())
						throw std::invalid_argument("Multiple qubits measure need qubits");
					circuit.emplace_back(CircuitOperation{.qubits = qubits, .is_measure = true, .m_kind = MeasureKind::MULTIPLE_QUBIT});
					break;
				}
				case MeasureKind::ALL_QUBIT:
				{
					circuit.emplace_back(CircuitOperation{.is_measure = true, .m_kind = MeasureKind::ALL_QUBIT});
					break;
				}
			}

			return *this;
		}

		void apply(StateVector& sv, std::vector<MeasureResult> &measure_results) const
		{
			for (const CircuitOperation& op : circuit)
			{	
				if (!op.is_measure)
					sv.gate(kind_to_gate(op.kind), op.qubits, op.params);
				else
				{
					switch (op.m_kind)
					{
						case MeasureKind::NO_MEASURE: continue;
						case MeasureKind::SINGLE_QUBIT:
						{
							MeasureResult res = {op.qubits, sv.single_qubit_measurement(op.qubits[0])};
							measure_results.push_back(res);
							break;
						}
						case MeasureKind::MULTIPLE_QUBIT:
						{
							MeasureResult res = {op.qubits, sv.multiple_qubit_measurement(op.qubits)};
							measure_results.push_back(res);
							break;
						}
						case MeasureKind::ALL_QUBIT:
						{
							MeasureResult res = {{}, sv.full_state_measurement()};
							measure_results.push_back(res);
							break;
						}
					}
				}
			}
		}

		void apply(StateVector& sv) const
		{
			for (const CircuitOperation& op : circuit)
			{	
				if (!op.is_measure)
					sv.gate(kind_to_gate(op.kind), op.qubits, op.params);
				else
					throw std::runtime_error("Trying to apply a measurement, but it was called without space to stock them, pass a std::vector<MeasureResult> if you have measurement gate in the circuit");
			}
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

		QuantumCircuit reverse() const
		{
			QuantumCircuit rqc(circuit);

			std::reverse(rqc.circuit.begin(), rqc.circuit.end());

			return rqc;
		}

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
