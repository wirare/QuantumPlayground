#include <vtkAutoInit.h>

VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);

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

#include <array>
#include <iostream>
#include <string>

class MyInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
	static MyInteractorStyle* New();
	vtkTypeMacro(MyInteractorStyle, vtkInteractorStyleTrackballCamera);

	void SetSphereActor(vtkActor* actor)
	{
		this->SphereActor = actor;
	}

	void SetRenderer(vtkRenderer* renderer)
	{
		this->Renderer = renderer;
	}

	void OnKeyPress() override
	{
		const std::string key = this->GetInteractor()->GetKeySym();

		if (key == "r")
		{
			if (this->Renderer)
				this->Renderer->ResetCamera();

			this->GetInteractor()->GetRenderWindow()->Render();
		}
		else if (key == "w")
		{
			SetAllActorsRepresentationToWireframe();
			this->GetInteractor()->GetRenderWindow()->Render();
		}
		else if (key == "s")
		{
			SetAllActorsRepresentationToSurface();
			this->GetInteractor()->GetRenderWindow()->Render();
		}
		else if (key == "h")
		{
			if (this->SphereActor)
			{
				this->SphereActor->SetVisibility(!this->SphereActor->GetVisibility());
				this->GetInteractor()->GetRenderWindow()->Render();
			}
		}
		else if (key == "Left")
		{
			if (this->SphereActor)
			{
				double p[3];
				this->SphereActor->GetPosition(p);
				this->SphereActor->SetPosition(p[0] - 0.1, p[1], p[2]);
				this->GetInteractor()->GetRenderWindow()->Render();
			}
		}
		else if (key == "Right")
		{
			if (this->SphereActor)
			{
				double p[3];
				this->SphereActor->GetPosition(p);
				this->SphereActor->SetPosition(p[0] + 0.1, p[1], p[2]);
				this->GetInteractor()->GetRenderWindow()->Render();
			}
		}
		else if (key == "Escape")
		{
			this->GetInteractor()->TerminateApp();
		}

		vtkInteractorStyleTrackballCamera::OnKeyPress();
	}

private:
	vtkActor* SphereActor = nullptr;
	vtkRenderer* Renderer = nullptr;

	void SetAllActorsRepresentationToWireframe()
	{
		if (!this->Renderer)
			return;

		auto actors = this->Renderer->GetActors();
		actors->InitTraversal();

		vtkActor* actor = nullptr;
		while ((actor = actors->GetNextActor()) != nullptr)
			actor->GetProperty()->SetRepresentationToWireframe();
	}

	void SetAllActorsRepresentationToSurface()
	{
		if (!this->Renderer)
			return;

		auto actors = this->Renderer->GetActors();
		actors->InitTraversal();

		vtkActor* actor = nullptr;
		while ((actor = actors->GetNextActor()) != nullptr)
			actor->GetProperty()->SetRepresentationToSurface();
	}
};

vtkStandardNewMacro(MyInteractorStyle);

static vtkSmartPointer<vtkActor> MakeSphereActor()
{
	vtkNew<vtkSphereSource> sphere;
	sphere->SetCenter(0.0, 0.0, 0.0);
	sphere->SetRadius(0.5);
	sphere->SetThetaResolution(48);
	sphere->SetPhiResolution(48);

	vtkNew<vtkPolyDataMapper> mapper;
	mapper->SetInputConnection(sphere->GetOutputPort());

	vtkNew<vtkActor> actor;
	actor->SetMapper(mapper);
	actor->GetProperty()->SetColor(0.1, 0.4, 1.0);
	actor->GetProperty()->SetSpecular(0.5);
	actor->GetProperty()->SetSpecularPower(30.0);

	return actor;
}

static vtkSmartPointer<vtkActor> MakeTriangleMeshActor()
{
	vtkNew<vtkPoints> points;

	points->InsertNextPoint(-0.8, -0.8, 0.0);
	points->InsertNextPoint( 0.8, -0.8, 0.0);
	points->InsertNextPoint( 0.0,  0.8, 0.0);

	vtkNew<vtkCellArray> triangles;

	vtkIdType triangle[3] = {0, 1, 2};
	triangles->InsertNextCell(3, triangle);

	vtkNew<vtkPolyData> polyData;
	polyData->SetPoints(points);
	polyData->SetPolys(triangles);

	vtkNew<vtkPolyDataMapper> mapper;
	mapper->SetInputData(polyData);

	vtkNew<vtkActor> actor;
	actor->SetMapper(mapper);
	actor->SetPosition(1.5, 0.0, 0.0);
	actor->GetProperty()->SetColor(0.0, 0.8, 0.2);
	actor->GetProperty()->SetOpacity(0.8);

	return actor;
}

static vtkSmartPointer<vtkActor> MakeArrowActor()
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

	// vtkArrowSource points along +X by default.
	actor->SetPosition(-1.5, 0.0, 0.0);
	actor->SetScale(1.2, 1.2, 1.2);

	// Rotate in 3D.
	actor->RotateZ(30.0);
	actor->RotateY(-20.0);

	actor->GetProperty()->SetColor(1.0, 0.6, 0.0);

	return actor;
}

int main()
{
	vtkNew<vtkRenderer> renderer;
	renderer->SetBackground(0.08, 0.08, 0.10);

	vtkSmartPointer<vtkActor> sphereActor = MakeSphereActor();
	vtkSmartPointer<vtkActor> meshActor = MakeTriangleMeshActor();
	vtkSmartPointer<vtkActor> arrowActor = MakeArrowActor();

	renderer->AddActor(sphereActor);
	renderer->AddActor(meshActor);
	renderer->AddActor(arrowActor);

	vtkNew<vtkRenderWindow> renderWindow;
	renderWindow->AddRenderer(renderer);
	renderWindow->SetSize(1280, 720);
	renderWindow->SetWindowName("VTK C++ 3D Demo");

	vtkNew<vtkRenderWindowInteractor> interactor;
	interactor->SetRenderWindow(renderWindow);

	vtkNew<MyInteractorStyle> style;
	style->SetSphereActor(sphereActor);
	style->SetRenderer(renderer);
	interactor->SetInteractorStyle(style);

	renderer->ResetCamera();

	renderWindow->Render();
	interactor->Start();

	return 0;
}
