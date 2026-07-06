#include <complex>
#include <format>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <algorithm>
#include <array>
#include <vtkAutoInit.h>
#include <vtkBillboardTextActor3D.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkTransform.h>
#include <vtkCallbackCommand.h>

VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);

#include <QuantumCircuit.hpp>

using Coordinate = std::array<double, 3>;

inline double norm_squared(const Coordinate &v)
{
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

inline double length(const Coordinate &v)
{
	return std::sqrt(norm_squared(v));
}

#define UNPACK_COORD(c) c[0], c[1], c[2]

static void SetArrowPos(vtkActor* actor, const Coordinate& start, const Coordinate& direction)
{
	double xAxis[3] = {UNPACK_COORD(direction)};

	const double length = vtkMath::Norm(xAxis);

	if (length < 1e-12)
	{
		actor->SetVisibility(false);
		return;
	}

	actor->SetVisibility(true);

	vtkMath::Normalize(xAxis);

	double arbitrary[3] = {0.0, 0.0, 1.0};

	if (std::abs(vtkMath::Dot(xAxis, arbitrary)) > 0.99)
	{
		arbitrary[0] = 0.0;
		arbitrary[1] = 1.0;
		arbitrary[2] = 0.0;
	}

	double zAxis[3];
	vtkMath::Cross(xAxis, arbitrary, zAxis);
	vtkMath::Normalize(zAxis);

	double yAxis[3];
	vtkMath::Cross(zAxis, xAxis, yAxis);
	vtkMath::Normalize(yAxis);

	vtkNew<vtkMatrix4x4> matrix;
	matrix->Identity();

	matrix->SetElement(0, 0, xAxis[0]);
	matrix->SetElement(1, 0, xAxis[1]);
	matrix->SetElement(2, 0, xAxis[2]);

	matrix->SetElement(0, 1, yAxis[0]);
	matrix->SetElement(1, 1, yAxis[1]);
	matrix->SetElement(2, 1, yAxis[2]);

	matrix->SetElement(0, 2, zAxis[0]);
	matrix->SetElement(1, 2, zAxis[1]);
	matrix->SetElement(2, 2, zAxis[2]);

	vtkNew<vtkTransform> transform;
	transform->Translate(start[0], start[1], start[2]);
	transform->Concatenate(matrix);
	transform->Scale(length, length, length);

	actor->SetUserTransform(transform);
}

namespace ActorFactory
{
	vtkSmartPointer<vtkActor> MakeSphereActor(double radius = 1.0, const Coordinate &center = {0, 0, 0})
	{
		vtkNew<vtkSphereSource> sphere;
		sphere->SetCenter(UNPACK_COORD(center));
		sphere->SetRadius(radius);
		sphere->SetThetaResolution(48);
		sphere->SetPhiResolution(48);

		vtkNew<vtkPolyDataMapper> mapper;
		mapper->SetInputConnection(sphere->GetOutputPort());

		vtkNew<vtkActor> actor;
		actor->SetMapper(mapper);

		return actor;
	}

	std::pair<vtkSmartPointer<vtkActor>, vtkSmartPointer<vtkPolyDataMapper>> MakeArrowActor(const Coordinate &pos = {0, 0, 0}, const Coordinate &orientation = {0, 0, 0}, double scale = 1)
	{
		vtkNew<vtkArrowSource> arrow;
		arrow->SetTipResolution(32);
		arrow->SetShaftResolution(32);
		arrow->SetTipLength(0.25);
		arrow->SetTipRadius(0.08);
		arrow->SetShaftRadius(0.03);

		vtkNew<vtkPolyDataMapper> mapper;
		mapper->SetInputConnection(arrow->GetOutputPort());

		vtkNew<vtkActor> actor;
		actor->SetMapper(mapper);

		actor->SetPosition(UNPACK_COORD(pos));
		actor->SetScale(scale, scale, scale);
		actor->SetOrientation(UNPACK_COORD(orientation));

		return {actor, mapper};
	}

	std::pair<vtkSmartPointer<vtkActor>, vtkSmartPointer<vtkPolyDataMapper>> MakeSpinArrowActor(const Coordinate &center, const Coordinate &dir = {0, 1, 0})
	{
		auto [actor, mapper] = MakeArrowActor({0, 0, 0});
		actor->GetProperty()->SetColor(1, 1, 0);

		SetArrowPos(actor, center, dir);
		return {actor, mapper};
	}

	vtkSmartPointer<vtkBillboardTextActor3D> Make3DLabelActor(const Coordinate &pos, const std::string &text = "")
	{
		vtkNew<vtkBillboardTextActor3D> label;

		label->SetInput(text.c_str());
		label->SetPosition(UNPACK_COORD(pos));

		label->GetTextProperty()->SetFontSize(16);
		label->GetTextProperty()->SetColor(1, 1, 1);
		label->GetTextProperty()->SetJustificationToCentered();
		label->GetTextProperty()->SetVerticalJustificationToCentered();

		return label;
	}

	vtkSmartPointer<vtkTextActor> MakeTextActor(int x, int y, int font_size = 11)
	{
		vtkNew<vtkTextActor> text;

		text->SetInput("");
		text->SetDisplayPosition(x, y);

		text->GetTextProperty()->SetFontSize(font_size);
		text->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
		text->GetTextProperty()->SetFontFamilyToCourier();

		return text;
	}
}

#define KEY_TO_ACTION(k, a, ...) __VA_OPT__(else) if (key == k) { a; }

class BlochStepVisualizer
{
	public:

		BlochStepVisualizer(size_t qubit_number, const QuantumCircuit &qc, size_t max_cols = 5): qubit_number(qubit_number), max_cols(max_cols), state(StateVector(qubit_number))
		{
			steps = qc.get_raw_circuit();

			renderer = vtkSmartPointer<vtkRenderer>::New();
			renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
			interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();

			renderWindow->AddRenderer(renderer);
			interactor->SetRenderWindow(renderWindow);

			make_centers();
			build_scene();
			bind_key();
			update_scene();

			interactor->Initialize();
		}

		void show()
		{
			renderWindow->Render();
			interactor->Start();
		}

	private:
		size_t qubit_number = 0;
		size_t max_cols = 5;

		std::vector<CircuitOperation> steps; 
		size_t step_index = 0;
		StateVector state;

		vtkSmartPointer<vtkRenderer> renderer;
		vtkSmartPointer<vtkRenderWindow> renderWindow;
		vtkSmartPointer<vtkRenderWindowInteractor> interactor;

		std::vector<Coordinate> centers;

		std::vector<vtkSmartPointer<vtkActor>> qubit_arrow_actors;
		std::vector<vtkSmartPointer<vtkPolyDataMapper>> qubit_arrow_mappers;
		std::vector<vtkSmartPointer<vtkActor>> bloch_sphere_axis_arrows;
		std::vector<vtkSmartPointer<vtkActor>> bloch_sphere_actors;
		std::vector<vtkSmartPointer<vtkBillboardTextActor3D>> label_actors;

		vtkSmartPointer<vtkTextActor> text_actor;
		vtkSmartPointer<vtkTextActor> info_actor;

		vtkSmartPointer<vtkCallbackCommand> key_callback;

		static void OnKeyPress(vtkObject *caller, unsigned long, void *client_data, void *)
		{
			auto *self = static_cast<BlochStepVisualizer*>(client_data);

			auto *interactor = vtkRenderWindowInteractor::SafeDownCast(caller);
			if (self == nullptr || interactor == nullptr)
				return;

			const std::string key = interactor->GetKeySym();

			KEY_TO_ACTION("space", self->next_step())
			KEY_TO_ACTION("n", self->next_step(), 1)
			KEY_TO_ACTION("b", self->previous_step(), 1)
			KEY_TO_ACTION("r", self->reset(), 1)
			KEY_TO_ACTION("q", self->close(), 1)
			KEY_TO_ACTION("Escape", self->close(), 1)
		}

		void bind_key()
		{
			key_callback = vtkSmartPointer<vtkCallbackCommand>::New();

			key_callback->SetClientData(this);
			key_callback->SetCallback(&OnKeyPress);

			interactor->AddObserver(vtkCommand::KeyPressEvent, key_callback);
		}

		void make_centers()
		{
			constexpr double spacing = 3.2;
			const size_t cols = std::min(qubit_number, max_cols);
			const size_t rows = std::ceil(qubit_number / cols);

			for (size_t i = 0; i != qubit_number; i++)
			{
				const size_t row = static_cast<size_t>(i / cols);
				const size_t col = i % cols;

				const double x = (col - (cols - 1) / 2.0) * spacing;
				const double z = ((rows - 1) / 2.0 - row) * spacing;

				centers.push_back({x, 0.0, z});
			}

		}

		double camera_distance()
		{
			const size_t cols = std::min(qubit_number, max_cols);
			const size_t rows = std::ceil(qubit_number / cols);

			return std::max(8.0, 3.2 * std::max(cols, rows) * 1.6);
		}

		void build_scene()
		{
			renderer->SetBackground(0, 0, 0);

			for (size_t i = 0; i != centers.size(); i++)
			{
				Coordinate center = centers[i];
				
				auto sphere = ActorFactory::MakeSphereActor(1.0, center);
				sphere->GetProperty()->SetOpacity(0.18);
				sphere->GetProperty()->SetColor(0.1, 0.3, 1.0);

				bloch_sphere_actors.push_back(sphere);
				renderer->AddActor(sphere);

				auto x_arrow = ActorFactory::MakeArrowActor(center).first;
				x_arrow->GetProperty()->SetColor(1, 0, 0);
				x_arrow->GetProperty()->SetOpacity(0.35);

				auto y_arrow = ActorFactory::MakeArrowActor(center, {0, 0, 90}).first;
				y_arrow->GetProperty()->SetColor(0, 1, 0);
				y_arrow->GetProperty()->SetOpacity(0.35);

				auto z_arrow = ActorFactory::MakeArrowActor(center, {0, -90, 0}).first;
				z_arrow->GetProperty()->SetColor(0, 0, 1);
				z_arrow->GetProperty()->SetOpacity(0.35);

				bloch_sphere_axis_arrows.push_back(x_arrow);
				bloch_sphere_axis_arrows.push_back(y_arrow);
				bloch_sphere_axis_arrows.push_back(z_arrow);
				renderer->AddActor(x_arrow);
				renderer->AddActor(y_arrow);
				renderer->AddActor(z_arrow);

				Coordinate label_point = {UNPACK_COORD(center) + 1.35};
				auto label = ActorFactory::Make3DLabelActor(label_point, "q" + std::to_string(i));

				label_actors.push_back(label);
				renderer->AddActor(label);

				auto [arrow_actor, arrow_mapper] = ActorFactory::MakeSpinArrowActor(center);

				qubit_arrow_actors.push_back(arrow_actor);
				qubit_arrow_mappers.push_back(arrow_mapper);
				renderer->AddActor(arrow_actor);
			}

			text_actor = ActorFactory::MakeTextActor(10, 780, 11);
			renderer->AddActor(text_actor);

			info_actor = ActorFactory::MakeTextActor(10, 35, 9);
			renderer->AddActor(info_actor);
		}

		std::string format_gate(size_t index)
		{
			if (index < 0 || index >= steps.size())
				return "—";

			std::stringstream gate_desc;
			gate_desc << steps[index];
			return std::format("{:02}: {}", index + 1, gate_desc.str());
		}

		std::string format_vector_info(size_t qubit, const Coordinate &vec)
		{
			double mag = length(vec);
			double inv_mag = 1 / mag;
			std::string direction;

			if (mag < 1e-9)
				direction = "undefined";
			else
				direction = std::format("{:+.3f}, {:+.3f}, {:+.3f}", vec[0] * inv_mag, vec[1] * inv_mag, vec[2] * inv_mag);

			double purity = 0.5 * (1 + mag * mag);

			return std::format("q{}: mag={:.3f}  dir={}  purity≈{:.3f}", qubit, mag, direction, purity);
		}

		std::string basis_label(size_t index, size_t qubit_count) const
		{
			std::string out;
			out.reserve(qubit_count + 2);

			out += '|';

			for (size_t q = qubit_count; q-- > 0;)
			{
				const bool bit = (index & (size_t{1} << q)) != 0;
				out += bit ? '1' : '0';
			}

			out += '>';

			return out;
		}

		std::vector<std::pair<std::string, double>> top_probabilities(size_t max_items = 8) const
		{
			const std::vector<double> probs = state.full_state_probabilities();

			std::vector<std::pair<std::string, double>> items;
			items.reserve(probs.size());

			for (size_t i = 0; i != probs.size(); i++)
				items.emplace_back(basis_label(i, state.get_qubit_count()), probs[i]);

			std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

			if (items.size() > max_items)
				items.resize(max_items);

			return items;
		}

		Coordinate bloch_vector(size_t qubit_index)
		{
			Coordinate vector;
			vector[0] = state.pauli_expectation(Pauli::X, qubit_index);
			vector[1] = state.pauli_expectation(Pauli::Y, qubit_index);
			vector[2] = state.pauli_expectation(Pauli::Z, qubit_index);
			return vector;
		}

		std::vector<Coordinate> all_bloch_vectors()
		{
			std::vector<Coordinate> vectors(qubit_number);

			for (size_t i = 0; i != qubit_number; i++)
				vectors[i] = bloch_vector(i);

			return vectors;
		}

		void update_scene()
		{
			std::vector<Coordinate> vectors = all_bloch_vectors();

			std::string last_gate = format_gate(step_index - 2);
			std::string current_gate = format_gate(step_index - 1);
			std::string next_gate = format_gate(step_index);

			std::stringstream header_lines;
			header_lines << "Generic Bloch Sphere visualizer\n\n"
						<< "Qubits: " << qubit_number << "\n"
						<< "Step: " << step_index << "/" << steps.size() << "\n\n"
						<< "Keys:\n"
						<< "SPACE / n : next gate\n"
						<< "b         : previous gate\n"
						<< "r         : reset\n"
						<< "q         : close\n";

			text_actor->SetInput(header_lines.str().c_str());

			std::vector<double> magnitudes;
			for (Coordinate &vec : vectors)
				magnitudes.push_back(length(vec));
			
			double avg_mag = std::accumulate(magnitudes.begin(), magnitudes.end(), 0) / static_cast<double>(magnitudes.size());

			std::stringstream info_lines;
			info_lines	<< "Quantum state info\n"
						<< "------------------\n"
						<< "Last gate:    " << last_gate << "\n"
						<< "Current gate: " << current_gate << "\n"
						<< "Next gate:    " << next_gate << "\n\n"
						<< "Local Bloch vectors:";

			for (size_t i = 0; i != vectors.size(); i++)
			{
				SetArrowPos(qubit_arrow_actors[i], centers[i], vectors[i]);

				info_lines << format_vector_info(i, vectors[i]) << "\n";
			}

			info_lines	<< std::format("\nAverage local mag {:.3f}\nNote: mag < 1 means local information is partly in correlations.\n\nTop computational-basis probabilities:\n", avg_mag);

			for (auto [str, prob] : top_probabilities())
				info_lines << std::format("{} : {:.4f}\n", str, prob);

			info_actor->SetInput(info_lines.str().c_str());
		}

		void return_to_previous_state()
		{
			StateVector localstate(qubit_number);
			QuantumCircuit qc;

			for (size_t i = 0; i != step_index - 1; i++)
				qc.add_gate(steps[i]);

			qc.apply(localstate);

			state = localstate;
		}

		void next_step()
		{
			if (step_index >= steps.size())
				return;

			CircuitOperation op = steps[step_index];

			QuantumCircuit step_qc;
			step_qc.add_gate(op);
			step_qc.apply(state);

			step_index++;
			update_scene();
			renderWindow->Render();
		}

		void previous_step()
		{
			if (step_index <= 0)
				return;

			return_to_previous_state();
			
			step_index--;
			update_scene();
			renderWindow->Render();
		}

		void reset()
		{
			step_index = 0;
			state.reset();
			update_scene();
			renderWindow->Render();
		}

		void close()
		{
			if (renderWindow)
				renderWindow->Finalize();

			if (interactor)
				interactor->TerminateApp();
		}
};

void apply_multi_control_phase_flip(QuantumCircuit &qc, std::vector<size_t> qubits)
{
	switch (qubits.size())
	{
		case 0:
			return;

		case 1:
			qc.add_gate(GateKind::Z, {qubits[0]});
			return;

		default:
			qc.add_gate(GateKind::MCP, qubits, {std::numbers::pi});
			return;
	}
}

QuantumCircuit phase_oracle_builder(const std::string& marked_bitstring)
{
	QuantumCircuit qc;

	const size_t n = marked_bitstring.size();

	std::vector<size_t> zero_qubits;

	for (size_t pos = 0; pos != n; ++pos)
	{
		const char bit = marked_bitstring[pos];

		if (bit != '0' && bit != '1')
			throw std::invalid_argument("marked_bitstring must contain only 0 or 1");

		// String is displayed as |q(n-1)...q0>
		const size_t q = n - 1 - pos;

		if (bit == '0')
			zero_qubits.push_back(q);
	}

	for (size_t q : zero_qubits)
		qc.add_gate(GateKind::X, {q});

	std::vector<size_t> qubits(n);
	std::iota(qubits.begin(), qubits.end(), size_t{0});

	apply_multi_control_phase_flip(qc, qubits);

	for (size_t q : zero_qubits)
		qc.add_gate(GateKind::X, {q});

	return qc;
}

QuantumCircuit get_diffuser_circuit(size_t qubit_count)
{
	QuantumCircuit qc;

	for (size_t q = 0; q != qubit_count; ++q)
		qc.add_gate(GateKind::H, {q});

	for (size_t q = 0; q != qubit_count; ++q)
		qc.add_gate(GateKind::X, {q});

	std::vector<size_t> all_qubits;
	all_qubits.reserve(qubit_count);

	for (size_t q = 0; q != qubit_count; ++q)
		all_qubits.push_back(q);

	// Phase flip |111...111>.
	qc.add_gate(GateKind::MCP, all_qubits, {std::numbers::pi});

	for (size_t q = 0; q != qubit_count; ++q)
		qc.add_gate(GateKind::X, {q});

	for (size_t q = 0; q != qubit_count; ++q)
		qc.add_gate(GateKind::H, {q});

	return qc;
}

size_t grover_iteration_count(size_t qubit_count)
{
	const double state_count = static_cast<double>(size_t{1} << qubit_count);

	return std::max<size_t>(
		1,
		static_cast<size_t>(
			std::floor((std::numbers::pi / 4.0) * std::sqrt(state_count))
		)
	);
}

QuantumCircuit build_grover_algo_circuit(size_t qubit_count, const QuantumCircuit &oracle)
{
	static const QuantumCircuit diffuser_qc = get_diffuser_circuit(qubit_count);

	QuantumCircuit grover_qc;

	for (size_t q = 0; q != qubit_count; q++)
		grover_qc.add_gate(GateKind::H, {q});

	const size_t iterations = grover_iteration_count(qubit_count);

	for (size_t i = 0; i != iterations; i++)
	{
		grover_qc += oracle;
		grover_qc += diffuser_qc;
	}

	return grover_qc;
}

int main(int ac, char **av)
{
	if (ac < 2)
		return 1;

	QuantumCircuit oracle = phase_oracle_builder(av[1]);
	QuantumCircuit qc = build_grover_algo_circuit(strlen(av[1]), oracle);

	auto visualizer = BlochStepVisualizer(strlen(av[1]), qc);
	visualizer.show();
}
