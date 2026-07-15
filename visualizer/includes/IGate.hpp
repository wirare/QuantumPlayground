#pragma once

#include "Matrix.hpp"
#include <complex>
#include <memory>
#include <string_view>
#include <span>

class MutableStateVectorView;

class IGate 
{
	public:
		virtual void apply([[maybe_unused]] MutableStateVectorView &mutableStateVectorView, 
						   [[maybe_unused]] std::span<const size_t> qubits,
						   [[maybe_unused]] std::span<const double> angular_params = {}) const = 0;
		virtual size_t qubits_count() const = 0;
		virtual size_t angular_params_count() const = 0;
		virtual std::string_view name() const = 0;
		virtual bool is_multi_qubit_gate() const = 0;
		virtual Matrix<std::complex<double>> matrix([[maybe_unused]] std::span<const double> angular_params) const
		{
			throw std::logic_error("This gate does not expose a matrix");
		};
		virtual ~IGate() = default;
};

enum class GateKind
{
	I,
	X,
	Y,
	Z,
	H,
	S,
	Sdg,
	T,
	Tdg,
	P,
	RZ,
	RX,
	RY,
	CX,
	CZ,
	CP,
	CRZ,
	CRX,
	CRY,
	SWAP,
	ISWAP,
	RZZ,
	CCX,
	CCZ,
	CSWAP,
	MCX,
	MCZ,
	MCP,
	RXX,
	RYY,
	RZX,
	ECR,
	U,
	SX,
	CH,
	SQRTSWAP,
	CNOT,
};
