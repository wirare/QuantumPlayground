#pragma once

#include <cmath>
#include <complex>
#include <initializer_list>
#include <random>
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

		StateVector &gate(const IGate &g, std::span<const size_t> qubits, std::span<const double> angular_params = {})
		{
			if (!g.is_multi_qubit_gate() && qubits.size() != g.qubits_count())
				throw std::invalid_argument(std::string("Gate ") + g.name().data() + std::string(" take ") + std::to_string(g.qubits_count()) + std::string(" qubit(s) in parameter."));

			if (g.is_multi_qubit_gate() && qubits.size() < g.qubits_count())
				throw std::invalid_argument(std::string("Gate ") + g.name().data() + std::string(" take at least ") + std::to_string(g.qubits_count()) + std::string(" qubit(s) in parameter."));

			if (angular_params.size() != g.angular_params_count())
				throw std::invalid_argument(std::string("Gate ") + g.name().data() + std::string(" take ") + std::to_string(g.angular_params_count()) + std::string(" angle(s) in parameter."));

			if (has_duplicate_qubit(qubits))
				throw std::invalid_argument("You can't pass duplicate qubit index in a gate");

			for (size_t q : qubits)
			{
				if (q >= qubit_count)
					throw std::invalid_argument("You can only pass existing qubit in a gate");
			}

			MutableStateVectorView view(amplitudes.data(), qubit_count, amplitudes.size());

			g.apply(view, qubits, angular_params);

			return *this;
		}

		StateVector &gate(const IGate &g, std::initializer_list<size_t> qubits, std::initializer_list<double> angular_params = {})
		{
			return gate(g, std::span<const size_t>(qubits.begin(), qubits.size()), std::span(angular_params.begin(), angular_params.size()));
		}

		StateVector &gate(const IGate &g, std::vector<size_t> &qubits, std::initializer_list<double> angular_params = {})
		{
			return gate(g, std::span<const size_t>(qubits.begin(), qubits.size()), std::span(angular_params.begin(), angular_params.size()));
		}

		StateVector &gate(const IGate &g, std::vector<size_t> &qubits, std::vector<double> angular_params = {})
		{
			return gate(g, std::span<const size_t>(qubits.begin(), qubits.size()), std::span(angular_params.begin(), angular_params.size()));
		}

		size_t full_state_measurement()
		{
			const size_t measured_index = sample_probas(full_state_probabilities());

			for (size_t i = 0; i != amplitudes.size(); i++)
				amplitudes[i] = Complex{0.0, 0.0};

			amplitudes[measured_index] = Complex{1.0, 0.0};

			return measured_index;
		}

		std::vector<double> full_state_probabilities() const
		{
			std::vector<double> probas(amplitudes.size());

			for (size_t i = 0; i != amplitudes.size(); i++)
				probas[i] = std::norm(amplitudes[i]);

			return probas;
		}

		bool single_qubit_measurement(size_t qubit_idx)
		{
			if (qubit_idx >= qubit_count)
				throw std::invalid_argument("Invalid qubit index");

			double proba_zero = 0.0;
			double proba_one = 0.0;

			const size_t mask = size_t{1} << qubit_idx;

			for (size_t i = 0; i != amplitudes.size(); i++)
			{
				if ((i & mask) != 0)
					proba_one += std::norm(amplitudes[i]);
				else
					proba_zero += std::norm(amplitudes[i]);
			}

			std::vector<double> probas = {proba_zero, proba_one};

			const size_t result = sample_probas(probas);
			const bool measured_one = result == 1;

			const double chosen_proba = measured_one ? proba_one : proba_zero;

			const double inv_sqrt_proba = 1.0 / std::sqrt(chosen_proba);

			for (size_t i = 0; i != amplitudes.size(); i++)
			{
				const bool bit_is_one = (i & mask) != 0;

				if (bit_is_one == measured_one)
					amplitudes[i] *= inv_sqrt_proba;
				else
					amplitudes[i] = Complex{0.0, 0.0};
			}

			return measured_one;
		}

		size_t multiple_qubit_measurement(std::span<const size_t> qubits)
		{
			if (qubits.empty())
				throw std::invalid_argument("Cannot measure zero qubits");

			for (size_t q : qubits)
			{
				if (q >= qubit_count)
					throw std::invalid_argument("Invalid qubit index");
			}

			if (has_duplicate_qubit(qubits))
				throw std::invalid_argument("Cannot measure duplicate qubit indices");

			const size_t outcome_count = size_t{1} << qubits.size();

			std::vector<double> probas(outcome_count, 0.0);

			for (size_t i = 0; i != amplitudes.size(); i++)
			{
				const size_t outcome = local_measurement_index(i, qubits);
				probas[outcome] += std::norm(amplitudes[i]);
			}

			const size_t measured_outcome = sample_probas(probas);
			const double chosen_proba = probas[measured_outcome];

			const double inv_sqrt_proba = 1.0 / std::sqrt(chosen_proba);

			for (size_t i = 0; i != amplitudes.size(); i++)
			{
				const size_t outcome = local_measurement_index(i, qubits);

				if (outcome == measured_outcome)
					amplitudes[i] *= inv_sqrt_proba;
				else
					amplitudes[i] = Complex{0.0, 0.0};
			}

			return measured_outcome;
		}

		size_t multiple_qubit_measurement(std::initializer_list<size_t> qubits)
		{
			return multiple_qubit_measurement(
					std::span<const size_t>(qubits.begin(), qubits.size())
					);
		}

		std::vector<size_t> sample_full_state(size_t shots) const
		{
			const std::vector<double> probas = full_state_probabilities();

			std::vector<size_t> counts(amplitudes.size(), 0);

			for (size_t shot = 0; shot != shots; shot++)
			{
				const size_t measured_index = sample_probas(probas);
				counts[measured_index]++;
			}

			return counts;
		}

		std::vector<size_t> sample_single_qubit_counts(size_t qubit_idx, size_t shots) const
		{
			if (qubit_idx >= qubit_count)
				throw std::invalid_argument("Invalid qubit index");

			double proba_zero = 0.0;
			double proba_one = 0.0;

			const size_t mask = size_t{1} << qubit_idx;

			for (size_t i = 0; i != amplitudes.size(); i++)
			{
				if ((i & mask) != 0)
					proba_one += std::norm(amplitudes[i]);
				else
					proba_zero += std::norm(amplitudes[i]);
			}

			std::vector<double> probas = {proba_zero, proba_one};
			std::vector<size_t> counts(2, 0);

			for (size_t shot = 0; shot != shots; ++shot)
			{
				const size_t result = sample_probas(probas);
				counts[result]++;
			}

			return counts;
		}

		std::vector<size_t> sample_multiple_qubit_counts(std::span<const size_t> qubits, size_t shots) const
		{
			if (qubits.empty())
				throw std::invalid_argument("Cannot measure zero qubits");

			for (size_t q : qubits)
			{
				if (q >= qubit_count)
					throw std::invalid_argument("Invalid qubit index");
			}

			if (has_duplicate_qubit(qubits))
				throw std::invalid_argument("Cannot measure duplicate qubit indices");

			const size_t outcome_count = size_t{1} << qubits.size();

			std::vector<double> probas(outcome_count, 0.0);

			for (size_t full_index = 0; full_index != amplitudes.size(); ++full_index)
			{
				const size_t outcome = local_measurement_index(full_index, qubits);
				probas[outcome] += std::norm(amplitudes[full_index]);
			}	

			std::vector<size_t> counts(outcome_count, 0);

			for (size_t shot = 0; shot != shots; ++shot)
			{
				const size_t outcome = sample_probas(probas);
				counts[outcome]++;
			}

			return counts;
		}

		std::vector<size_t> sample_multiple_qubit_counts(std::initializer_list<size_t> qubits, size_t shots) const
		{
			return sample_multiple_qubit_counts(std::span<const size_t>(qubits.begin(), qubits.size()), shots);
		}

		friend std::ostream& operator<<(std::ostream& os, const StateVector& sv);

	private:
		size_t qubit_count;
		std::vector<Complex> amplitudes;

		bool has_duplicate_qubit(std::span<const size_t> qubits) const
		{
			size_t mask = 0;

			for (size_t q : qubits)
			{
				const size_t bit = size_t{1} << q;

				if ((mask & bit) != 0)
					return true;

				mask |= bit;
			}

			return false;
		}

		size_t sample_probas(const std::vector<double> &probas) const
		{
			static std::random_device rd;
			static std::mt19937 gen(rd());

			std::discrete_distribution<size_t> dist(probas.begin(), probas.end());

			return dist(gen);
		}

		size_t local_measurement_index(size_t full_index, std::span<const size_t> qubits) const
		{
			size_t local_outcome = 0;

			for (size_t local_bit = 0; local_bit != qubits.size(); local_bit++)
			{
				const size_t qubit_mask = size_t{1} << qubits[local_bit];

				if ((full_index & qubit_mask) != 0)
					local_outcome |= size_t{1} << local_bit;
			}

			return local_outcome;
		}
};

inline std::ostream &operator<<(std::ostream &os, const StateVector &sv)
{
	os << "StateVector(" << sv.qubit_count << " qubits, "
	   << sv.amplitudes.size() << " amplitudes)\n";

	for (size_t i = 0; i != sv.amplitudes.size(); ++i)
		os << "[" << i << "] = " << sv.amplitudes[i] << "\n";

	return os;
}
