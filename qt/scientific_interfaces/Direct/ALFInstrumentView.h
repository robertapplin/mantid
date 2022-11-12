// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"

#include "MantidKernel/V3D.h"

#include <optional>
#include <string>

#include <QAction>
#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <QWidget>

namespace MantidQt {

namespace API {
class FileFinderWidget;
}

namespace MantidWidgets {
class InstrumentActor;
class InstrumentWidget;
} // namespace MantidWidgets

namespace CustomInterfaces {

class IALFInstrumentPresenter;

class MANTIDQT_DIRECT_DLL IALFInstrumentView {

public:
  virtual void setUpInstrument(std::string const &fileName) = 0;

  virtual QWidget *generateLoadWidget() = 0;
  virtual MantidWidgets::InstrumentWidget *getInstrumentView() = 0;

  virtual void subscribePresenter(IALFInstrumentPresenter *presenter) = 0;

  virtual std::optional<std::string> getFile() = 0;
  virtual void setRunQuietly(std::string const &runNumber) = 0;

  virtual MantidWidgets::InstrumentActor &getInstrumentActor() const = 0;

  virtual std::vector<std::size_t> getSelectedDetectors() const = 0;

  virtual void warningBox(std::string const &message) = 0;
};

class MANTIDQT_DIRECT_DLL ALFInstrumentView final : public QWidget, public IALFInstrumentView {
  Q_OBJECT

public:
  explicit ALFInstrumentView(QWidget *parent = nullptr);

  void setUpInstrument(std::string const &fileName) override;

  QWidget *generateLoadWidget() override;
  MantidWidgets::InstrumentWidget *getInstrumentView() override { return m_instrumentWidget; };

  void subscribePresenter(IALFInstrumentPresenter *presenter) override;

  std::optional<std::string> getFile() override;
  void setRunQuietly(std::string const &runNumber) override;

  MantidWidgets::InstrumentActor &getInstrumentActor() const override;

  std::vector<std::size_t> getSelectedDetectors() const override;

  void warningBox(std::string const &message) override;

private slots:
  void fileLoaded();
  void notifyShapeChanged();
  void selectWholeTube();
  void extractSingleTube();
  void averageTube();

private:
  API::FileFinderWidget *m_files;
  MantidWidgets::InstrumentWidget *m_instrumentWidget;
  QAction *m_extractAction;
  QAction *m_averageAction;

  IALFInstrumentPresenter *m_presenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt
