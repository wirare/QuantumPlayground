from math import pi, sqrt, floor, ceil
import numpy as np
import pyvista as pv

from qiskit import QuantumCircuit
from qiskit.quantum_info import Statevector, SparsePauliOp


# ------------------------------------------------------------
# Generic Grover construction
# ------------------------------------------------------------

def apply_multi_control_phase_flip(qc, qubits):
    """
    Apply a phase flip to |111...1> on the given qubits.
    """
    qubits = list(qubits)

    if len(qubits) == 0:
        raise ValueError("Need at least one qubit for a phase flip.")

    if len(qubits) == 1:
        qc.z(qubits[0])
    else:
        controls = qubits[:-1]
        target = qubits[-1]
        qc.mcp(pi, controls, target)


def build_diffuser(n):
    """
    Grover diffuser on n qubits.

    Implements, up to global phase:

        2|s><s| - I

    where |s> is the uniform superposition.
    """
    qc = QuantumCircuit(n, name="Diffuser")
    qrange = list(range(n))

    qc.h(qrange)
    qc.x(qrange)

    apply_multi_control_phase_flip(qc, qrange)

    qc.x(qrange)
    qc.h(qrange)

    return qc


def optimal_grover_iterations(n, marked_count=1):
    """
    Standard Grover iteration estimate.

    For one marked state:
        floor((pi / 4) * sqrt(2**n))
    """
    if marked_count <= 0:
        raise ValueError("marked_count must be >= 1.")

    N = 2 ** n
    iterations = floor((pi / 4) * sqrt(N / marked_count))

    return max(1, iterations)


def build_grover_circuit(
    oracle_qubits,
    oracle_builder,
    marked_count=1,
    iterations=None,
    add_measurements=False,
):
    """
    Build a Grover circuit for any oracle size.

    Parameters
    ----------
    oracle_qubits:
        Number of search qubits.

    oracle_builder:
        Function with signature:

            oracle_builder(n) -> QuantumCircuit

        It must return a phase oracle acting on n qubits.

    marked_count:
        Number of marked states. Used only when iterations is None.

    iterations:
        Optional manual Grover iteration count.

    add_measurements:
        Whether to add final measurements.
    """
    n = oracle_qubits

    oracle = oracle_builder(n)

    if oracle.num_qubits != n:
        raise ValueError(
            f"Oracle has {oracle.num_qubits} qubits, but expected {n}."
        )

    if iterations is None:
        iterations = optimal_grover_iterations(n, marked_count=marked_count)

    if add_measurements:
        qc = QuantumCircuit(n, n)
    else:
        qc = QuantumCircuit(n)

    qrange = list(range(n))

    # Initial uniform superposition
    qc.h(qrange)

    diffuser = build_diffuser(n)

    for _ in range(iterations):
        qc.compose(oracle, qubits=qrange, inplace=True)
        qc.compose(diffuser, qubits=qrange, inplace=True)

    if add_measurements:
        qc.measure(qrange, qrange)

    return qc


# ------------------------------------------------------------
# Generic phase-oracle helpers
# ------------------------------------------------------------

def phase_oracle_for_bitstring(marked_bitstring):
    """
    Build a phase oracle that marks one computational-basis state.

    The bitstring is written in Qiskit's displayed order:

        q[n-1] ... q[1] q[0]

    Example:
        "101" means q2=1, q1=0, q0=1.
    """
    n = len(marked_bitstring)

    if any(bit not in {"0", "1"} for bit in marked_bitstring):
        raise ValueError("marked_bitstring must contain only '0' and '1'.")

    qc = QuantumCircuit(n, name=f"Oracle {marked_bitstring}")

    # Convert displayed bitstring q[n-1]...q0 into qubit indices.
    # If the target bit is 0, use X to turn it into 1 before the phase flip.
    zero_qubits = []

    for qubit_index, bit in enumerate(reversed(marked_bitstring)):
        if bit == "0":
            zero_qubits.append(qubit_index)

    for q in zero_qubits:
        qc.x(q)

    apply_multi_control_phase_flip(qc, range(n))

    for q in reversed(zero_qubits):
        qc.x(q)

    return qc


