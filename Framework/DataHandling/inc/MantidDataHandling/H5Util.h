// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/DllConfig.h"

#include <map>
#include <string>
#include <vector>

// forward declarations
namespace H5 {
class DataSpace;
class DataSet;
class DSetCreatPropList;
class DataType;
class Group;
class H5File;
class H5Object;
} // namespace H5

namespace Mantid {
namespace DataHandling {
namespace H5Util {
/** H5Util : TODO: DESCRIPTION
 */

/// Create a 1D data-space to hold data of length.
MANTID_DATAHANDLING_DLL H5::DataSpace getDataSpace(const size_t length);

/// Create a 1D data-space that will hold the supplied vector.
template <typename NumT> H5::DataSpace getDataSpace(const std::vector<NumT> &data);

/// Convert a primitive type to the appropriate H5::DataType.
template <typename NumT> H5::DataType getType();

MANTID_DATAHANDLING_DLL H5::Group createGroupNXS(H5::H5File &file, const std::string &name, const std::string &nxtype);

MANTID_DATAHANDLING_DLL H5::Group createGroupNXS(H5::Group &group, const std::string &name, const std::string &nxtype);

MANTID_DATAHANDLING_DLL H5::Group createGroupCanSAS(H5::Group &group, const std::string &name,
                                                    const std::string &nxtype, const std::string &cstype);

MANTID_DATAHANDLING_DLL H5::Group createGroupCanSAS(H5::H5File &file, const std::string &name,
                                                    const std::string &nxtype, const std::string &cstype);

/**
 * Sets up the chunking and compression rate.
 * @param length
 * @param deflateLevel
 * @return The configured property list
 */
MANTID_DATAHANDLING_DLL H5::DSetCreatPropList setCompressionAttributes(const std::size_t length,
                                                                       const int deflateLevel = 6);

template <typename LocationType>
void writeStrAttribute(LocationType &location, const std::string &name, const std::string &value);

template <typename NumT, typename LocationType>
void writeNumAttribute(LocationType &location, const std::string &name, const NumT &value);

template <typename NumT, typename LocationType>
void writeNumAttribute(LocationType &location, const std::string &name, const std::vector<NumT> &value);

MANTID_DATAHANDLING_DLL void write(H5::Group &group, const std::string &name, const std::string &value);

template <typename T>
void writeScalarDataSetWithStrAttributes(H5::Group &group, const std::string &name, const T &value,
                                         const std::map<std::string, std::string> &attributes);

template <typename NumT> void writeArray1D(H5::Group &group, const std::string &name, const std::vector<NumT> &values);

MANTID_DATAHANDLING_DLL std::string readString(H5::H5File &file, const std::string &path);

MANTID_DATAHANDLING_DLL std::string readString(H5::Group &group, const std::string &name);

MANTID_DATAHANDLING_DLL std::string readString(const H5::DataSet &dataset);

MANTID_DATAHANDLING_DLL std::vector<std::string> readStringVector(H5::Group &, const std::string &);

MANTID_DATAHANDLING_DLL bool hasAttribute(const H5::DataSet &dataset, const char *attributeName);

template <typename T> T readAttributeAsStrType(const H5::DataSet &dataset, const std::string &attributeName);

template <typename NumT, typename LocationType>
NumT readNumAttributeCoerce(LocationType &location, const std::string &attributeName);

template <typename NumT, typename LocationType>
std::vector<NumT> readNumArrayAttributeCoerce(LocationType &location, const std::string &attributeName);

template <typename NumT> std::vector<NumT> readArray1DCoerce(H5::Group &group, const std::string &name);

template <typename NumT> std::vector<NumT> readArray1DCoerce(H5::DataSet &dataset);

/// Test if a group already exists within an HDF5 file or parent group.
MANTID_DATAHANDLING_DLL bool groupExists(H5::H5Object &h5, const std::string &groupPath);

/// Test if an attribute is present and has a specific string value for an HDF5 group or dataset.
MANTID_DATAHANDLING_DLL bool keyHasValue(H5::H5Object &h5, const std::string &key, const std::string &value);

/// Copy a group and all of its contents, between the same or different HDF5 files or groups.
MANTID_DATAHANDLING_DLL void copyGroup(H5::H5Object &dest, const std::string &destGroupPath, H5::H5Object &src,
                                       const std::string &srcGroupPath);

/**
 * Delete a target link for a group or dataset from a parent group.
 * If this is the last link to the target in the HDF5 graph, then it will be removed from the file.
 */
MANTID_DATAHANDLING_DLL void deleteObjectLink(H5::H5Object &h5, const std::string &target);

} // namespace H5Util
} // namespace DataHandling
} // namespace Mantid
