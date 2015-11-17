# Copyright &copy; 2014-2015 ISIS Rutherford Appleton Laboratory, NScD
# Oak Ridge National Laboratory & European Spallation Source
#
# This file is part of Mantid.
# Mantid is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# Mantid is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# File change history is stored at: <https://github.com/mantidproject/mantid>.
# Code Documentation is available at: <http://doxygen.mantidproject.org>
"""
A collection of scripts for neutron imaging. Includes
pre-processing/preparation steps, reconstruction via third party
tools, and post-processing steps, in addition to input/output and
format conversion routines.

"""

try:
    import io
except ImportError as exc:
    raise ImportError("Inconsistency found. Could not import 'io' (input/output "
                      "routines) which should be available in this package. "
                      "Details/reason: {0}".format(exc))

try:
    import tool_imports
except ImportError as exc:
    raise ImportError("Inconsistency found. Could not import 'tool_imports' (for third "
                      "party tools such as Tomopy and Astra) which should be available "
                      "in this package. Details/reason: {0}".format(exc))
