from qiskit import QuantumCircuit
from qiskit.transpiler import generate_preset_pass_manager
from qiskit_ibm_runtime import QiskitRuntimeService
from qiskit_ibm_runtime.fake_provider import FakeBelemV2
from qiskit_ibm_runtime import SamplerV2 as Sampler
from qiskit.visualization import plot_histogram
from matplotlib import pyplot as plt
from math import pi, sqrt, floor


def get_backend(fake=True):
    if fake:
        return FakeBelemV2()
    else:
        service = QiskitRuntimeService()
        return service.least_busy(simulator=False, operational=True)


def build_diffuser(n):
    qc = QuantumCircuit(n, name="Diffuser")

    n_range = range(n)

    qc.h(n_range)
    qc.x(n_range)

    qc.mcp(pi, list(range(n - 1)), n - 1)

    qc.x(n_range)
    qc.h(n_range)

    return qc


def grover_algorithm(oracle, qubits_count):
    qc = QuantumCircuit(qubits_count, qubits_count)

    qubit_range = range(qubits_count)
    qc.h(qubit_range)

    number_of_iteration = max(1, floor((pi/4) * sqrt(2 ** qubits_count)))

    diffuser = build_diffuser(qubits_count)

    for _ in range(number_of_iteration):
        qc.compose(oracle, qubits=qubit_range, inplace=True)
        qc.compose(diffuser, qubits=qubit_range, inplace=True)

    qc.measure(qubit_range, qubit_range)

    return qc


def oracle_11():
    oracle = QuantumCircuit(2, name="Oracle 11")
    oracle.cz(0, 1)
    return oracle


def oracle_101():
    oracle = QuantumCircuit(3, name="Oracle 101")

    # Mark |101>
    # Convert |101> -> |111> by flipping the middle qubit.
    oracle.x(1)

    # Phase-flip |111>
    oracle.mcp(pi, [0, 1], 2)

    # Undo conversion
    oracle.x(1)

    return oracle


oracle = oracle_101()

qc = grover_algorithm(oracle, 3)

print("Logical circuit:")
print(qc.draw())

backend = get_backend(fake=True)

pm = generate_preset_pass_manager(backend=backend, optimization_level=1)
isa_circuit = pm.run(qc)

print("Transpiled circuit:", isa_circuit.draw(idle_wires=False))
print("Depth:", isa_circuit.depth())
print("Ops:", isa_circuit.count_ops())

"""
options = {
    "simulator": {
        "seed_simulator": 42
    }
}
"""

sampler = Sampler(mode=backend)

job = sampler.run([isa_circuit], shots=5000)
result = job.result()

pub_result = result[0]

counts = pub_result.data.c.get_counts()

print(counts)

"""
most_likely = max(counts, key=counts.get)

if most_likely == "000":
    print("Oracle is constant")
else:
    print("Oracle is balanced")
"""
plot_histogram(counts)
plt.show()
