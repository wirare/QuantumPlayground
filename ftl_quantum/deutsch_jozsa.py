from qiskit import QuantumCircuit
from qiskit.transpiler import generate_preset_pass_manager
from qiskit_ibm_runtime import QiskitRuntimeService
from qiskit_ibm_runtime.fake_provider import FakeBelemV2
from qiskit_ibm_runtime import SamplerV2 as Sampler
from qiskit.visualization import plot_histogram
from matplotlib import pyplot as plt


def get_backend(fake=False):
    if fake:
        return FakeBelemV2()
    else:
        service = QiskitRuntimeService()
        return service.least_busy(simulator=False, operational=True)


def deutsch_jozsa(oracle):
    # 4 Qubits, 3 logical Bits
    qc = QuantumCircuit(4, 3)

    # Prepare helper qubits q3 as H|1⟩, so |-⟩
    qc.x(3)
    qc.h(3)

    # Put input qubits q0, q1, q2 into superposition
    qc.h([0, 1, 2])

    # Apply oracle
    qc.compose(oracle, qubits=[0, 1, 2, 3], inplace=True)

    # Interference step
    qc.h([0, 1, 2])

    # Measure the amplitude of the qubits into the logical bits
    qc.measure([0, 1, 2], [0, 1, 2])

    return qc


def constant_oracle():
    oracle = QuantumCircuit(4, name="Constant Oracle")

    # q3 is the helper/output qubit.
    # Flipping q3 means f(x)=1 for every x.
    oracle.x(3)

    return oracle


def balanced_oracle():
    oracle = QuantumCircuit(4, name="Balanced Oracle")

    # q0, q1, q2 are inputs.
    # q3 is helper/output.
    oracle.cx(0, 3)
    oracle.cx(1, 3)
    oracle.cx(2, 3)

    return oracle


oracle = balanced_oracle()
# oracle = constant_oracle()

qc = deutsch_jozsa(oracle)

print("Logical circuit:")
print(qc.draw())

backend = get_backend(fake=True)

pm = generate_preset_pass_manager(backend=backend, optimization_level=1)
isa_circuit = pm.run(qc)

print("Transpiled circuit:", isa_circuit.draw(idle_wires=False))
print("Depth:", isa_circuit.depth())
print("Ops:", isa_circuit.count_ops())

options = {
    "simulator": {
        "seed_simulator": 42
    }
}

sampler = Sampler(mode=backend, options=options)

job = sampler.run([isa_circuit], shots=5000)
result = job.result()

pub_result = result[0]

counts = pub_result.data.c.get_counts()

print(counts)

most_likely = max(counts, key=counts.get)

if most_likely == "000":
    print("Oracle is constant")
else:
    print("Oracle is balanced")

plot_histogram(counts)
plt.show()