def make_bitstring_oracle_builder(marked_bitstring):
    """
    Returns an oracle_builder(n) function for a fixed marked bitstring.
    """
    def oracle_builder(n):
        if n != len(marked_bitstring):
            raise ValueError(
                f"Marked bitstring has length {len(marked_bitstring)}, "
                f"but oracle_qubits={n}."
            )

        return phase_oracle_for_bitstring(marked_bitstring)

    return oracle_builder


# Example oracle builders

def oracle_101(n):
    return make_bitstring_oracle_builder("101")(n)


def oracle_11(n):
    return make_bitstring_oracle_builder("11")(n)


def oracle_10101(n):
    return make_bitstring_oracle_builder("10101")(n)


# ------------------------------------------------------------
# State / Bloch utilities
# ------------------------------------------------------------

def expectation(state, pauli_label):
    observable = SparsePauliOp(pauli_label)
    return float(np.real(state.expectation_value(observable)))


def pauli_label_for_qubit(n, qubit_index, axis):
    """
    Qiskit Pauli strings are ordered left-to-right as q[n-1] ... q[0].

    For n=3:
        X on q0 -> "IIX"
        X on q1 -> "IXI"
        X on q2 -> "XII"
    """
    labels = ["I"] * n
    labels[n - 1 - qubit_index] = axis
    return "".join(labels)


def bloch_vector(state, n, qubit_index):
    x = expectation(state, pauli_label_for_qubit(n, qubit_index, "X"))
    y = expectation(state, pauli_label_for_qubit(n, qubit_index, "Y"))
    z = expectation(state, pauli_label_for_qubit(n, qubit_index, "Z"))
    return np.array([x, y, z], dtype=float)


def all_bloch_vectors(state, n):
    return [bloch_vector(state, n, q) for q in range(n)]


def make_spin_arrow(center, vector):
    vector = np.array(vector, dtype=float)
    mag = np.linalg.norm(vector)

    if mag < 1e-9:
        direction = np.array([0.0, 0.0, 1.0])
        scale = 0.001
    else:
        direction = vector / mag
        scale = mag

    return pv.Arrow(
        start=center,
        direction=direction,
        scale=scale,
    )


def gate_description(operation, q_indices):
    qtxt = ", ".join(f"q{q}" for q in q_indices)

    if operation.params:
        params = []

        for p in operation.params:
            try:
                params.append(f"{float(p):.4g}")
            except Exception:
                params.append(str(p))

        ptxt = "[" + ", ".join(params) + "]"
    else:
        ptxt = ""

    return f"{operation.name}{ptxt}({qtxt})"


# ------------------------------------------------------------
# Circuit stepping
# ------------------------------------------------------------

def get_steps(qc):
    """
    Extract quantum operations from a circuit.

    Skips measurements and barriers.
    """
    steps = []

    for item in qc.data:
        operation = item.operation
        qargs = item.qubits

        if operation.name in {"measure", "barrier"}:
            continue

        q_indices = [qc.find_bit(q).index for q in qargs]
        steps.append((operation, q_indices))

    return steps


def replay_until_step(n, steps, step_index):
    """
    Rebuild state from |000...0> through step_index operations.

    step_index = 0 means no gates applied yet.
    step_index = 1 means first gate applied.
    """
    state = Statevector.from_label("0" * n)

    for operation, q_indices in steps[:step_index]:
        step_qc = QuantumCircuit(n)
        step_qc.append(operation.copy(), q_indices)
        state = state.evolve(step_qc)

    return state


# ------------------------------------------------------------
# PyVista visualizer
# ------------------------------------------------------------

