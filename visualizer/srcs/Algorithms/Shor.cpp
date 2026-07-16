#include <Algorithms/Shor.hpp>
#include <QuantumCircuit.hpp>
#include <bit>
#include <numeric>
#include <StateVector.hpp>
#include <stdexcept>
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

inline QuantumCircuit get_controlled_x(std::vector<size_t> qubits)
{
	QuantumCircuit qc;

	switch (qubits.size())
	{
		case 0:
			throw std::logic_error("A target qubit is required");

		case 1:
			qc.add_gate(GateKind::X, std::move(qubits), {});
			break;

		case 2:
			qc.add_gate(GateKind::CX, std::move(qubits), {});
			break;

		case 3:
			qc.add_gate(GateKind::CCX, std::move(qubits), {});
			break;

		default:
			qc.add_gate(GateKind::MCX, std::move(qubits), {});
			break;
	}

	return qc;
}

QuantumCircuit CAddPowerOfTwo(const std::vector<size_t>& qubit_register, const std::vector<size_t>& control_qubits, size_t power_of_two)
{
#ifndef NO_GATE_REGISTER_CHECK
    if (qubit_register.empty())
        throw std::invalid_argument("The qubit register cannot be empty");

    if (power_of_two >= qubit_register.size())
        throw std::invalid_argument("The power-of-two exponent must be smaller than the register size");

    for (size_t control : control_qubits)
    {
        if (std::find(qubit_register.begin(), qubit_register.end(), control) != qubit_register.end())
            throw std::invalid_argument("A control qubit cannot belong to the target register");
    }

    for (size_t i = 0; i < control_qubits.size(); ++i)
    {
        if (std::find(control_qubits.begin() + i + 1, control_qubits.end(), control_qubits[i]) != control_qubits.end())
            throw std::invalid_argument("The control-qubit list contains duplicates");
    }
#endif

    QuantumCircuit qc;

    for (size_t target = qubit_register.size() - 1; target > power_of_two; --target)
    {
        std::vector<size_t> gate_qubits;

        const size_t propagation_control_count = target - power_of_two;

        gate_qubits.reserve(control_qubits.size() + propagation_control_count + 1);
        gate_qubits.insert(gate_qubits.end(), control_qubits.begin(), control_qubits.end());

        for (size_t bit = power_of_two; bit < target; ++bit)
            gate_qubits.push_back(qubit_register[bit]);

        gate_qubits.push_back(qubit_register[target]);

        qc += get_controlled_x(std::move(gate_qubits));
    }

    std::vector<size_t> final_gate_qubits;

    final_gate_qubits.reserve(control_qubits.size() + 1);
    final_gate_qubits.insert(final_gate_qubits.end(), control_qubits.begin(), control_qubits.end());
    final_gate_qubits.push_back(qubit_register[power_of_two]);

    qc += get_controlled_x(std::move(final_gate_qubits));

    return qc;
}

QuantumCircuit CAddConst(const std::vector<size_t> &qubit_register, const std::vector<size_t> &control_qubits, uint64_t constant)
{
#ifndef NO_GATE_REGISTER_CHECK
    if (qubit_register.empty())
        throw std::invalid_argument("The qubit register cannot be empty");

    for (size_t control : control_qubits)
    {
        if (std::find(qubit_register.begin(), qubit_register.end(), control) != qubit_register.end())
            throw std::invalid_argument("A control qubit cannot belong to the target register");
    }

    for (size_t i = 0; i < control_qubits.size(); ++i)
    {
        if (std::find(control_qubits.begin() + i + 1, control_qubits.end(), control_qubits[i]) != control_qubits.end())
            throw std::invalid_argument("The control-qubit list contains duplicates");
    }

	if ((size_t)std::bit_width(constant) > qubit_register.size())
        throw std::invalid_argument("The constant does not fit in the target register");
#endif

	QuantumCircuit qc;

	for (size_t i = 0; (int)i != std::bit_width(constant); i++)
	{
		if ((constant >> i) & 1)
			qc += CAddPowerOfTwo(qubit_register, control_qubits, i);
	}

	return qc;
}

inline QuantumCircuit CSubConst(const std::vector<size_t> &qubit_register, const std::vector<size_t> &control_qubits, uint64_t constant)
{
	return CAddConst(qubit_register, control_qubits, constant).reverse();
}

QuantumCircuit CModAddConst(const std::vector<size_t> &qubit_register, size_t flag, const std::vector<size_t> &control_qubits, uint64_t constant, uint64_t modular_constant)
{
#ifndef NO_GATE_REGISTER_CHECK
    if (qubit_register.size() < 2)
        throw std::invalid_argument("The target register must contain at least one value qubit and one carry qubit");

    if (modular_constant == 0)
        throw std::invalid_argument("The modular constant cannot be zero");

    if (constant >= modular_constant)
        throw std::invalid_argument("The added constant must be smaller than the modular constant");

    const size_t value_register_size = qubit_register.size() - 1;

    if ((size_t)std::bit_width(modular_constant) > value_register_size)
        throw std::invalid_argument("The modular constant does not fit in the value part of the target register");

    if ((size_t)std::bit_width(constant) > value_register_size)
        throw std::invalid_argument("The constant does not fit in the value part of the target register");

    if (std::find(qubit_register.begin(), qubit_register.end(), flag) != qubit_register.end())
        throw std::invalid_argument("The flag qubit cannot belong to the target register");

    for (size_t i = 0; i < qubit_register.size(); ++i)
    {
        if (std::find(qubit_register.begin() + i + 1, qubit_register.end(), qubit_register[i]) != qubit_register.end())
            throw std::invalid_argument("The target register contains duplicate qubits");
    }

    for (size_t control : control_qubits)
    {
        if (std::find(qubit_register.begin(), qubit_register.end(), control) != qubit_register.end())
            throw std::invalid_argument("A control qubit cannot belong to the target register");

        if (control == flag)
            throw std::invalid_argument("The flag qubit cannot also be a control qubit");
    }

    for (size_t i = 0; i < control_qubits.size(); ++i)
    {
        if (std::find(control_qubits.begin() + i + 1, control_qubits.end(), control_qubits[i]) != control_qubits.end())
            throw std::invalid_argument("The control-qubit list contains duplicates");
    }
#endif

	QuantumCircuit qc;

	const size_t carry = qubit_register.back();

	qc += CAddConst(qubit_register, control_qubits, constant);
	qc += CSubConst(qubit_register, control_qubits, modular_constant);
	
	qc.add_gate(GateKind::CX, {carry, flag});
	
	qc += CAddConst(qubit_register, {flag}, modular_constant);
	qc += CSubConst(qubit_register, control_qubits, constant);
	
	qc.add_gate(GateKind::X, {carry});
	
	std::vector<size_t> x_qubits;
	x_qubits.reserve(control_qubits.size() + 2);

	x_qubits.insert(x_qubits.end(), control_qubits.begin(), control_qubits.end());
	x_qubits.push_back(carry);
	x_qubits.push_back(flag);

	qc += get_controlled_x(x_qubits);

	qc.add_gate(GateKind::X, {carry});

	qc += CAddConst(qubit_register, control_qubits, constant);

	return qc;
}

QuantumCircuit CModMulAddConst(const std::vector<size_t> &Y_register, 
							   const std::vector<size_t> &qubit_register, 
							   size_t flag, 
							   const std::vector<size_t> &control_qubits, 
							   uint64_t mul_constant, 
							   uint64_t modular_constant)
{
	for (size_t i = 0; (int)i != std::bit_width())
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
