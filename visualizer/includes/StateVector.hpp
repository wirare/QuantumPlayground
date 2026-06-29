#pragma once

#include <complex>
#include <initializer_list>
#include <span>
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

		StateVector &gate(const IGate &g, std::span<const size_t> qubits)
		{
			if (qubits.size() != g.qubits_count())
				throw std::invalid_argument(std::string("Wrong number of qubits for gate ") + g.name().data());

			MutableStateVectorView view(amplitudes.data(), amplitudes.size(), qubit_count);

			g.apply(view, qubits);

			return *this;
		}

		StateVector &gate(const IGate &g, std::initializer_list<size_t> qubits)
		{
			return gate(g, std::span<const size_t>(qubits.begin(), qubits.size()));
		}

		template <typename... Qs>
		StateVector &gate(const IGate &g, Qs... qs)
		{
			static_assert((std::is_integral_v<Qs> && ...), "Qubit indices must be integers");

			std::array<size_t, sizeof...(qs)> qubits{
				static_cast<size_t>(qs)...
			};

			return gate(g, std::span<const size_t>(qubits.data(), qubits.size()));
		}

	private:
		size_t qubit_count;
		std::vector<Complex> amplitudes;
};