class BlochStepVisualizer:
    def __init__(self, qc, max_cols=5):
        self.qc = qc
        self.n = qc.num_qubits
        self.max_cols = max_cols

        self.steps = get_steps(qc)
        self.step_index = 0
        self.state = replay_until_step(self.n, self.steps, self.step_index)

        self.plotter = pv.Plotter(window_size=(1400, 850))
        self.centers = self._make_centers()

        self.arrow_actors = []
        self.text_actor = None
        self.info_actor = None

        self._build_scene()
        self._bind_keys()
        self._update_scene()

    def _make_centers(self):
        """
        Put qubits on a grid so the visualizer still works for larger n.
        """
        spacing = 3.2
        cols = min(self.n, self.max_cols)
        rows = ceil(self.n / cols)

        centers = []

        for q in range(self.n):
            row = q // cols
            col = q % cols

            x = (col - (cols - 1) / 2) * spacing
            z = ((rows - 1) / 2 - row) * spacing

            centers.append((x, 0.0, z))

        return centers

    def _camera_distance(self):
        cols = min(self.n, self.max_cols)
        rows = ceil(self.n / cols)

        return max(8.0, 3.2 * max(cols, rows) * 1.6)

    def _build_scene(self):
        self.plotter.set_background("black")

        for q, center in enumerate(self.centers):
            sphere = pv.Sphere(radius=1.0, center=center)

            self.plotter.add_mesh(
                sphere,
                color="royalblue",
                opacity=0.18,
                smooth_shading=True,
            )

            # Per-sphere reference axes
            self.plotter.add_mesh(
                pv.Arrow(start=center, direction=(1, 0, 0), scale=1.0),
                color="red",
                opacity=0.35,
            )
            self.plotter.add_mesh(
                pv.Arrow(start=center, direction=(0, 1, 0), scale=1.0),
                color="green",
                opacity=0.35,
            )
            self.plotter.add_mesh(
                pv.Arrow(start=center, direction=(0, 0, 1), scale=1.0),
                color="blue",
                opacity=0.35,
            )

            label_point = (center[0], center[1], center[2] + 1.35)

            self.plotter.add_point_labels(
                [label_point],
                [f"q{q}"],
                point_size=0,
                font_size=16,
                text_color="white",
                shape_opacity=0.0,
            )

            arrow = make_spin_arrow(center, (0, 0, 1))
            actor = self.plotter.add_mesh(arrow, color="yellow")
            self.arrow_actors.append(actor)

        self.text_actor = self.plotter.add_text(
            "",
            position=(10, 780),
            font_size=11,
            color="white",
        )

        self.info_actor = self.plotter.add_text(
            "",
            position=(10, 35),
            font_size=9,
            color="white",
        )

        d = self._camera_distance()

        self.plotter.camera_position = [
            (0, -d, 2.5),
            (0, 0, 0),
            (0, 0, 1),
        ]

    def _bind_keys(self):
        self.plotter.add_key_event("space", self.next_step)
        self.plotter.add_key_event("n", self.next_step)
        self.plotter.add_key_event("b", self.previous_step)
        self.plotter.add_key_event("r", self.reset)
        self.plotter.add_key_event("q", self.close)

    def _set_text_actor(self, actor, text):
        if hasattr(actor, "SetInput"):
            actor.SetInput(text)

    def _format_gate(self, index):
        if index < 0 or index >= len(self.steps):
            return "—"

        operation, q_indices = self.steps[index]
        return f"{index + 1:02d}: {gate_description(operation, q_indices)}"

    def _format_vector_info(self, q, vector):
        mag = np.linalg.norm(vector)

        if mag < 1e-9:
            direction = "undefined"
        else:
            unit = vector / mag
            direction = f"({unit[0]:+.3f}, {unit[1]:+.3f}, {unit[2]:+.3f})"

        purity = 0.5 * (1.0 + mag * mag)

        return (
            f"q{q}: "
            f"mag={mag:.3f}  "
            f"dir={direction}  "
            f"purity≈{purity:.3f}"
        )

    def _top_probabilities(self, max_items=8):
        probs = self.state.probabilities_dict()

        items = sorted(
            probs.items(),
            key=lambda item: item[1],
            reverse=True,
        )

        return items[:max_items]

    def _update_scene(self):
        vectors = all_bloch_vectors(self.state, self.n)

        for q, vector in enumerate(vectors):
            new_arrow = make_spin_arrow(self.centers[q], vector)

            self.arrow_actors[q].mapper.SetInputData(new_arrow)
            self.arrow_actors[q].mapper.Modified()

        last_gate = self._format_gate(self.step_index - 2)
        current_gate = self._format_gate(self.step_index - 1)
        next_gate = self._format_gate(self.step_index)

        header_lines = [
            "Generic Grover Bloch visualizer",
            "",
            f"Qubits: {self.n}",
            f"Step: {self.step_index} / {len(self.steps)}",
            "",
            "Keys:",
            "SPACE / n : next gate",
            "b         : previous gate",
            "r         : reset",
            "q         : close",
        ]

        self._set_text_actor(
            self.text_actor,
            "\n".join(header_lines),
        )

        magnitudes = [np.linalg.norm(v) for v in vectors]
        avg_mag = float(np.mean(magnitudes))

        info_lines = [
            "Quantum state info",
            "------------------",
            f"Last gate:    {last_gate}",
            f"Current gate: {current_gate}",
            f"Next gate:    {next_gate}",
            "",
            "Local Bloch vectors:",
        ]

        for q, vector in enumerate(vectors):
            info_lines.append(self._format_vector_info(q, vector))

        info_lines.extend(
            [
                "",
                f"Average local mag: {avg_mag:.3f}",
                "Note: mag < 1 means local information is partly in correlations.",
                "",
                "Top computational-basis probabilities:",
            ]
        )

        for bitstring, prob in self._top_probabilities(max_items=8):
            info_lines.append(f"|{bitstring}> : {prob:.4f}")

        self._set_text_actor(
            self.info_actor,
            "\n".join(info_lines),
        )

        self.plotter.render()

    def next_step(self):
        if self.step_index >= len(self.steps):
            return

        operation, q_indices = self.steps[self.step_index]

        step_qc = QuantumCircuit(self.n)
        step_qc.append(operation.copy(), q_indices)

        self.state = self.state.evolve(step_qc)
        self.step_index += 1

        self._update_scene()

    def previous_step(self):
        if self.step_index <= 0:
            return

        self.step_index -= 1
        self.state = replay_until_step(self.n, self.steps, self.step_index)

        self._update_scene()

    def reset(self):
        self.step_index = 0
        self.state = replay_until_step(self.n, self.steps, self.step_index)
        self._update_scene()

    def close(self):
        self.plotter.close()

    def show(self):
        self.plotter.show()


