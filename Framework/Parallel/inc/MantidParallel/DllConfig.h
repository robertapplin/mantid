// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/*
  This file contains the DLLExport/DLLImport linkage configuration for the
  Parallel library
*/
#include "MantidKernel/System.h"

#ifdef IN_MANTID_PARALLEL
#define MANTID_PARALLEL_DLL DLLExport
#else
#define MANTID_PARALLEL_DLL DLLImport
#endif // IN_MANTID_PARALLEL
