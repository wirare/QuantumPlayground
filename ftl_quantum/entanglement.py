from qiskit import QuantumCircuit
from qiskit.primitives import StatevectorSampler
from qiskit.visualization import plot_histogram
from qiskit_ibm_runtime import QiskitRuntimeService
import matplotlib.pyplot as plt

shots_count = 500

qc = QuantumCircuit(2)
qc.h(0)
qc.cx(0, 1)
qc.measure_all()

sampler = StatevectorSampler()
result = sampler.run([qc], shots=shots_count).result()
counts = result[0].data.meas.get_counts()

probabilities = {state: count / shots_count for state, count in counts.items()}

print("Counts:", counts)
print("Probabilities:", probabilities)

plot_histogram(probabilities)
plt.show()
