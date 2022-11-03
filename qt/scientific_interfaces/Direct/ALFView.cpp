// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFView.h"

#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentModel.h"
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentView.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneModel.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneView.h"

namespace MantidQt::CustomInterfaces {

DECLARE_SUBWINDOW(ALFView)

ALFView::ALFView(QWidget *parent) : UserSubWindow(parent), m_instrumentPresenter(), m_analysisPresenter() {
  this->setWindowTitle("ALF View");

  m_instrumentModel = std::make_unique<MantidWidgets::BaseCustomInstrumentModel>();
  m_instrumentPresenter = std::make_unique<MantidWidgets::BaseCustomInstrumentPresenter>(
      new MantidWidgets::BaseCustomInstrumentView(m_instrumentModel->getInstrument(), this), m_instrumentModel.get());

  m_analysisPresenter = std::make_unique<MantidWidgets::PlotFitAnalysisPanePresenter>(
      new MantidWidgets::PlotFitAnalysisPaneView(-15.0, 15.0, this),
      std::make_unique<MantidWidgets::PlotFitAnalysisPaneModel>());

  m_instrumentPresenter->subscribeAnalysisPresenter(m_analysisPresenter.get());
}

void ALFView::initLayout() {
  auto widg = new QSplitter(Qt::Vertical);
  auto *split = new QSplitter(Qt::Horizontal);
  split->addWidget(m_instrumentPresenter->getInstrumentView());
  split->addWidget(m_analysisPresenter->getView());
  widg->addWidget(m_instrumentPresenter->getLoadWidget());
  widg->addWidget(split);
  this->setCentralWidget(widg);
}

} // namespace MantidQt::CustomInterfaces
