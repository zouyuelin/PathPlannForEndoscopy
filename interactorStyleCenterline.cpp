#include "interactorStyleCenterline.h"
#include <vtkLine.h>
#include <vtkLineSource.h>
#include <vtkCamera.h>

#include <unistd.h>
#include <Eigen/Core>
#include <Eigen/Eigen>

void MouseInteractorStyleCenterline::OnKeyPress()
{
	vtkRenderWindowInteractor *rwi = this->Interactor;
	std::string key = rwi->GetKeySym();

	if (key == "n")
	{
		if ((m_currentSeedPosition[0] != 0 && m_currentSeedPosition[1] != 0 && m_currentSeedPosition[2] != 0) || m_numOfSeeds == 0)
		{
			if (m_currentSeedType == 0)
				m_numOfSourceSeed++;
			else
				m_numOfTargetSeed++;

			m_numOfSeeds = m_numOfSourceSeed + m_numOfTargetSeed;

			// Create a sphere
			vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
			double sphereCenter[3];
			sphereSource->SetCenter(0, 0, 0);
			sphereSource->SetRadius(0.3);
            seedList.insert(pair<unsigned int, vtkSphereSource*>(m_numOfSeeds, sphereSource));

			m_currentSeedPosition[0] = sphereSource->GetCenter()[0];
			m_currentSeedPosition[1] = sphereSource->GetCenter()[1];
			m_currentSeedPosition[2] = sphereSource->GetCenter()[2];

			vtkSmartPointer<vtkPolyDataMapper> sphereMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
			sphereMapper->SetInputConnection(sphereSource->GetOutputPort());

			vtkSmartPointer<vtkActor> sphereActor = vtkSmartPointer<vtkActor>::New();
			sphereActor->SetMapper(sphereMapper);
			if (m_currentSeedType == 0)
				sphereActor->GetProperty()->SetColor(1, 0, 0);
			else
				sphereActor->GetProperty()->SetColor(0, 1, 0);

            seedActorList.insert(pair<unsigned int, vtkActor*>(m_numOfSeeds, sphereActor));
            seedTypeList.insert(pair<unsigned int, bool>(m_numOfSeeds, m_currentSeedType));
			this->GetCurrentRenderer()->AddActor(sphereActor);
		}
		else
			cout << "Invalid seed position, no new seed is inserted" << endl;
	}
	else if (key == "Tab")
	{
		// switch seed type
		m_currentSeedType = !m_currentSeedType;
		if (m_currentSeedType == 0)
		{
			cout << "Current seed type is source" << endl;
			if (m_numOfSeeds > 0)
			{
				m_numOfSourceSeed++;
				if (m_numOfTargetSeed > 0)
					m_numOfTargetSeed--;
				seedTypeList[m_numOfSeeds] = m_currentSeedType;
				this->GetCurrentRenderer()->GetActors()->GetLastActor()->GetProperty()->SetColor(1, 0, 0);
			}
		}
		else
		{
			cout << "Current seed type is target" << endl;
			if (m_numOfSeeds > 0)
			{
				if (m_numOfSourceSeed>0)
					m_numOfSourceSeed --;
				m_numOfTargetSeed++;
				seedTypeList[m_numOfSeeds] = m_currentSeedType;
				this->GetCurrentRenderer()->GetActors()->GetLastActor()->GetProperty()->SetColor(0, 1, 0);
			}	
		}
		cout << "Number of source seeds = " << m_numOfSourceSeed << ", number of target seeds = " << m_numOfTargetSeed << endl;
	}
	else if (key == "space")
	{
		this->Interactor->GetPicker()->Pick(this->Interactor->GetEventPosition()[0],
			this->Interactor->GetEventPosition()[1],
			0,  // always zero.
			this->Interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer());
		double picked[3];
		this->Interactor->GetPicker()->GetPickPosition(picked);

		m_currentSeedPosition[0] = picked[0];
		m_currentSeedPosition[1] = picked[1];
		m_currentSeedPosition[2] = picked[2];

		if (m_numOfSeeds != 0)
		{
			seedList[m_numOfSeeds]->SetCenter(picked);
			
		}

		//cout << picked[0] << "," << picked[1] << "," << picked[2] << endl;
	}
	else if (key == "Return")
	{
		if (m_numOfSourceSeed == 0 && m_numOfTargetSeed==0)
			cout << "Source/target seed not found" << endl;
		else if (m_numOfSourceSeed == 0 && m_numOfTargetSeed>0)
			cout << "Source seed not found" << endl;
		else if (m_numOfSourceSeed > 0 && m_numOfTargetSeed==0)
			cout << "Target seed not found" << endl;
		else if (m_currentSeedPosition[0] == 0 && m_currentSeedPosition[1] == 0 && m_currentSeedPosition[2] == 0)
			cout << "Invalid seed position, cannot calculate centerline" << endl;
		else
		{
			cout << "Centerline calculation in progress" << endl;
			// Create the kd tree
			vtkSmartPointer<vtkKdTreePointLocator> kDTree = vtkSmartPointer<vtkKdTreePointLocator>::New();
			kDTree->SetDataSet(m_surface);
			kDTree->BuildLocator();

			vtkSmartPointer<vtkIdList> sourceIds = vtkSmartPointer<vtkIdList>::New();
			sourceIds->SetNumberOfIds(m_numOfSourceSeed);
			vtkSmartPointer<vtkIdList> targetIds = vtkSmartPointer<vtkIdList>::New();
			targetIds->SetNumberOfIds(m_numOfTargetSeed);

			int _sourceSeedCount = 0;
			int _targetSeedCount = 0;

			for (int i = 0; i < m_numOfSeeds; i++)
			{
				// Find the closest point ids to the seeds
				//cout << seedList[i+1]->GetCenter()[0] << "," << seedList[i]->GetCenter()[1] << "," << seedList[i]->GetCenter()[2];
				vtkIdType iD = kDTree->FindClosestPoint(seedList[i + 1]->GetCenter());
				//std::cout << "The closest point is point " << iD << std::endl;
                if (seedTypeList[i + 1] == 0)
				{
					sourceIds->SetId(_sourceSeedCount, iD);
					_sourceSeedCount++;
				}
				else
				{
					targetIds->SetId(_targetSeedCount, iD);
					_targetSeedCount++;
				}

				seedActorList[i + 1]->SetVisibility(0);
			}


			Centerline* centerline = new Centerline;
			centerline->SetCappedSurface(m_surface);
			centerline->SetSourceIds(sourceIds);
			centerline->SetTargetIds(targetIds);
			centerline->SetAppendEndPoints(m_appendFlag);
			centerline->Update();

			m_centerline->DeepCopy(centerline->GetCenterline());

			// set surface opacity
			vtkActor* actor = vtkActor::SafeDownCast(this->GetCurrentRenderer()->GetActors()->GetItemAsObject(0));
			actor->GetProperty()->SetOpacity(0.5);

            SetThePoints();

		}
	}
    else if (key == "x")
    {
        Roaming();
    }
    else if (key == "a")
    {
        pathSelect += 1;
        if(pathSelect >= mPoints.size()-1)
            pathSelect = mPoints.size()-1;

        for(int i = 0;i < pointActors.size();i++)
        {
            if(i==pathSelect)
                pointActors[pathSelect]->GetProperty()->SetColor(1,0,0);
            else
                pointActors[i]->GetProperty()->SetColor(0,1,0);
        }
    }
    else if (key == "d")
    {
        pathSelect -= 1;
        if(pathSelect <= 0)
            pathSelect = 0;

        for(int i = 0;i < pointActors.size();i++)
        {
            if(i==pathSelect)
                pointActors[pathSelect]->GetProperty()->SetColor(1,0,0);
            else
                pointActors[i]->GetProperty()->SetColor(0,1,0);
        }
    }

    this->GetInteractor()->Render();
}

