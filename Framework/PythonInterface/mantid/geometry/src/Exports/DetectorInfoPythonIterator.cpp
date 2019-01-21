// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/api/DetectorInfoPythonIterator.h"
#include "MantidPythonInterface/core/VersionCompat.h"
#include <boost/python/class.hpp>
#include <boost/python/iterator.hpp>
#include <boost/python/module.hpp>

using Mantid::PythonInterface::DetectorInfoPythonIterator;
using namespace boost::python;

// Export DetectorInfoPythonIterator
void export_DetectorInfoPythonIterator() {

  // Export to Python
  class_<DetectorInfoPythonIterator>("DetectorInfoPythonIterator", no_init)
      .def("__iter__", objects::identity_function())
#ifdef IS_PY3K
      .def("__next__", &DetectorInfoPythonIterator::next)
#else
      .def("next", &DetectorInfoPythonIterator::next)
#endif
      ;
  /*
   Return value policy for next is to copy the const reference. Copy by value is
   essential for python 2.0 compatibility because items (DetectorInfoItem) will
   outlive their iterators if declared as part of for loops. There is no good
   way to deal with this other than to force a copy so that internals of the
   item are not also corrupted. Future developers may wish to choose a separte
   policy for python 3.0 where this is not a concern, and const ref returns
   would be faster.
  */
}
