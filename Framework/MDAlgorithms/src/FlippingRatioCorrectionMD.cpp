// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidMDAlgorithms/FlippingRatioCorrectionMD.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDEventFactory.h"

#include <muParser.h>

namespace Mantid {
namespace MDAlgorithms {
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FlippingRatioCorrectionMD)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string FlippingRatioCorrectionMD::name() const { return "FlippingRatioCorrectionMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int FlippingRatioCorrectionMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string FlippingRatioCorrectionMD::category() const {
  return "MDAlgorithms\\Transforms";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string FlippingRatioCorrectionMD::summary() const {
  return "Creates MDEvent workspaces with polarization flipping ratio corrections."
         "The polarization might be angle dependent.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void FlippingRatioCorrectionMD::init() {
  declareProperty(Kernel::make_unique<WorkspaceProperty<API::IMDEventWorkspace>>(
                                                   "InputWorkspace", "", Kernel::Direction::Input),
                                               "An input MDEventWorkspace. Must be in Q_sample frame.");
  declareProperty(Kernel::make_unique<Mantid::Kernel::PropertyWithValue<std::string>>(
                      "FlippingRatio", "", Direction::Input),
                  "Formula to define the flipping ratio. It can depend on the variables in the list "
                  "of sample logs defined below");
  declareProperty(Kernel::make_unique<Mantid::Kernel::ArrayProperty<std::string>>("SampleLogs", Direction::Input),
                  "Comma separated list of sample logs that can appear in the formula for flipping ratio");
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<API::Workspace>>("OutputWorkspace1", "",
                                                             Direction::Output),
      "Output workspace 1. Equal to Input workspace multiplied by FR/(FR-1).");
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<API::Workspace>>("OutputWorkspace2", "",
                                                             Direction::Output),
      "Output workspace 2. Equal to Input workspace multiplied by 1/(FR-1).");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void FlippingRatioCorrectionMD::exec() {
  Mantid::API::IMDEventWorkspace_sptr inWS;
  inWS = getProperty("InputWorkspace");
  std::string inputFormula = getProperty("FlippingRatio");
  std::vector<std::string> sampleLogStrings = getProperty("SampleLogs");


  std::vector<double> flippingRatio, C1, C2; //C1=FR/(FR-1.), C2=1./(FR-1.)

  for(uint16_t i=0; i < inWS->getNumExperimentInfo(); i++) {
      const auto &currentRun=inWS->getExperimentInfo(i)->run();
      std::vector<double> sampleLogs(sampleLogStrings.size());
      mu::Parser muParser;
      for(size_t j=0;j<sampleLogStrings.size();j++) {
        std::string s=sampleLogStrings[j];
        double val=currentRun.getLogAsSingleValue(s,Kernel::Math::TimeAveragedMean);
        sampleLogs[j]=val;
        muParser.DefineVar(s, &sampleLogs[j]);
      }
      muParser.DefineConst("pi", M_PI);
      muParser.SetExpr(inputFormula);
      try {
        flippingRatio.push_back(muParser.Eval());
      }
      catch (mu::Parser::exception_type &e) {
        g_log.error()<<"Parsing error in experiment info "<<i<<"\n"<< e.GetMsg() << std::endl;
        throw std::runtime_error("Parsing error");
      }
  }
  for(auto &fr:flippingRatio) {
    C1.push_back(fr/(fr-1.));
    C2.push_back(1./(fr-1.));
  }
  // Create workspaces by cloning
  API::IMDWorkspace_sptr outputWS1, outputWS2;
  API::IAlgorithm_sptr cloneMD = createChildAlgorithm("CloneMDWorkspace",0,0.25,true);
  cloneMD->setProperty("InputWorkspace",inWS);
  cloneMD->setProperty("OutputWorkspace",getPropertyValue("OutputWorkspace1"));
  cloneMD->executeAsChildAlg();
  outputWS1 = cloneMD->getProperty("OutputWorkspace");
  API::IMDEventWorkspace_sptr event1 =
      boost::dynamic_pointer_cast<API::IMDEventWorkspace>(outputWS1);
  cloneMD->setProperty("OutputWorkspace",getPropertyValue("OutputWorkspace2"));
  cloneMD->setChildStartProgress(0.25);
  cloneMD->setChildEndProgress(0.5);
  cloneMD->executeAsChildAlg();
  outputWS2 = cloneMD->getProperty("OutputWorkspace");
  API::IMDEventWorkspace_sptr event2 =
      boost::dynamic_pointer_cast<API::IMDEventWorkspace>(outputWS2);

  if(event1){
    m_factor=C1;
    CALL_MDEVENT_FUNCTION(this->executeTemplatedMDE, event1);
    this->setProperty("OutputWorkspace1", event1);
  } else {
    throw std::runtime_error("Could not clone the workspace for first correction");
  }
  if(event2){
    m_factor=C2;
    CALL_MDEVENT_FUNCTION(this->executeTemplatedMDE, event2);
    this->setProperty("OutputWorkspace2", event2);
  } else {
    throw std::runtime_error("Could not clone the workspace for first correction");
  }
}

template<typename MDE, size_t nd>
void FlippingRatioCorrectionMD::executeTemplatedMDE(typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws)
{
  // Get all the MDBoxes contained
  DataObjects::MDBoxBase<MDE, nd> *parentBox = ws->getBox();
  std::vector<API::IMDNode *> boxes;
  parentBox->getBoxes(boxes, 1000, true);

  bool fileBackedTarget(false);
  Kernel::DiskBuffer *dbuff(nullptr);
  if (ws->isFileBacked()) {
    fileBackedTarget = true;
    dbuff = ws->getBoxController()->getFileIO();
  }
  for (auto &boxe : boxes) {
    DataObjects::MDBox<MDE, nd> *box = dynamic_cast<DataObjects::MDBox<MDE, nd> *>(boxe);
    if (box) {
      typename std::vector<MDE> &events = box->getEvents();
      size_t ic(events.size());
      auto it = events.begin();
      auto it_end = events.end();
      for (; it != it_end; it++) {
        size_t ind = static_cast<size_t>(it->getRunIndex());
        float scalar=static_cast<float>(m_factor[ind]);
        float scalarSquared=static_cast<float>(m_factor[ind]*m_factor[ind]);
        // Multiply weight by a scalar, propagating error
        float oldSignal = it->getSignal();
        float signal = oldSignal * scalar;
        float errorSquared = scalarSquared * it->getErrorSquared();
        it->setSignal(signal);
        it->setErrorSquared(errorSquared);
      }
      box->releaseEvents();
      if (fileBackedTarget && ic > 0) {
        Kernel::ISaveable *const pSaver(box->getISaveable());
        dbuff->toWrite(pSaver);
      }
    }
  }
  // Recalculate the totals
  ws->refreshCache();
  // Mark file-backed workspace as dirty
  ws->setFileNeedsUpdating(true);
}

} // namespace MDAlgorithms
} // namespace Mantid
