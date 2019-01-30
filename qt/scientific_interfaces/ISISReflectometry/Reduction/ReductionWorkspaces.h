// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CUSTOMINTERFACES_REDUCTIONWORKSPACES_H_
#define MANTID_CUSTOMINTERFACES_REDUCTIONWORKSPACES_H_

#include "Common/DllConfig.h"
#include "TransmissionRunPair.h"
#include <boost/algorithm/string/join.hpp>
#include <boost/optional.hpp>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL ReductionWorkspaces {
public:
  ReductionWorkspaces(std::vector<std::string> inputRunNumbers,
                      TransmissionRunPair transmissionRuns);

  std::vector<std::string> const &inputRunNumbers() const;
  TransmissionRunPair const &transmissionRuns() const;
  std::string const &iVsLambda() const;
  std::string const &iVsQ() const;
  std::string const &iVsQBinned() const;

  void setOutputNames(std::string iVsLambda, std::string iVsQ,
                      std::string iVsQBinned);
  void resetOutputNames();

private:
  std::vector<std::string> m_inputRunNumbers;
  TransmissionRunPair m_transmissionRuns;
  std::string m_iVsLambda;
  std::string m_iVsQ;
  std::string m_iVsQBinned;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(ReductionWorkspaces const &lhs,
                                               ReductionWorkspaces const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(ReductionWorkspaces const &lhs,
                                               ReductionWorkspaces const &rhs);

TransmissionRunPair
transmissionWorkspaceNames(TransmissionRunPair const &transmissionRuns);

MANTIDQT_ISISREFLECTOMETRY_DLL ReductionWorkspaces
workspaceNames(std::vector<std::string> const &inputRunNumbers,
               TransmissionRunPair const &transmissionRuns);

MANTIDQT_ISISREFLECTOMETRY_DLL std::string postprocessedWorkspaceName(
    std::vector<std::vector<std::string> const *> const &summedRunNumbers);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_REDUCTIONWORKSPACES_H_
