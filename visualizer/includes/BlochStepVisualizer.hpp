#pragma once

#include <QuantumCircuit.hpp>
#include <StateVector.hpp>
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
#include <vtkAutoInit.h>
#include <vtkBillboardTextActor3D.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <ActorFactory.hpp>

inline double norm_squared(const Coordinate &v)
{
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

inline double length(const Coordinate &v)
{
	return std::sqrt(norm_squared(v));
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

			renderWindow->SetSize(1920, 1080);
			renderWindow->AddRenderer(renderer);
			interactor->SetRenderWindow(renderWindow);

			setup_camera();
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

		void setup_camera()
		{
			const double d = camera_distance();

			vtkCamera *camera = renderer->GetActiveCamera();

			camera->SetPosition(0, -d, 2.5);
			camera->SetFocalPoint(0, 0, 0);
			camera->SetViewUp(0, 0, 1);

			renderer->ResetCameraClippingRange();
		}

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

			text_actor = ActorFactory::MakeTextActor(30, 720, 28);
			renderer->AddActor(text_actor);

			info_actor = ActorFactory::MakeTextActor(30, 60, 22);
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
			setup_camera();
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

