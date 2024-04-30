// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "FittingModel.h"

#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class IFitDataModel;
class IFitOutput;
class IFitTab;
class InelasticFitPropertyBrowser;

class MANTIDQT_INELASTIC_DLL IFittingPresenter {
public:
  virtual void notifyFunctionChanged() = 0;
};

class MANTIDQT_INELASTIC_DLL FittingPresenter : public IFittingPresenter {
public:
  FittingPresenter(IFitTab *tab, InelasticFitPropertyBrowser *browser, std::unique_ptr<FittingModel> model);

  void notifyFunctionChanged() override;

  void validate(UserInputValidator &validator);

  void setFitFunction(Mantid::API::MultiDomainFunction_sptr function);

  void setFittingMode(FittingMode mode);
  FittingMode getFittingMode() const;

  void setErrorsEnabled(bool const enable);
  void setFitEnabled(bool const enable);
  void setCurrentDataset(FitDomainIndex index);
  Mantid::API::MultiDomainFunction_sptr fitFunction() const;
  std::string minimizer() const;

  EstimationDataSelector getEstimationDataSelector() const;
  void updateParameterEstimationData(DataForParameterEstimationCollection &&data);
  void estimateFunctionParameters(WorkspaceID workspaceID, WorkspaceIndex spectrum);

  void updateFunctionBrowserData(int nData, const QList<MantidWidgets::FunctionModelDataset> &datasets,
                                 const std::vector<double> &qValues,
                                 const std::vector<std::pair<std::string, size_t>> &fitResolutions);
  void updateFunctionListInBrowser(const std::map<std::string, std::string> &functionStrings);

  void cleanFailedRun(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm);
  void cleanFailedSingleRun(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm, WorkspaceID workspaceID);
  void removeFittingData();
  void addDefaultParameters();
  void removeDefaultParameters();

  void addOutput(Mantid::API::IAlgorithm_sptr &fittingAlgorithm);
  void addSingleFitOutput(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm, WorkspaceID workspaceID,
                          WorkspaceIndex spectrum);

  std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> fitProperties() const;

  Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const;
  IFitOutput *getFitOutput() const;
  Mantid::API::IAlgorithm_sptr getFittingAlgorithm(FittingMode mode) const;
  Mantid::API::IAlgorithm_sptr getSingleFit(WorkspaceID workspaceID, WorkspaceIndex spectrum) const;
  std::string getOutputBasename() const;

  IFitDataModel *getFitDataModel() const;

  bool isPreviouslyFit(WorkspaceID workspaceID, WorkspaceIndex spectrum) const;

  void setFWHM(WorkspaceID WorkspaceID, double fwhm);
  void setBackground(WorkspaceID WorkspaceID, double background);

  void updateFittingModeFromBrowser();
  void updateFitBrowserParameterValues(const std::unordered_map<std::string, ParameterValue> &params =
                                           std::unordered_map<std::string, ParameterValue>());
  void updateFitBrowserParameterValuesFromAlg(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm,
                                              std::size_t const &numberOfDomains);
  void updateFitStatus(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm, std::size_t const &numberOfDomains);
  void updateFitTypeString();

private:
  IFitTab *m_tab;
  InelasticFitPropertyBrowser *m_fitPropertyBrowser;
  std::unique_ptr<FittingModel> m_model;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt