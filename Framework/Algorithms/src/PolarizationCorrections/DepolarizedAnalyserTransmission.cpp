// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/PolarizationCorrections/DepolarizedAnalyserTransmission.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/Divide.h"

namespace {
/// Property Names
namespace PropNames {
std::string_view constexpr DEP_WORKSPACE{"DepolarisedWorkspace"};
std::string_view constexpr MT_WORKSPACE{"EmptyCellWorkspace"};
std::string_view constexpr EMPTY_CELL_TRANS_START{"TEStartingValue"};
std::string_view constexpr DEPOL_OPACITY_START{"PxDStartingValue"};
std::string_view constexpr OUTPUT_WORKSPACE{"OutputWorkspace"};
std::string_view constexpr OUTPUT_FIT{"OutputFitCurves"};
std::string_view constexpr OUTPUT_COV_MATRIX{"OutputCovarianceMatrix"};
std::string_view constexpr GROUP_INPUT{"Input Workspaces"};
std::string_view constexpr GROUP_OUTPUT{"Output Workspaces"};
std::string_view constexpr GROUP_FIT{"Fit Starting Values"};
} // namespace PropNames

/// Initial fitting function values.
namespace FitValues {
double constexpr LAMBDA_CONVERSION_FACTOR = -0.0733;
double constexpr EMPTY_CELL_TRANS_START = 0.9;
double constexpr DEPOL_OPACITY_START = 12.6;
std::string_view constexpr EMPTY_CELL_TRANS_NAME = "T_E";
std::string_view constexpr DEPOL_OPACITY_NAME = "pxd";
double constexpr START_X = 1.75;
double constexpr END_X = 14;
std::string_view constexpr FIT_SUCCESS{"success"};

std::ostringstream createFunctionStrStream() {
  std::ostringstream func;
  func << "name=UserFunction, Formula=" << EMPTY_CELL_TRANS_NAME << "*exp(" << LAMBDA_CONVERSION_FACTOR << "*"
       << DEPOL_OPACITY_NAME << "*x)";
  return func;
}
} // namespace FitValues
} // namespace

namespace Mantid::Algorithms {

using namespace API;
using namespace Kernel;

// Register the algorithm in the AlgorithmFactory
DECLARE_ALGORITHM(DepolarizedAnalyserTransmission)

std::string const DepolarizedAnalyserTransmission::summary() const {
  return "Calculate the transmission rate through a depolarised He3 cell.";
}

void DepolarizedAnalyserTransmission::init() {
  auto wsValidator = std::make_shared<WorkspaceUnitValidator>("Wavelength");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(std::string(PropNames::DEP_WORKSPACE), "",
                                                           Kernel::Direction::Input, wsValidator),
      "The fully depolarised helium cell workspace. Should contain a single spectra. Units must be in wavelength.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(std::string(PropNames::MT_WORKSPACE), "",
                                                                       Kernel::Direction::Input, wsValidator),
                  "The empty cell workspace. Must contain a single spectra. Units must be in wavelength");
  declareProperty(std::string(PropNames::EMPTY_CELL_TRANS_START), FitValues::EMPTY_CELL_TRANS_START,
                  "Starting value for the empty analyser cell transmission fit property " +
                      std::string(FitValues::EMPTY_CELL_TRANS_NAME) + ".");
  declareProperty(std::string(PropNames::DEPOL_OPACITY_START), FitValues::DEPOL_OPACITY_START,
                  "Starting value for the depolarised cell transmission fit property " +
                      std::string(FitValues::DEPOL_OPACITY_NAME) + ".");
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(std::string(PropNames::OUTPUT_WORKSPACE), "",
                                                                       Kernel::Direction::Output),
                  "The name of the table workspace containing the fit parameter results.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      std::string(PropNames::OUTPUT_FIT), "", Kernel::Direction::Output, PropertyMode::Optional),
                  "The name of the workspace containing the calculated fit curve.");
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(
                      std::string(PropNames::OUTPUT_COV_MATRIX), "", Kernel::Direction::Output, PropertyMode::Optional),
                  "The name of the table workspace containing the normalised covariance matrix from the fit.");

  auto const &inputGroup = std::string(PropNames::GROUP_INPUT);
  setPropertyGroup(std::string(PropNames::DEP_WORKSPACE), inputGroup);
  setPropertyGroup(std::string(PropNames::MT_WORKSPACE), inputGroup);
  auto const &fitGroup = std::string(PropNames::GROUP_FIT);
  setPropertyGroup(std::string(PropNames::EMPTY_CELL_TRANS_START), fitGroup);
  setPropertyGroup(std::string(PropNames::DEPOL_OPACITY_START), fitGroup);
  auto const &outputGroup = std::string(PropNames::GROUP_OUTPUT);
  setPropertyGroup(std::string(PropNames::OUTPUT_WORKSPACE), outputGroup);
  setPropertyGroup(std::string(PropNames::OUTPUT_FIT), outputGroup);
  setPropertyGroup(std::string(PropNames::OUTPUT_COV_MATRIX), outputGroup);
}

