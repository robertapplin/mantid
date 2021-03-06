// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <fstream>
#include <string>
#include <vector>

class Component;

class AssReader
{
public:
  AssReader(const std::string& filename);
  virtual ~AssReader(void);

  // Find the next component in the current file
  Component* parseFile();
private:
  std::ifstream m_assfile;
};
