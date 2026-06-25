from qiskit import QuantumCircuit
from qiskit.quantum_info import Statevector, SparsePauliOp, Operator
import pyvista as pv
import numpy as np


def initial_state():
    qc = QuantumCircuit(2)

    # qc.x(0)
    qc.h([0, 1])

    return Statevector.from_instruction(qc)


def evolution_step_circuit(
    dt,
    Jx=1.0,
    Jy=1.0,
    Jz=1.0,
    B0=(0.0, 0.0, 0.0),
    B1=(0.0, 0.0, 0.0),
):
    qc = QuantumCircuit(2)

    B0x, B0y, B0z = B0
    B1x, B1y, B1z = B1

    # Spin-spin interaction terms
    qc.rxx(2 * Jx * dt, 0, 1)
    qc.ryy(2 * Jy * dt, 0, 1)
    qc.rzz(2 * Jz * dt, 0, 1)

    # Local field on spin 0
    if B0x != 0.0:
        qc.rx(2 * B0x * dt, 0)
    if B0y != 0.0:
        qc.ry(2 * B0y * dt, 0)
    if B0z != 0.0:
        qc.rz(2 * B0z * dt, 0)

    # Local field on spin 1
    if B1x != 0.0:
        qc.rx(2 * B1x * dt, 0)
    if B1y != 0.0:
        qc.ry(2 * B1y * dt, 0)
    if B1z != 0.0:
        qc.rz(2 * B1z * dt, 0)

    return qc


simulation = {
    "state": initial_state(),
    "time": 0.0,
}

dt = 0.03

U_dt = Operator(
    evolution_step_circuit(
        dt=dt,
        Jx=1.3,
        Jy=1.0,
        Jz=0.7,
        B0=(1.0, -1.0, 0.4),
        B1=(0.4, 0.0, 0.9),
    )
)


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


def bloch_vector_to_array(bloch_vector):
    return np.array(
        [bloch_vector["X"], bloch_vector["Y"], bloch_vector["Z"]],
        dtype=float,
    )


def compute_next_spin_vectors():
    simulation["state"] = simulation["state"].evolve(U_dt)
    simulation["time"] += dt

    spin_0, spin_1 = bloch_vectors(simulation["state"])

    return (
        bloch_vector_to_array(spin_0),
        bloch_vector_to_array(spin_1),
    )


def make_spin_arrow(center, vector):
    vector = np.array(vector, dtype=float)
    length = np.linalg.norm(vector)

    if length < 1e-9:
        direction = np.array([0.0, 0.0, 1.0])
        scale = 0.001
    else:
        direction = vector / length
        scale = length

    return pv.Arrow(
        start=center,
        direction=direction,
        scale=scale,
    )


def vector_length(v):
    return np.linalg.norm(v)


frame = {"i": 0}

plotter = pv.Plotter()

sphere_0 = pv.Sphere(radius=1, center=(-2, 0, 0))
sphere_1 = pv.Sphere(radius=1, center=(2, 0, 0))

plotter.add_mesh(sphere_0, color="blue", opacity=0.25)
plotter.add_mesh(sphere_1, color="blue", opacity=0.25)

arrow_0 = pv.Arrow(start=(-2, 0, 0), direction=(1, 0, 0), scale=1)
arrow_1 = pv.Arrow(start=(2, 0, 0), direction=(1, 0, 0), scale=1)

actor_0 = plotter.add_mesh(arrow_0, color="yellow")
actor_1 = plotter.add_mesh(arrow_1, color="yellow")

plotter.add_axes()


def update(_):
    v0, v1 = compute_next_spin_vectors()

    new_arrow_0 = make_spin_arrow((-2, 0, 0), v0)
    new_arrow_1 = make_spin_arrow((2, 0, 0), v1)

    actor_0.mapper.SetInputData(new_arrow_0)
    actor_0.mapper.Modified()

    actor_1.mapper.SetInputData(new_arrow_1)
    actor_1.mapper.Modified()

    plotter.render()


def update_scene():
    v0, v1 = compute_next_spin_vectors()

    new_arrow_0 = make_spin_arrow((-2, 0, 0), v0)
    new_arrow_1 = make_spin_arrow((2, 0, 0), v1)

    actor_0.mapper.SetInputData(new_arrow_0)
    actor_0.mapper.Modified()

    actor_1.mapper.SetInputData(new_arrow_1)
    actor_1.mapper.Modified()


def set_orbit_camera(frame, n_frames):
    angle = 2 * np.pi * frame / n_frames

    radius = 14.0
    height = 2.5

    plotter.camera_position = [
        (
            radius * np.cos(angle),
            radius * np.sin(angle),
            height,
        ),
        (0, 0, 0),
        (0, 0, 1),
    ]


record = False


if record:
    fps = 30
    duration_seconds = 10
    n_frames = fps * duration_seconds

    plotter.open_movie(
        "two_spin_sim.mp4",
        framerate=fps,
        quality=8,
        format="FFMPEG",
        codec="libx264",
        macro_block_size=None,
    )

    plotter.show(auto_close=False, interactive_update=True)

    for frame in range(n_frames):
        update_scene()
        set_orbit_camera(frame, n_frames)

        plotter.render()
        plotter.write_frame()

    plotter.close()

else:
    plotter.add_timer_event(
        max_steps=10_000,
        duration=30,
        callback=update,
    )
    plotter.show()
