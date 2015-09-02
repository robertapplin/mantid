#include "MantidVatesAPI/vtkDataSetToScaledDataSet.h"
#include "MantidKernel/Logger.h"

#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkUnsignedCharArray.h>
#include <vtkPointSet.h>
#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>
#include <vtkPVChangeOfBasisHelper.h>
#include <vtkInformation.h>

#include <stdexcept>

namespace {
Mantid::Kernel::Logger g_log("vtkDataSetTOScaledDataSet");
}

namespace Mantid {
namespace VATES {
/**
 * Standard constructor for object.
 */
vtkDataSetToScaledDataSet::vtkDataSetToScaledDataSet() {
}

vtkDataSetToScaledDataSet::~vtkDataSetToScaledDataSet() {}

/**
 * Process the input data. First, scale a copy of the points and apply
 * that to the output data. Next, update the metadata for range information.
 *
 * This is a data source method.
 *
 * @param xScale : Scale factor for the x direction
 * @param yScale : Scale factor for the y direction
 * @param zScale : Scale factor for the z direction
 * @param info : The dataset to scale
 * @return The resulting scaled dataset
 */
vtkPointSet *
vtkDataSetToScaledDataSet::execute(double xScale, double yScale, double zScale,
                                   vtkPointSet *inputData, vtkInformation* info) {

  // Extract output dataset from information.
  vtkPointSet *outputData = vtkPointSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

  return execute(xScale, yScale, zScale, inputData, outputData);

}




/**
 * Process the input data. First, scale a copy of the points and apply
 * that to the output data. Next, update the metadata for range information.
 *
 * This is a data source method.
 *
 * @param xScale : Scale factor for the x direction
 * @param yScale : Scale factor for the y direction
 * @param zScale : Scale factor for the z direction
 * @param inputData : The dataset to scale
 * @param outputData : The output dataset. Optional. If not specified or null, new one created.
 * @return The resulting scaled dataset
 */
vtkPointSet *
vtkDataSetToScaledDataSet::execute(double xScale, double yScale, double zScale,
                                   vtkPointSet *inputData, vtkPointSet* outputData) {

  if (NULL == inputData) {
    throw std::runtime_error("Cannot construct vtkDataSetToScaledDataSet with "
                             "NULL input vtkPointSet");
  }

  if(outputData == NULL){
       outputData = inputData->NewInstance();
  }

  vtkPoints *points = inputData->GetPoints();

  double *point;
  vtkPoints *newPoints = vtkPoints::New();
  newPoints->Allocate(points->GetNumberOfPoints());
  for (int i = 0; i < points->GetNumberOfPoints(); i++) {
    point = points->GetPoint(i);
    point[0] *= xScale;
    point[1] *= yScale;
    point[2] *= zScale;
    newPoints->InsertNextPoint(point);
  }
  // Shallow copy the input.
  outputData->ShallowCopy(inputData);
  // Give the output dataset the scaled set of points.
  outputData->SetPoints(newPoints);

  this->updateMetaData(xScale, yScale, zScale, inputData, outputData);
  return outputData;
}

/**
 * In order for the axis range and labels to not come out scaled,
 * this function set metadata that ParaView will read to override
 * the scaling to return the original presentation.
 * See
 * http://www.paraview.org/ParaQ/Doc/Nightly/html/classvtkCubeAxesRepresentation.html
 * and
 * http://www.paraview.org/ParaView/Doc/Nightly/www/cxx-doc/classvtkPVChangeOfBasisHelper.html
 * for a better understanding.
 * @param xScale : Scale factor for the x direction
 * @param yScale : Scale factor for the y direction
 * @param zScale : Scale factor for the z direction
 * @param inputData : Input dataset
 * @param outputData : Output dataset
 */
void vtkDataSetToScaledDataSet::updateMetaData(double xScale, double yScale,
                                               double zScale, vtkPointSet *inputData, vtkPointSet *outputData) {
  // We need to put the scaling on the diagonal elements of the ChangeOfBasis
  // (COB) Matrix.
  vtkSmartPointer<vtkMatrix4x4> cobMatrix =
      vtkSmartPointer<vtkMatrix4x4>::New();
  cobMatrix->Identity();
  cobMatrix->Element[0][0] *= xScale;
  cobMatrix->Element[1][1] *= yScale;
  cobMatrix->Element[2][2] *= zScale;

  if (!vtkPVChangeOfBasisHelper::AddChangeOfBasisMatrixToFieldData(outputData,
                                                                   cobMatrix)) {
    g_log.warning("The Change-of-Basis-Matrix could not be added to the field "
                  "data of the scaled data set.\n");
  }

  // We also need to update the bounding box for the COB Matrix
  double boundingBox[6];
  inputData->GetBounds(boundingBox);
  if (!vtkPVChangeOfBasisHelper::AddBoundingBoxInBasis(outputData,
                                                       boundingBox)) {
    g_log.warning("The bounding box could not be added to the field data of "
                  "the scaled data set.\n");
  }
}

} // namespace VATES
} // namespace Mantid
