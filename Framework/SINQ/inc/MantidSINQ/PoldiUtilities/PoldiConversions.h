// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidSINQ/DllConfig.h"

namespace Mantid {

namespace Poldi {

namespace Conversions {
// conversion between TOF (in musec) and d (in Angstrom), related through
// distance in mm and sin(theta)
double MANTID_SINQ_DLL TOFtoD(double tof, double distance, double sinTheta);
double MANTID_SINQ_DLL dtoTOF(double d, double distance, double sinTheta);

// conversions between d and Q
double MANTID_SINQ_DLL dToQ(double d);
double MANTID_SINQ_DLL qToD(double q);

// conversions between degree and radians
double MANTID_SINQ_DLL degToRad(double degree);
double MANTID_SINQ_DLL radToDeg(double radians);
} // namespace Conversions
} // namespace Poldi
} // namespace Mantid
