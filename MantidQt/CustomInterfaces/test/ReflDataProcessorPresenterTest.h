#ifndef MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflGenericDataProcessorPresenterFactory.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorMockObjects.h"
#include "MantidQtMantidWidgets/DataProcessorUI/GenericDataProcessorPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/ProgressableViewMockObject.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::MantidWidgets;
using namespace testing;

class ReflDataProcessorPresenterTest : public CxxTest::TestSuite {

private:
  ITableWorkspace_sptr
  createWorkspace(const std::string &wsName,
                  const DataProcessorWhiteList &whitelist) {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

    const int ncols = static_cast<int>(whitelist.size());

    auto colGroup = ws->addColumn("str", "Group");
    colGroup->setPlotType(0);

    for (int col = 0; col < ncols; col++) {
      auto column = ws->addColumn("str", whitelist.colNameFromColIndex(col));
      column->setPlotType(0);
    }

    if (wsName.length() > 0)
      AnalysisDataService::Instance().addOrReplace(wsName, ws);

    return ws;
  }

  ITableWorkspace_sptr
  createPrefilledWorkspace(const std::string &wsName,
                                const DataProcessorWhiteList &whitelist) {
    auto ws = createWorkspace(wsName, whitelist);
    TableRow row = ws->appendRow();
    row << "0"
        << "13460"
        << "0.7"
        << "13463,13464"
        << "0.01"
        << "0.06"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "0"
        << "13462"
        << "2.3"
        << "13463,13464"
        << "0.035"
        << "0.3"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "1"
        << "13469"
        << "0.7"
        << "13463,13464"
        << "0.01"
        << "0.06"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "1"
        << "13470"
        << "2.3"
        << "13463,13464"
        << "0.01"
        << "0.06"
        << "0.04"
        << "1"
        << "";
    return ws;
  }

  ITableWorkspace_sptr
  createPrefilledMixedWorkspace(const std::string &wsName,
                                const DataProcessorWhiteList &whitelist) {
    auto ws = createWorkspace(wsName, whitelist);
    TableRow row = ws->appendRow();
    row << "0"
        << "38415"
        << "0.5069"
        << "38393"
        << "0.0065"
        << "0.0737"
        << "0.0148"
        << "1"
        << "";
    row = ws->appendRow();
    row << "0"
        << "38417"
        << "0.5069"
        << "38393"
        << "0.0065"
        << "0.0737"
        << "0.0198"
        << "1"
        << "";
    return ws;
  }

  ITableWorkspace_sptr
    createPrefilledMinimalWorkspace(const std::string &wsName,
      const DataProcessorWhiteList &whitelist) {
    auto ws = createWorkspace(wsName, whitelist);
    TableRow row = ws->appendRow();
    row << "0"
      << "38415"
      << "0.5069"
      << ""
      << "0.0065"
      << "0.0737"
      << "0.0148"
      << "1"
      << "";
    row = ws->appendRow();
    return ws;
  }

  ReflGenericDataProcessorPresenterFactory presenterFactory;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflDataProcessorPresenterTest *createSuite() {
    return new ReflDataProcessorPresenterTest();
  }
  static void destroySuite(ReflDataProcessorPresenterTest *suite) {
    delete suite;
  }

  ReflDataProcessorPresenterTest() { FrameworkManager::Instance(); }

