#ifndef INTERACTORSTYLECENTERLINE_H
#define INTERACTORSTYLECENTERLINE_H

#include <vtkPolyData.h>
#include <vtkIdList.h>
#include <vtkSmartPointer.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkPointData.h>
#include <vtkObjectFactory.h>
#include <vtkPointPicker.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkCoordinate.h>
#include <vtkKdTreePointLocator.h>
#include <vtkSphereSource.h>
#include <vtkRenderer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkActorCollection.h>
#include <vtkParametricSpline.h>
#include <vtkParametricFunctionSource.h>
#include <vtkKochanekSpline.h>
#include <map>

#include "Centerline.h"
#include <vtkLODActor.h>
#include <thread>
#include <memory>
#include <functional>

class MouseInteractorStyleCenterline : public vtkInteractorStyleTrackballCamera
{
public:
	static MouseInteractorStyleCenterline* New();
	vtkTypeMacro(MouseInteractorStyleCenterline, vtkInteractorStyleTrackballCamera);

	void OnKeyPress();
	void SetSurface(vtkPolyData* surface);
	void SetCenterline(vtkPolyData* centerline);
	void SetAppendEndPoints(bool appendFlag);
    void SetThePoints();
    void Roaming();
    void GetTheGuidePoints(vtkSmartPointer<vtkParametricSpline> spline, unsigned int numPoints);

private:
    vtkPolyData* m_surface = nullptr;
    vtkSphereSource* m_sphere = nullptr;
    vtkPolyData* m_centerline = nullptr;
	bool m_currentSeedType = 0; //0 and 1 are source and target seed type respectively
	bool m_appendFlag = 0;
	unsigned int m_numOfSourceSeed = 0;
	unsigned int m_numOfTargetSeed = 0;
	unsigned int m_numOfSeeds = 0;
    map<unsigned int, vtkSphereSource*> seedList;
    vector<vtkLODActor*> pointActors;
    vector<vtkPoints*> mPoints;
    map<unsigned int, vtkActor*> seedActorList;
    map<unsigned int, bool> seedTypeList;
	double m_currentSeedPosition[3] = { 0, 0, 0 };

    int pathSelect = 0;
};

#endif
