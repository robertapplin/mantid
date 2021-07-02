// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IGLDisplay.h"
#include "IInstrumentDisplay.h"
#include "IQtDisplay.h"

#include <gmock/gmock.h>
#include <memory>

namespace MantidQt::MantidWidgets {

class MockInstrumentDisplay : public IInstrumentDisplay {
public:
  MOCK_METHOD(IGLDisplay *, getGLDisplay, (), (const, override));
  MOCK_METHOD(IQtDisplay *, getQtDisplay, (), (const, override));
  MOCK_METHOD(void, installEventFilter, (QObject * obj), (override));
};
} // namespace MantidQt::MantidWidgets