#include <iostream>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkSmartPointer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPolyData.h>
#include <vtkPolyDataReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkSTLReader.h>
#include <vtkSTLWriter.h>
#include <vtkUnstructuredGridVolumeMapper.h>
#include <vtkProperty.h>
#include <vtkIdList.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkPointData.h>
#include <vtkLookupTable.h>
#include <vtkScalarBarActor.h>
#include <vtkCleanPolyData.h>
#include <vtkTriangleFilter.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkImageData.h>
#include <vtkImageShiftScale.h>
#include <vtkPNGWriter.h>

#include <vtkvmtkCenterlineAttributesFilter.h>
#include <vtkvmtkCenterlineBranchExtractor.h>
#include <vtkvmtkCenterlineBranchGeometry.h>
#include <vtkvmtkPolyDataCenterlineGroupsClipper.h>
#include <vtkvmtkCenterlineBifurcationReferenceSystems.h>
#include <vtkvmtkPolyDataCenterlineAngularMetricFilter.h>
#include <vtkvmtkPolyDataCenterlineAbscissaMetricFilter.h>
#include <vtkvmtkPolyDataReferenceSystemBoundaryMetricFilter.h>
#include <vtkvmtkPolyDataMultipleCylinderHarmonicMappingFilter.h>
#include <vtkvmtkPolyDataStretchMappingFilter.h>
#include <vtkvmtkCapPolyData.h>
#include <vtkvmtkPolyDataPatchingFilter.h>
#include <vtkvmtkCenterlineGeometry.h>
#include <vtkParametricSpline.h>
#include <vtkParametricFunctionSource.h>
#include <itkImageSeriesReader.h>
#include <itkImage.h>

#include "interactorStyleCenterline.h"
#include "Centerline.h"

//itk
#include <itkImageSeriesReader.h>
#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImageToVTKImageFilter.h>

#include <iostream>

using namespace std;

void ReadSurface(string surfacePath, vtkPolyData* surface);
vtkSmartPointer<vtkImageData> GetDICOMPolyData(string fileName);
void ExtractCenterline(string surfacePath, vtkPolyData* inputSurface, vtkPolyData* cappedSurface);

int main(int argc,char**argv)
{
    if(argc<2)
    {
        cout<<"please input a model\n";
        return -1;
    }
    string stl_path = argv[1];
    // /home/zyl/ubuntu/VTK/VMTK/vmtk-1.4.0/example/Vessel-Centerline-Extraction/test_data/left/lumen.stl
    // /home/zyl/ubuntu/VTK/CT_LungAndAbdominal/stl/lung.stl

    vtkSmartPointer<vtkPolyData> inputSurface = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkPolyData> cappedSurface = vtkSmartPointer<vtkPolyData>::New();

    ExtractCenterline(stl_path, inputSurface, cappedSurface);

    return 0;
}

vtkSmartPointer<vtkImageData> GetDICOMPolyData(string fileName)
{
    using ImageType = itk::Image<signed short, 3>; //多张dcm图片时

    itk::ImageSeriesReader< ImageType >::Pointer reader = itk::ImageSeriesReader< ImageType >::New();
    itk::GDCMImageIO::Pointer docomIO = itk::GDCMImageIO::New();
    reader->SetImageIO(docomIO);

    itk::GDCMSeriesFileNames::Pointer nameGenerator = itk::GDCMSeriesFileNames::New();
    nameGenerator->SetDirectory(fileName);

    std::vector< std::string > DICOMNames = nameGenerator->GetInputFileNames();
    reader->SetFileNames(DICOMNames);
    reader->Update();

    typedef itk::ImageToVTKImageFilter< ImageType> itkTovtkFilterType;
    itkTovtkFilterType::Pointer itkTovtkImageFilter = itkTovtkFilterType::New();
    itkTovtkImageFilter->SetInput(reader->GetOutput());
    itkTovtkImageFilter->Update();

    vtkSmartPointer<vtkImageData> imagedata = vtkSmartPointer<vtkImageData>::New();
    imagedata->DeepCopy(itkTovtkImageFilter->GetOutput());
    return imagedata;
}


void ReadSurface(string surfacePath, vtkPolyData* surface)
{
    // read surface
    vtkSmartPointer<vtkSTLReader> reader = vtkSmartPointer<vtkSTLReader>::New();
    reader->SetFileName(surfacePath.c_str());
    reader->Update();
    surface->DeepCopy(reader->GetOutput());
}

void ExtractCenterline(string surfacePath, vtkPolyData* inputSurface, vtkPolyData* cappedSurface)
{
    // read surface file
        ReadSurface(surfacePath, inputSurface);

        // capping
        vtkSmartPointer<vtkvmtkCapPolyData> capper = vtkSmartPointer<vtkvmtkCapPolyData>::New();
        capper->SetInputData(inputSurface);
        capper->Update();

        vtkSmartPointer<vtkCleanPolyData> cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
        cleaner->SetInputData(capper->GetOutput());
        cleaner->Update();

        vtkSmartPointer<vtkTriangleFilter> triangulator = vtkSmartPointer<vtkTriangleFilter>::New();
        triangulator->SetInputData(cleaner->GetOutput());
        triangulator->PassLinesOff();
        triangulator->PassVertsOff();
        triangulator->Update();

        cappedSurface->DeepCopy(triangulator->GetOutput());

        cout<<"The number of the lumen is:" <<capper->GetCapCenterIds()->GetNumberOfIds()<<endl;

        vtkSmartPointer<vtkPolyDataMapper> surfaceMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        surfaceMapper->SetInputData(cappedSurface);
        vtkSmartPointer<vtkActor> surfaceActor = vtkSmartPointer<vtkActor>::New();
        surfaceActor->SetMapper(surfaceMapper);

        // put the actor into render window
        vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();
        ren->AddActor(surfaceActor);

        vtkSmartPointer<vtkPolyData> centerline = vtkSmartPointer<vtkPolyData>::New();
//        vtkSmartPointer<vtkPolyDataMapper> centerlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
//        centerlineMapper->SetInputData(centerline);
//        vtkSmartPointer<vtkActor> centerlineActor = vtkSmartPointer<vtkActor>::New();
//        centerlineActor->SetMapper(centerlineMapper);
//        ren->AddActor(centerlineActor);

        vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
        renWin->AddRenderer(ren);
        renWin->SetSize(1920,1080);

        vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
        vtkSmartPointer<MouseInteractorStyleCenterline> style = vtkSmartPointer<MouseInteractorStyleCenterline>::New();
        iren->SetInteractorStyle(style);
        iren->SetRenderWindow(renWin);
        style->SetSurface(cappedSurface);
        style->SetCenterline(centerline);

        iren->Initialize();
        renWin->Render();
        ren->ResetCamera();
        iren->Start();

}