# ------------------------------------------------------------
# Main runner
# ------------------------------------------------------------

def run_visualizer(
    oracle_qubits,
    oracle_builder,
    marked_count=1,
    iterations=None,
):
    qc = build_grover_circuit(
        oracle_qubits=oracle_qubits,
        oracle_builder=oracle_builder,
        marked_count=marked_count,
        iterations=iterations,
        add_measurements=False,
    )

    print("Logical circuit:")
    print(qc.draw())

    print("\nOperations visualized:")
    for i, (operation, q_indices) in enumerate(get_steps(qc), start=1):
        print(f"{i:02d}: {gate_description(operation, q_indices)}")

    visualizer = BlochStepVisualizer(qc)
    visualizer.show()

    return qc


# ------------------------------------------------------------
# Choose your oracle here
# ------------------------------------------------------------

if __name__ == "__main__":
    run_visualizer(
        oracle_qubits=5,
        oracle_builder=make_bitstring_oracle_builder("11001"),
    )

    # Other examples:
    #
    # run_visualizer(
    #     oracle_qubits=2,
    #     oracle_builder=oracle_11,
    # )
    #
    # run_visualizer(
    #     oracle_qubits=5,
    #     oracle_builder=oracle_10101,
    # )
    #
    # run_visualizer(
    #     oracle_qubits=4,
    #     oracle_builder=make_bitstring_oracle_builder("1101"),
    # )
