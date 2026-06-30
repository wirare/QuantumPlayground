#pragma once

#include <complex>
#include <initializer_list>
#include <span>
#include <string>
#include <vector>
#include <IGate.hpp>

using Complex = std::complex<double>;

class MutableStateVectorView
{
	public:
		MutableStateVectorView(Complex *data, size_t qubit_count, size_t amplitude_count)
			: m_data(data),
			  m_qubit_count(qubit_count),
			  m_amplitude_count(amplitude_count)
		{}

		size_t qubit_count() const { return m_qubit_count; }
		size_t amplitude_count() const { return m_amplitude_count; }

		std::complex<double>& amplitude(size_t i) { return m_data[i]; }
		const std::complex<double>& amplitude(size_t i) const { return m_data[i]; }

	private:
		Complex *m_data;
		size_t m_qubit_count;
		size_t m_amplitude_count;
};

class StateVector
{
	public:
		StateVector(size_t qubit_count): qubit_count(qubit_count), amplitudes(size_t{1} << qubit_count, Complex{0.0, 0.0})
		{
			amplitudes[0] = Complex{1.0, 0.0}; // Base state |000...0⟩
		}

		StateVector(const StateVector& other): qubit_count(other.qubit_count), amplitudes(other.amplitudes) {}

		StateVector &operator=(const StateVector &other)
		{
			qubit_count = other.qubit_count;
			amplitudes = other.amplitudes;
			return *this;
		}

		StateVector &gate(const IGate &g, std::span<const size_t> qubits, std::span<const double> angular_params)
		{
			if (qubits.size() != g.qubits_count())
				throw std::invalid_argument(std::string("Gate ") + g.name().data() + std::string(" take ") + std::to_string(g.qubits_count()) + std::string(" qubit(s) in parameter."));
			if (angular_params.size() != g.angular_params_count())
				throw std::invalid_argument(std::string("Gate ") + g.name().data() + std::string(" take ") + std::to_string(g.angular_params_count()) + std::string(" angle(s) in parameter."));

			MutableStateVectorView view(amplitudes.data(), amplitudes.size(), qubit_count);

			g.apply(view, qubits, angular_params);

			return *this;
		}

		StateVector &gate(const IGate &g, std::initializer_list<size_t> qubits, std::initializer_list<double> angular_params)
		{
			return gate(g, std::span<const size_t>(qubits.begin(), qubits.size()), std::span(angular_params.begin(), angular_params.size()));
		}

	private:
		size_t qubit_count;
		std::vector<Complex> amplitudes;
};