void MouseInteractorStyleCenterline::SetThePoints()
{
    vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();
    vtkSmartPointer<vtkPoints> pathpointsALL =  vtkSmartPointer<vtkPoints>::New();
    pathpointsALL->DeepCopy(m_centerline->GetPoints());
    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
    lines->DeepCopy(m_centerline->GetLines());

    int numberOfLines = m_centerline->GetLines()->GetNumberOfCells();
    cout<<"The lines number is :"<<numberOfLines<<endl;

    for (auto actor:pointActors)
    {
       this->GetCurrentRenderer()->RemoveActor(actor);
    }

    pointActors.clear();
    mPoints.clear();

    for(int i=0;i<numberOfLines;i++)
    {
        lines->GetNextCell(idList);
        int n = idList->GetNumberOfIds();
        vtkSmartPointer<vtkPoints> nPoints =  vtkSmartPointer<vtkPoints>::New();

        for (int id = 0; id < n; id++) {
           auto pointId = idList->GetId(id);
           nPoints->InsertNextPoint(pathpointsALL->GetPoint(pointId));
        }

        //pathpoints
        vtkSmartPointer<vtkPoints> pathpoints_ =  vtkSmartPointer<vtkPoints>::New();//ExtractCenterline(data);
        pathpoints_->DeepCopy(nPoints);

        //样条曲线拟合
        vtkNew<vtkKochanekSpline> xSpline;
        vtkNew<vtkKochanekSpline> ySpline;
        vtkNew<vtkKochanekSpline> zSpline;

        vtkSmartPointer<vtkParametricSpline> spline = vtkSmartPointer<vtkParametricSpline>::New();
            spline->SetPoints(pathpoints_);
            spline->SetXSpline(xSpline);
            spline->SetYSpline(ySpline);
            spline->SetZSpline(zSpline);
            spline->ClosedOff();

        vtkSmartPointer<vtkParametricFunctionSource> functionSource = vtkSmartPointer<vtkParametricFunctionSource>::New();
            functionSource->SetParametricFunction(spline);
            functionSource->SetUResolution(10*pathpoints_->GetNumberOfPoints()); //10 * pathpoints_->GetNumberOfPoints()
            functionSource->SetVResolution(10*pathpoints_->GetNumberOfPoints());
            functionSource->SetWResolution(10*pathpoints_->GetNumberOfPoints());
            functionSource->Update();

        vtkSmartPointer<vtkPolyDataMapper> pointMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            pointMapper->SetInputConnection(functionSource->GetOutputPort());

        vtkSmartPointer<vtkLODActor> actorPoints = vtkSmartPointer<vtkLODActor>::New();
            actorPoints->SetMapper(pointMapper);
            actorPoints->GetProperty()->SetColor(0,1,0);
            actorPoints->GetProperty()->SetLineWidth(2);

        pointActors.push_back(actorPoints);
        GetTheGuidePoints(spline,pathpoints_->GetNumberOfPoints());

        this->GetCurrentRenderer()->AddActor(actorPoints);
    }

}