std::map<std::string, std::string> DepolarizedAnalyserTransmission::validateInputs() {
  std::map<std::string, std::string> result;
  MatrixWorkspace_sptr const &depWs = getProperty(std::string(PropNames::DEP_WORKSPACE));
  if (depWs->getNumberHistograms() != 1) {
    result[std::string(PropNames::DEP_WORKSPACE)] =
        "The depolarised workspace must contain a single spectrum. Contains " +
        std::to_string(depWs->getNumberHistograms()) + " spectra.";
  }
  MatrixWorkspace_sptr const &mtWs = getProperty(std::string(PropNames::MT_WORKSPACE));
  if (mtWs->getNumberHistograms() != 1) {
    result[std::string(PropNames::MT_WORKSPACE)] =
        "The empty cell workspace must contain a single spectrum. Contains " +
        std::to_string(mtWs->getNumberHistograms()) + " spectra.";
  }

  if (!WorkspaceHelpers::matchingBins(*depWs, *mtWs, true)) {
    result[std::string(PropNames::DEP_WORKSPACE)] =
        "The bins in the DepolarisedWorkspace and EmptyCellWorkspace do not match.";
  }
  return result;
}

void DepolarizedAnalyserTransmission::exec() {
  auto const &dividedWs = calcDepolarisedProportion();
  calcWavelengthDependentTransmission(dividedWs, getPropertyValue(std::string(PropNames::OUTPUT_WORKSPACE)));
}

MatrixWorkspace_sptr DepolarizedAnalyserTransmission::calcDepolarisedProportion() {
  MatrixWorkspace_sptr const &depWs = getProperty(std::string(PropNames::DEP_WORKSPACE));
  MatrixWorkspace_sptr const &mtWs = getProperty(std::string(PropNames::MT_WORKSPACE));
  auto divideAlg = createChildAlgorithm("Divide");
  divideAlg->setProperty("LHSWorkspace", depWs);
  divideAlg->setProperty("RHSWorkspace", mtWs);
  divideAlg->execute();
  return divideAlg->getProperty(std::string(PropNames::OUTPUT_WORKSPACE));
}