  void testProcessEventWorkspacesCustomSlicing() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    auto presenter = presenterFactory.create();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter->getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    std::set<int> grouplist;
    grouplist.insert(0);

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits the "process" button with the first group selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(grouplist));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingOptions())
        .Times(1)
        .WillOnce(Return("0,10,20,30"));
    EXPECT_CALL(mockMainPresenter, getPreprocessingOptions())
        .Times(6)
        .WillRepeatedly(Return(std::map<std::string, std::string>()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions())
        .Times(6)
        .WillRepeatedly(Return(""));
    EXPECT_CALL(mockMainPresenter, getPostprocessingOptions())
        .Times(3)
        .WillRepeatedly(Return(""));
    EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(mockDataProcessorView, getProcessInstrument())
        .Times(14)
        .WillRepeatedly(Return("INTER"));
    EXPECT_CALL(mockDataProcessorView, requestNotebookPath()).Times(0);

    presenter->notify(DataProcessorPresenter::ProcessFlag);

    // Check output workspaces were created as expected
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_13460_0_10"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_13460_10_20"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_13460_20_30"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_13462_0_10"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_13462_10_20"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_13462_20_30"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_13460_0_10"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_13460_10_20"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_13460_20_30"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_13462_0_10"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_13462_10_20"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_13462_20_30"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist(
        "IvsQ_13460_0_10_13462_0_10"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist(
        "IvsQ_13460_10_20_13462_10_20"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist(
        "IvsQ_13460_20_30_13462_20_30"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_13460_0_10"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_13460_10_20"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_13460_20_30"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_13462_0_10"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_13462_10_20"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_13462_20_30"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_13460"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_13462"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_13460_0_10"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_13460_10_20"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_13460_20_30"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_13462_0_10"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_13462_10_20"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_13462_20_30"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_13460_monitors"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_13460_monitors"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TRANS_13463"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TRANS_13464"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TRANS_13463_13464"));

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("IvsLam_13460_0_10");
    AnalysisDataService::Instance().remove("IvsLam_13460_10_20");
    AnalysisDataService::Instance().remove("IvsLam_13460_20_30");
    AnalysisDataService::Instance().remove("IvsLam_13462_0_10");
    AnalysisDataService::Instance().remove("IvsLam_13462_10_20");
    AnalysisDataService::Instance().remove("IvsLam_13462_20_30");
    AnalysisDataService::Instance().remove("IvsQ_13460_0_10");
    AnalysisDataService::Instance().remove("IvsQ_13460_10_20");
    AnalysisDataService::Instance().remove("IvsQ_13460_20_30");
    AnalysisDataService::Instance().remove("IvsQ_13462_0_10");
    AnalysisDataService::Instance().remove("IvsQ_13462_10_20");
    AnalysisDataService::Instance().remove("IvsQ_13462_20_30");
    AnalysisDataService::Instance().remove("IvsQ_13460_0_10_13462_0_10");
    AnalysisDataService::Instance().remove("IvsQ_13460_10_20_13462_10_20");
    AnalysisDataService::Instance().remove("IvsQ_13460_20_30_13462_20_30");
    AnalysisDataService::Instance().remove("IvsQ_binned_13460_0_10");
    AnalysisDataService::Instance().remove("IvsQ_binned_13460_10_20");
    AnalysisDataService::Instance().remove("IvsQ_binned_13460_20_30");
    AnalysisDataService::Instance().remove("IvsQ_binned_13462_0_10");
    AnalysisDataService::Instance().remove("IvsQ_binned_13462_10_20");
    AnalysisDataService::Instance().remove("IvsQ_binned_13462_20_30");
    AnalysisDataService::Instance().remove("TOF_13460");
    AnalysisDataService::Instance().remove("TOF_13462");
    AnalysisDataService::Instance().remove("TOF_13460_0_10");
    AnalysisDataService::Instance().remove("TOF_13460_10_20");
    AnalysisDataService::Instance().remove("TOF_13460_20_30");
    AnalysisDataService::Instance().remove("TOF_13462_0_10");
    AnalysisDataService::Instance().remove("TOF_13462_10_20");
    AnalysisDataService::Instance().remove("TOF_13462_20_30");
    AnalysisDataService::Instance().remove("TOF_13460_monitors");
    AnalysisDataService::Instance().remove("TOF_13462_monitors");
    AnalysisDataService::Instance().remove("TRANS_13463");
    AnalysisDataService::Instance().remove("TRANS_13464");
    AnalysisDataService::Instance().remove("TRANS_13463_13464");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testProcessWithNotebookWarn() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    auto presenter = presenterFactory.create();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    createPrefilledMinimalWorkspace("TestWorkspace", presenter->getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    std::set<int> groupList;
    groupList.insert(0);

    // We should be warned
    EXPECT_CALL(mockMainPresenter, giveUserWarning(_, _)).Times(1);

    // The user hits the "process" button with the first group selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(groupList));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingOptions())
        .Times(1)
        .WillOnce(Return("0,10"));
    EXPECT_CALL(mockMainPresenter, getPreprocessingOptions())
        .Times(1)
        .WillRepeatedly(Return(std::map<std::string, std::string>()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions())
        .Times(1)
        .WillRepeatedly(Return(""));
    EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(mockDataProcessorView, requestNotebookPath()).Times(0);

    presenter->notify(DataProcessorPresenter::ProcessFlag);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("IvsLam_38415_0_10");
    AnalysisDataService::Instance().remove("IvsQ_38415_0_10");
    AnalysisDataService::Instance().remove("IvsQ_binned_38415_0_10");
    AnalysisDataService::Instance().remove("TOF_38415");
    AnalysisDataService::Instance().remove("TOF_38415_0_10");
    AnalysisDataService::Instance().remove("TOF_38415_monitors");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testProcessMixedWorkspacesWarn() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    auto presenter = presenterFactory.create();
    presenter->acceptViews(&mockDataProcessorView, &mockProgress);
    presenter->accept(&mockMainPresenter);

    for (auto &x : AnalysisDataService::Instance().getObjectNames())
      std::cout << "Name = " << x << "\n";

    createPrefilledMixedWorkspace("TestWorkspace", presenter->getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter->notify(DataProcessorPresenter::OpenTableFlag);

    std::set<int> grouplist;
    grouplist.insert(0);

    // We should be warned
    EXPECT_CALL(mockMainPresenter, giveUserWarning(_, _)).Times(1);

    // The user hits the "process" button with the first group selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(grouplist));
    EXPECT_CALL(mockMainPresenter, getTimeSlicingOptions())
        .Times(1)
        .WillOnce(Return("0,10,20,30"));

    presenter->notify(DataProcessorPresenter::ProcessFlag);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("TOF_38415");
    AnalysisDataService::Instance().remove("TOF_38417");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPlotRowPythonCode() {}

  void testPlotGroupPythonCode() {}

  void testPlotRowWarn() {}

  void testPlotGroupWarn() {}
};

#endif /* MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTERTEST_H */