void MouseInteractorStyleCenterline::GetTheGuidePoints(vtkSmartPointer<vtkParametricSpline> spline,unsigned int numPoints)
{
    //get the new points
//    vtkSmartPointer<vtkPoints> newPoints = vtkSmartPointer<vtkPoints>::New();
    vtkPoints *newPoints = vtkPoints::New();

    int NumberInsertPoints = 5 * numPoints;
    double ratioSpeed = 1.0/(NumberInsertPoints-1);

    for(int i = 0; i < NumberInsertPoints; i++)
    {
         double u[] = {i*ratioSpeed, 0, 0};
         double p[3];
         spline->Evaluate(u, p, nullptr);
         newPoints->InsertNextPoint(p);
    }
    mPoints.push_back(newPoints);
//    mPoints.back()->DeepCopy(newPoints);
}

void MouseInteractorStyleCenterline::Roaming()
{
    if(mPoints.size() != 0)
    {
        vtkSmartPointer<vtkPoints> newPoints = vtkSmartPointer<vtkPoints>::New();
        newPoints->DeepCopy(mPoints[pathSelect]);

        vtkRenderWindowInteractor * rwi = this->Interactor;
        vtkCamera* aCamera = vtkCamera::SafeDownCast(this->GetCurrentRenderer()->GetActiveCamera());

        for(int i=0;i<newPoints->GetNumberOfPoints()-1;i++) //newPoints->GetNumberOfPoints()-1
        {
            double* point = new double[3];
            double* NextPoint = new double[3];
            newPoints->GetPoint(i,point);
            newPoints->GetPoint(i+1,NextPoint);

            Eigen::Vector3d lastPosition(point[0],point[1],point[2] );
            Eigen::Vector3d NextPosition(NextPoint[0],NextPoint[1],NextPoint[2] );
            Eigen::Vector3d focalPoint = NextPosition + (NextPosition - lastPosition)*1.0;

            aCamera->SetPosition ( lastPosition[0], lastPosition[1], lastPosition[2] );
            aCamera->SetFocalPoint( focalPoint[0], focalPoint[1], focalPoint[2] );
            //aCamera->SetViewUp ( 0, -1, 0);
            aCamera->ComputeViewPlaneNormal();
            //It is important to reset the camera clipping range.
            this->CurrentRenderer->ResetCameraClippingRange();

            this->Interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->SetActiveCamera(aCamera);
            rwi->Render();

            usleep(30*1000);
         }
    }
    else
    {
        cout<<"Please set a path to roaming"<<endl;
    }
}

void MouseInteractorStyleCenterline::SetCenterline(vtkPolyData* centerline)
{
	m_centerline = centerline;
}

void MouseInteractorStyleCenterline::SetSurface(vtkPolyData* surface)
{
	m_surface = surface;
}

void MouseInteractorStyleCenterline::SetAppendEndPoints(bool appendFlag)
{
	m_appendFlag = appendFlag;
}


vtkStandardNewMacro(MouseInteractorStyleCenterline);

