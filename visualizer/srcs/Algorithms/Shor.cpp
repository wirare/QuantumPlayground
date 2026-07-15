#include <Algorithms/Shor.hpp>
#include <QuantumCircuit.hpp>
#include <numeric>
#include <StateVector.hpp>
#include <utility>
#include <random>

bool is_prime(uint64_t n)
{
	if (n <= 1)
		return false;

	if (n == 2 || n == 3)
		return true;

	if (n % 2 == 0 || n % 3 == 0)
		return false;

	for (uint64_t i = 5; i * i <= n; i += 6)
	{
		if (n % i == 0 || n % (i + 2) == 0)
			return false;
	}

	return true;
}

QuantumCircuit initial_quantum_state_circuit(size_t bits_number)
{
	QuantumCircuit qc;

	qc.add_gate(GateKind::X, {0});

	for (size_t i = 2 * bits_number + 2; i < 4 * bits_number + 2; i++)
		qc.add_gate(GateKind::H, {i});

	return qc;
}

QuantumCircuit CMODADD_qc(size_t bits_number)
{

}

QuantumCircuit CMULT_qc(size_t bits_number)
{
	QuantumCircuit qc;

}

std::pair<uint64_t, uint64_t> shor_factorisation(uint64_t N, uint64_t choosed_a = 0)
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<uint64_t> dist(2, N - 2);

	if (N % 2 == 0)
		return {2, N / 2};

	if (is_prime(N))
		return {0, 0};

	const size_t n = std::bit_width(N);
	const size_t total_qubits = 4 * n + 2;

	QuantumCircuit initial_state_qc = initial_quantum_state_circuit(n);
	StateVector state(total_qubits);

	while (true)
	{
		state.reset();

		uint64_t a;

		if (choosed_a != 0)
			a = choosed_a;
		else
			a = dist(gen);

		uint64_t divisor = std::gcd(a, N);

		if (divisor > 1)
			return {divisor, N / divisor};

		initial_state_qc.apply(state);

		uint64_t c = a % N;
		std::vector<uint64_t> constants(2 * n + 1);

		for (size_t i = 0; i != 2 * n + 1; i++)
		{
			constants[i] = c;
			c = (c * c) % N;
		}
	}
}
