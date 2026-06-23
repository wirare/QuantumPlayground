from qiskit import QuantumCircuit
from qiskit.primitives import StatevectorSampler
from qiskit.visualization import plot_histogram
import matplotlib.pyplot as plt

qc = QuantumCircuit(1)
qc.h(0)
qc.measure_all()

sampler = StatevectorSampler()
result = sampler.run([qc], shots=1000).result()

counts = result[0].data.meas.get_counts()

shots = sum(counts.values())
probabilities = {state: count / shots for state, count in counts.items()}

print("Counts:", counts)
print("Probabilities:", probabilities)

plot_histogram(probabilities)
plt.show()
