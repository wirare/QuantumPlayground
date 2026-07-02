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
#include <vtkActor2D.h>

VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);

#include <QuantumCircuit.hpp>

using Coordinate = std::array<double, 3>;

#define UNPACK_COORD(c) c[0], c[1], c[2]

static void SetArrowPose(vtkActor* actor, const Coordinate& start, const Coordinate& direction)
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

	vtkSmartPointer<vtkActor> MakeArrowActor(const Coordinate &pos = {0, 0, 0}, const Coordinate &orientation = {0, 0, 0}, double scale = 1)
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

		return actor;
	}

	vtkSmartPointer<vtkActor> MakeSpinArrowActor(const Coordinate &center, const Coordinate &dir = {0, 1, 0})
	{
		auto arrow = MakeArrowActor(center);
		arrow->GetProperty()->SetColor(1, 1, 0);

		SetArrowPose(arrow, center, dir);
		return arrow;
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

class BlochStepVisualizer
{
	public:


	private:
		QuantumCircuit qc;
		size_t qubit_number = 0;
		size_t max_cols = 5;

		const std::vector<CircuitOperation> steps; 
		size_t step_index = 0;
		StateVector state;

		vtkSmartPointer<vtkRenderer> renderer;
		vtkSmartPointer<vtkRenderWindow> renderWindow;
		vtkSmartPointer<vtkRenderWindowInteractor> interactor;

		std::vector<Coordinate> centers;

		std::vector<vtkSmartPointer<vtkActor>> qubit_arrow_actors;
		std::vector<vtkSmartPointer<vtkActor>> bloch_sphere_axis_arrows;
		std::vector<vtkSmartPointer<vtkActor>> bloch_sphere_actors;
		std::vector<vtkSmartPointer<vtkBillboardTextActor3D>> label_actors;

		vtkSmartPointer<vtkTextActor> text_actor;
		vtkSmartPointer<vtkTextActor> info_actor;

		std::vector<Coordinate> make_centers()
		{
			constexpr double spacing = 3.2;
			const size_t cols = std::min(qubit_number, max_cols);
			const size_t rows = std::ceil(qubit_number / cols);

			std::vector<Coordinate> centers;

			for (size_t i = 0; i != qubit_number; i++)
			{
				const size_t row = static_cast<size_t>(i / cols);
				const size_t col = i % cols;

				const double x = (col - (cols - 1) / 2.0) * spacing;
				const double z = ((rows - 1) / 2.0 - row) * spacing;

				centers.push_back({x, 0.0, z});
			}

			return centers;
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

				auto x_arrow = ActorFactory::MakeArrowActor(center);
				x_arrow->GetProperty()->SetColor(1, 0, 0);
				x_arrow->GetProperty()->SetOpacity(0.35);

				auto y_arrow = ActorFactory::MakeArrowActor(center, {0, 0, 90});
				y_arrow->GetProperty()->SetColor(0, 1, 0);
				y_arrow->GetProperty()->SetOpacity(0.35);

				auto z_arrow = ActorFactory::MakeArrowActor(center, {0, -90, 0});
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

				auto arrow_actor = ActorFactory::MakeSpinArrowActor(center);

				qubit_arrow_actors.push_back(arrow_actor);
				renderer->AddActor(arrow_actor);
			}

			text_actor = ActorFactory::MakeTextActor(10, 780, 11);
			renderer->AddActor2D(text_actor);

			info_actor = ActorFactory::MakeTextActor(10, 35, 9);
		}


};
