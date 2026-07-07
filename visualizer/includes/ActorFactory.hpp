#pragma once

#include <array>
#include <vtkActor.h>
#include <vtkSmartPointer.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkBillboardTextActor3D.h>
#include <vtkSphereSource.h>
#include <vtkArrowSource.h>
#include <vtkTextProperty.h>
#include <vtkTextActor.h>
#include <vtkProperty.h>
#include <vtkMatrix4x4.h>
#include <vtkTransform.h>

#define UNPACK_COORD(c) c[0], c[1], c[2]
using Coordinate = std::array<double, 3>;

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
	inline vtkSmartPointer<vtkActor> MakeSphereActor(double radius = 1.0, const Coordinate &center = {0, 0, 0})
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

	inline std::pair<vtkSmartPointer<vtkActor>, vtkSmartPointer<vtkPolyDataMapper>> MakeArrowActor(const Coordinate &pos = {0, 0, 0}, const Coordinate &orientation = {0, 0, 0}, double scale = 1)
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

	inline std::pair<vtkSmartPointer<vtkActor>, vtkSmartPointer<vtkPolyDataMapper>> MakeSpinArrowActor(const Coordinate &center, const Coordinate &dir = {0, 1, 0})
	{
		auto [actor, mapper] = MakeArrowActor({0, 0, 0});
		actor->GetProperty()->SetColor(1, 1, 0);

		SetArrowPos(actor, center, dir);
		return {actor, mapper};
	}

	inline vtkSmartPointer<vtkBillboardTextActor3D> Make3DLabelActor(const Coordinate &pos, const std::string &text = "")
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

	inline vtkSmartPointer<vtkTextActor> MakeTextActor(int x, int y, int font_size = 11)
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
