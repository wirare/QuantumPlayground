from qiskit import QuantumCircuit
from qiskit.quantum_info import Statevector, SparsePauliOp
import pyvista as pv
import typing

def initial_state_circuit():
    qc = QuantumCircuit(2)

    # Start from |00>
    # Apply H to both qubits to make |++>
    qc.h([0, 1])

    return qc


def expectation(state, pauli_label):
    observable = SparsePauliOp(pauli_label)
    value = state.expectation_value(observable)
    return value.real


def bloch_vectors(state):
    spin_0 = {
        "X": expectation(state, "IX"),
        "Y": expectation(state, "IY"),
        "Z": expectation(state, "IZ"),
    }

    spin_1 = {
        "X": expectation(state, "XI"),
        "Y": expectation(state, "YI"),
        "Z": expectation(state, "ZI"),
    }

    return spin_0, spin_1


def bloch_vector_to_tuple(bloch_vector: dict):
    return (bloch_vector["X"], bloch_vector["Y"], bloch_vector["Z"])


qc = initial_state_circuit()
state = Statevector.from_instruction(qc)

print("Circuit:")
print(qc.draw())

print("Statevector:")
print(state)

spin_0, spin_1 = bloch_vectors(state)

print("Spin 0 Bloch vector:", spin_0)
print("Spin 1 Bloch vector:", spin_1)

sphere_0 = pv.Sphere(radius=1, center=[-2, 0, 0])
sphere_1 = pv.Sphere(radius=1, center=[2, 0, 0])

spin_0_arrow = pv.Arrow(
        start=(-2, 0, 0),
        direction=bloch_vector_to_tuple(spin_0)
)
spin_1_arrow = pv.Arrow(
        start=(2, 0, 0),
        direction=bloch_vector_to_tuple(spin_1)
)

plotter = pv.Plotter()
plotter.add_mesh(sphere_0, color="blue", opacity=0.25)
plotter.add_mesh(spin_0_arrow, color="yellow")
plotter.add_mesh(sphere_1, color="blue", opacity=0.25)
plotter.add_mesh(spin_1_arrow, color="yellow")
plotter.add_axes()
plotter.show()
