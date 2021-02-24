# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidworkbench package


def input_qinputdialog(prompt=None):
    from qtpy.QtWidgets import QInputDialog

    dlg = QInputDialog()
    dlg.setInputMode(QInputDialog.TextInput)
    dlg.setLabelText(str(prompt) if prompt is not None else "")
    accepted = dlg.exec_()
    if accepted:
        return dlg.textValue()
    else:
        raise RuntimeError("User input request cancelled")