void DepolarizedAnalyserTransmission::calcWavelengthDependentTransmission(MatrixWorkspace_sptr const &inputWs,
                                                                          std::string const &outputWsName) {
  auto funcStream = FitValues::createFunctionStrStream();
  funcStream << "," << FitValues::EMPTY_CELL_TRANS_NAME << "="
             << getPropertyValue(std::string(PropNames::EMPTY_CELL_TRANS_START));
  funcStream << "," << FitValues::DEPOL_OPACITY_NAME << "="
             << getPropertyValue(std::string(PropNames::DEPOL_OPACITY_START));
  auto const &func = FunctionFactory::Instance().createInitialized(funcStream.str());
  auto fitAlg = createChildAlgorithm("Fit");
  fitAlg->setProperty("Function", func);
  fitAlg->setProperty("InputWorkspace", inputWs);
  fitAlg->setProperty("IgnoreInvalidData", true);
  fitAlg->setProperty("StartX", FitValues::START_X);
  fitAlg->setProperty("EndX", FitValues::END_X);
  fitAlg->setPropertyValue("Output", outputWsName);
  fitAlg->execute();

  std::string const &status = fitAlg->getProperty("OutputStatus");
  if (!fitAlg->isExecuted() || status != FitValues::FIT_SUCCESS) {
    auto const &errMsg{"Failed to fit to transmission workspace, " + inputWs->getName() + ": " + status};
    throw std::runtime_error(errMsg);
  }
  // If a non-monitor MT workspace is provided by mistake the workspace to be fitted can contain only NaNs/infs due to
  // divide-by-0 results. In this case, the fit succeeds but the quality is 0, so we should still throw an error.
  double const &fitQuality = fitAlg->getProperty("OutputChi2overDoF");
  if (fitQuality <= 0) {
    throw std::runtime_error("Failed to fit to transmission workspace, " + inputWs->getName() +
                             ": Fit quality is too low (" + std::to_string(fitQuality) +
                             "). You may want to check that the correct monitor spectrum was provided.");
  }
  ITableWorkspace_sptr const &paramWs = fitAlg->getProperty("OutputParameters");
  setProperty(std::string(PropNames::OUTPUT_WORKSPACE), paramWs);

  if (!getPropertyValue(std::string(PropNames::OUTPUT_FIT)).empty()) {
    MatrixWorkspace_sptr const &fitWs = fitAlg->getProperty("OutputWorkspace");
    setProperty(std::string(PropNames::OUTPUT_FIT), fitWs);
  }

  if (!getPropertyValue(std::string(PropNames::OUTPUT_COV_MATRIX)).empty()) {
    ITableWorkspace_sptr const &normCovMatrix = fitAlg->getProperty("OutputNormalisedCovarianceMatrix");
    calcNonNormCovarianceMatrix(normCovMatrix, paramWs);
  }
}

void DepolarizedAnalyserTransmission::calcNonNormCovarianceMatrix(ITableWorkspace_sptr const &normCovMatrix,
                                                                  ITableWorkspace_sptr const &paramsWs) {
  auto const &T_EError = paramsWs->getColumn("Error")->toDouble(0);
  auto const &pxdError = paramsWs->getColumn("Error")->toDouble(1);

  // Diagonal terms given by s_ii = (err_i)^2 where s_ii is the non-normalised matrix.
  auto const &covMatrix_00 = std::pow(T_EError, 2);
  auto const &covMatrix_11 = std::pow(pxdError, 2);

  // Off-diagonal terms given by s_ij = n_ij * sqrt(s_ii * s_jj)/100
  auto const &scaleFactor = std::sqrt(covMatrix_00 * covMatrix_11) / 100.0;
  // Column indexes are increased by 1 to account for the "Name" column.
  auto const &covMatrix_01 = normCovMatrix->getColumn(2)->toDouble(0) * scaleFactor;
  auto const &covMatrix_10 = normCovMatrix->getColumn(1)->toDouble(1) * scaleFactor;

  // Make a TableWorkspace, which matches the original normalised one's format.
  auto covMatrix = WorkspaceFactory::Instance().createTable("TableWorkspace");
  covMatrix->addColumn("str", "Name");
  covMatrix->addColumn("double", std::string(FitValues::EMPTY_CELL_TRANS_NAME));
  covMatrix->addColumn("double", std::string(FitValues::DEPOL_OPACITY_NAME));
  TableRow row = covMatrix->appendRow();
  row << std::string(FitValues::EMPTY_CELL_TRANS_NAME) << covMatrix_00 << covMatrix_01;
  row = covMatrix->appendRow();
  row << std::string(FitValues::DEPOL_OPACITY_NAME) << covMatrix_10 << covMatrix_11;
  setProperty(std::string(PropNames::OUTPUT_COV_MATRIX), covMatrix);
}

} // namespace Mantid::Algorithms
