# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context import FrequencyContext, UNIT, MHz, GAUSS, FIELD, FREQ
from unittest import mock

FFT_NAME_RE_2 = "FFT; Re MUSR62260; Group; fwd; Asymmmetry; FD_Re"
FFT_NAME_RE_MOD = "FFT; Re MUSR62260; Group; fwd; Asymmmetry; FD_Mod"

FFT_NAME_COMPLEX_RE = "FFT; Re MUSR62260; Group; fwd; Asymmmetry; FD; Im MUSR62261; Group; top; Asymmetry; FD_Re"
FFT_NAME_COMPLEX_IM = "FFT; Re MUSR62260; Group; fwd; Asymmmetry; FD; Im MUSR62261; Group; top; Asymmetry; FD_Im"
FFT_NAME_COMPLEX_MOD = "FFT; Re MUSR62260; Group; fwd; Asymmmetry; FD; Im MUSR62261; Group; top; Asymmetry; FD_Mod"

PHASEQUAD_NAME_IM = (
    "FFT; Re MUSR62261; PhaseQuad FD MUSR62260; PhaseTable FD; top; bkwd; Im MUSR62261; PhaseQuad FD "
    "MUSR62260; PhaseTable FD; top; bkwd_Mod"
)
PHASEQUAD_NAME_RE = "FFT; Re MUSR62261; PhaseQuad FD MUSR62260; PhaseTable FD; top; bkwd_Mod"


def get_name(name, units):
    return name + UNIT + units


def alg_patch(_, name):
    return name


class MuonFreqContextTest(unittest.TestCase):
    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context.RenameWorkspace")
    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context.convert_to_field")
    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context.convert_to_freq")
    def setUp(self, freq, field, rename):
        freq.side_effect = alg_patch
        field.side_effect = alg_patch

        self.ws_freq = "MUSR62260_raw_data FD; MaxEnt"
        self.context = FrequencyContext()
        run = "62260"
        self.context.add_maxEnt(run, self.ws_freq)
        self.context.add_FFT(ws_freq_name=FFT_NAME_RE_2, Re_run="62260", Re="fwd", Im_run="", Im="")
        self.context.add_FFT(ws_freq_name=FFT_NAME_RE_MOD, Re_run="62260", Re="fwd", Im_run="", Im="")

    def test_widnow_title(self):
        self.assertEqual(self.context.window_title, "Frequency Domain Analysis")

    def test_add_maxEnt(self):
        self.assertEqual(list(self.context._maxEnt_freq.keys()), ["MUSR62260_raw_data FD; MaxEnt"])
        self.assertEqual(self.context._maxEnt_freq["MUSR62260_raw_data FD; MaxEnt"].run, "62260")
        self.assertEqual(self.context._maxEnt_freq["MUSR62260_raw_data FD; MaxEnt"].ws._freq, get_name(self.ws_freq, MHz))
        self.assertEqual(self.context._maxEnt_freq["MUSR62260_raw_data FD; MaxEnt"].ws._field, get_name(self.ws_freq, GAUSS))

    def test_add_FFT(self):
        self.assertCountEqual(list(self.context._FFT_freq.keys()), [FFT_NAME_RE_2, FFT_NAME_RE_MOD])
        self.assertEqual(self.context._FFT_freq[FFT_NAME_RE_2].Re_run, "62260")
        self.assertEqual(self.context._FFT_freq[FFT_NAME_RE_2].Re, "fwd")
        self.assertEqual(self.context._FFT_freq[FFT_NAME_RE_2].Im, None)
        self.assertEqual(self.context._FFT_freq[FFT_NAME_RE_2].Im_run, None)

        self.assertEqual(self.context._FFT_freq[FFT_NAME_RE_2].ws._freq, get_name(FFT_NAME_RE_2, MHz))
        self.assertEqual(self.context._FFT_freq[FFT_NAME_RE_2].ws._field, get_name(FFT_NAME_RE_2, GAUSS))

    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context.RenameWorkspace")
    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context.convert_to_field")
    def test_add_complex_FFT(self, field, rename):
        field.side_effect = alg_patch
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_RE, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_IM, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_MOD, Re_run="62260", Re="top", Im_run="62261", Im="fwd")

        self.assertCountEqual(
            list(self.context._FFT_freq.keys()),
            [FFT_NAME_COMPLEX_RE, FFT_NAME_COMPLEX_IM, FFT_NAME_COMPLEX_MOD, FFT_NAME_RE_2, FFT_NAME_RE_MOD],
        )
        self.assertEqual(self.context._FFT_freq[FFT_NAME_COMPLEX_RE].Re_run, "62260")
        self.assertEqual(self.context._FFT_freq[FFT_NAME_COMPLEX_RE].Re, "top")
        self.assertEqual(self.context._FFT_freq[FFT_NAME_COMPLEX_RE].Im, "fwd")
        self.assertEqual(self.context._FFT_freq[FFT_NAME_COMPLEX_RE].Im_run, "62261")

        self.assertEqual(self.context._FFT_freq[FFT_NAME_COMPLEX_RE].ws._freq, get_name(FFT_NAME_COMPLEX_RE, MHz))
        self.assertEqual(self.context._FFT_freq[FFT_NAME_COMPLEX_RE].ws._field, get_name(FFT_NAME_COMPLEX_RE, GAUSS))

    def test_get_freq_names_maxEnt(self):
        output = self.context.get_frequency_workspace_names(
            run_list=[["62260", "62261", "62262"]], group=["fwd", "top"], pair=[], frequency_type="MaxEnt", x_label=FIELD
        )
        self.assertEqual(output, [get_name("MUSR62260_raw_data FD; MaxEnt", GAUSS)])

        output2 = self.context.get_frequency_workspace_names(
            run_list=[["62260", "62261", "62262"]], group=["fwd", "top"], pair=[], frequency_type="MaxEnt", x_label=FREQ
        )
        self.assertEqual(output2, [get_name("MUSR62260_raw_data FD; MaxEnt", MHz)])

    def test_get_freq_names_FFT_no_Im_all_parts(self):
        output = self.context.get_frequency_workspace_names(
            run_list=[["62260"]], group=["fwd", "top"], pair=[], frequency_type="FFT All", x_label=FIELD
        )
        self.assertCountEqual(output, [get_name(FFT_NAME_RE_2, GAUSS), get_name(FFT_NAME_RE_MOD, GAUSS)])

    def test_get_freq_names_FFT_no_Im_all_parts_freq(self):
        output = self.context.get_frequency_workspace_names(
            run_list=[["62260"]], group=["fwd", "top"], pair=[], frequency_type="FFT All", x_label=FREQ
        )
        self.assertCountEqual(output, [get_name(FFT_NAME_RE_2, MHz), get_name(FFT_NAME_RE_MOD, MHz)])

    def test_get_freq_names_FFT_no_Im_Re_parts(self):
        output = self.context.get_frequency_workspace_names(
            run_list=[["62260"]], group=["fwd", "top"], pair=[], frequency_type="Re", x_label=FIELD
        )
        self.assertCountEqual(output, [get_name(FFT_NAME_RE_2, GAUSS)])

    def test_get_freq_names_FFT_no_Im_Re_parts_freq(self):
        output = self.context.get_frequency_workspace_names(
            run_list=[["62260"]], group=["fwd", "top"], pair=[], frequency_type="Re", x_label=FREQ
        )
        self.assertCountEqual(output, [get_name(FFT_NAME_RE_2, MHz)])

    def test_get_freq_names_FFT_no_Im_Mod_parts(self):
        output = self.context.get_frequency_workspace_names(
            run_list=[["62260"]], group=["fwd", "top"], pair=[], frequency_type="Mod", x_label=FIELD
        )
        self.assertCountEqual(output, [get_name(FFT_NAME_RE_MOD, GAUSS)])

    def test_get_freq_names_FFT_no_Im_Mod_parts_freq(self):
        output = self.context.get_frequency_workspace_names(
            run_list=[["62260"]], group=["fwd", "top"], pair=[], frequency_type="Mod", x_label=FREQ
        )
        self.assertCountEqual(output, [get_name(FFT_NAME_RE_MOD, MHz)])

    def test_get_freq_names_FFT_no_Im_Im_parts(self):
        output = self.context.get_frequency_workspace_names(
            run_list=[["62260"]], group=["fwd", "top"], pair=[], frequency_type="Im", x_label=FREQ
        )
        self.assertCountEqual(output, [])

    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context.RenameWorkspace")
    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context.convert_to_field")
    def test_get_freq_names_FFT_run(self, field, rename):
        field.side_effect = alg_patch
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_RE, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_IM, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_MOD, Re_run="62260", Re="top", Im_run="62261", Im="fwd")

        output = self.context.get_frequency_workspace_names(
            run_list=[["62261", "62262"]], group=["fwd", "top"], pair=[], frequency_type="FFT All", x_label=FIELD
        )
        self.assertCountEqual(
            output, [get_name(FFT_NAME_COMPLEX_RE, GAUSS), get_name(FFT_NAME_COMPLEX_IM, GAUSS), get_name(FFT_NAME_COMPLEX_MOD, GAUSS)]
        )

    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context.RenameWorkspace")
    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context.convert_to_field")
    def test_get_freq_names_FFT_run_freq(self, field, rename):
        field.side_effect = alg_patch
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_RE, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_IM, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_MOD, Re_run="62260", Re="top", Im_run="62261", Im="fwd")

        output = self.context.get_frequency_workspace_names(
            run_list=[["62261", "62262"]], group=["fwd", "top"], pair=[], frequency_type="FFT All", x_label=FREQ
        )
        self.assertCountEqual(
            output, [get_name(FFT_NAME_COMPLEX_RE, MHz), get_name(FFT_NAME_COMPLEX_IM, MHz), get_name(FFT_NAME_COMPLEX_MOD, MHz)]
        )

    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context.RenameWorkspace")
    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context.convert_to_field")
    def test_get_freq_names_FFT_group(self, field, rename):
        field.side_effect = alg_patch
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_RE, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_IM, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_MOD, Re_run="62260", Re="top", Im_run="62261", Im="fwd")

        output = self.context.get_frequency_workspace_names(
            run_list=[["62260"]], group=["top"], pair=[], frequency_type="FFT All", x_label=FIELD
        )
        self.assertCountEqual(
            output, [get_name(FFT_NAME_COMPLEX_RE, GAUSS), get_name(FFT_NAME_COMPLEX_IM, GAUSS), get_name(FFT_NAME_COMPLEX_MOD, GAUSS)]
        )

    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context.RenameWorkspace")
    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context.convert_to_field")
    def test_get_freq_names_FFT_group_freq(self, field, rename):
        field.side_effect = alg_patch
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_RE, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_IM, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_MOD, Re_run="62260", Re="top", Im_run="62261", Im="fwd")

        output = self.context.get_frequency_workspace_names(
            run_list=[["62260"]], group=["top"], pair=[], frequency_type="FFT All", x_label=FREQ
        )
        self.assertCountEqual(
            output, [get_name(FFT_NAME_COMPLEX_RE, MHz), get_name(FFT_NAME_COMPLEX_IM, MHz), get_name(FFT_NAME_COMPLEX_MOD, MHz)]
        )

    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context.RenameWorkspace")
    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context.convert_to_field")
    def test_get_freq_names_all(self, field, rename):
        field.side_effect = alg_patch
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_RE, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_IM, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_MOD, Re_run="62260", Re="top", Im_run="62261", Im="fwd")

        output = self.context.get_frequency_workspace_names(
            run_list=[[62260], [62261]], group=["fwd"], pair=[], frequency_type="All", x_label=FIELD
        )
        self.assertCountEqual(
            output,
            [
                get_name(FFT_NAME_COMPLEX_RE, GAUSS),
                get_name(FFT_NAME_COMPLEX_IM, GAUSS),
                get_name(FFT_NAME_COMPLEX_MOD, GAUSS),
                get_name(FFT_NAME_RE_2, GAUSS),
                get_name(FFT_NAME_RE_MOD, GAUSS),
                get_name("MUSR62260_raw_data FD; MaxEnt", GAUSS),
            ],
        )

    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context.RenameWorkspace")
    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context.convert_to_field")
    def test_get_freq_names_all_freq(self, field, rename):
        field.side_effect = alg_patch
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_RE, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_IM, Re_run="62260", Re="top", Im_run="62261", Im="fwd")
        self.context.add_FFT(ws_freq_name=FFT_NAME_COMPLEX_MOD, Re_run="62260", Re="top", Im_run="62261", Im="fwd")

        output = self.context.get_frequency_workspace_names(
            run_list=[[62260], [62261]], group=["fwd"], pair=[], frequency_type="All", x_label=FREQ
        )
        self.assertCountEqual(
            output,
            [
                get_name(FFT_NAME_COMPLEX_RE, MHz),
                get_name(FFT_NAME_COMPLEX_IM, MHz),
                get_name(FFT_NAME_COMPLEX_MOD, MHz),
                get_name(FFT_NAME_RE_2, MHz),
                get_name(FFT_NAME_RE_MOD, MHz),
                get_name("MUSR62260_raw_data FD; MaxEnt", MHz),
            ],
        )

    def mock_table(self, name):
        table = mock.Mock()
        type(table).workspace_name = mock.PropertyMock(return_value=name)
        return table

    def test_add_group_phase_table(self):
        groups_2 = self.mock_table("MUSR phase table 2 groups")
        two_groups = self.mock_table("MUSR phases table two groups")
        three_groups = self.mock_table("MUSR phases table 3 groups")

        self.assertEqual(self.context._group_phase_tables, {})

        self.context.add_group_phase_table(groups_2, 2)
        self.context.add_group_phase_table(two_groups, 2)
        self.context.add_group_phase_table(three_groups, 3)

        self.assertEqual(list(self.context._group_phase_tables.keys()), [2, 3])
        self.assertEqual(self.context._group_phase_tables[2], [groups_2, two_groups])
        self.assertEqual(self.context._group_phase_tables[3], [three_groups])

    def test_add_group_phase_table_same_table_twice(self):
        groups_2 = self.mock_table("MUSR phase table 2 groups")
        two_groups = self.mock_table("MUSR phases table two groups")
        three_groups = self.mock_table("MUSR phases table 3 groups")

        self.assertEqual(self.context._group_phase_tables, {})

        self.context.add_group_phase_table(groups_2, 2)
        self.context.add_group_phase_table(two_groups, 2)
        self.context.add_group_phase_table(three_groups, 3)
        self.context.add_group_phase_table(three_groups, 3)

        self.assertEqual(list(self.context._group_phase_tables.keys()), [2, 3])
        self.assertEqual(self.context._group_phase_tables[2], [groups_2, two_groups])
        self.assertEqual(self.context._group_phase_tables[3], [three_groups])

    def test_get_group_phase_table(self):
        groups_2 = self.mock_table("MUSR phase table 2 groups")
        two_groups = self.mock_table("MUSR phases table two groups")
        different_inst = self.mock_table("EMU phases table two groups")
        three_groups = self.mock_table("MUSR phases table 3 groups")

        self.assertEqual(self.context._group_phase_tables, {})

        self.context.add_group_phase_table(groups_2, 2)
        self.context.add_group_phase_table(two_groups, 2)
        self.context.add_group_phase_table(different_inst, 2)
        self.context.add_group_phase_table(three_groups, 3)

        self.assertEqual(self.context.get_group_phase_tables(2, "MUSR"), [groups_2.workspace_name, two_groups.workspace_name])
        self.assertEqual(self.context.get_group_phase_tables(3, "MUSR"), [three_groups.workspace_name])
        self.assertEqual(self.context.get_group_phase_tables(2, "EMU"), [different_inst.workspace_name])

    def test_unit(self):
        self.context.x_label = FIELD
        self.assertEqual(self.context.unit(), GAUSS)

        self.context.x_label = FREQ
        self.assertEqual(self.context.unit(), MHz)

    def test_get_ws_name(self):
        self.context.x_label = FIELD
        name = "unit test"
        self.assertEqual(self.context.get_ws_name(name), get_name(name, GAUSS))

        self.context.x_label = FREQ
        self.assertEqual(self.context.get_ws_name(name), get_name(name, MHz))

    def test_switch_units_in_name_freq_to_field(self):
        self.assertEqual(self.context.switch_units_in_name(f"unit {MHz} test"), f"unit {GAUSS} test")

    def test_switch_units_in_name_field_to_freq(self):
        self.assertEqual(self.context.switch_units_in_name(f"unit {GAUSS} test"), f"unit {MHz} test")

    def test_switch_units_in_name_with_None(self):
        self.assertEqual(self.context.switch_units_in_name(None), None)

    def test_range(self):
        self.context.x_label = FIELD
        self.assertEqual(self.context.range(), [0.0, 1000.0])

        self.context.x_label = FREQ
        self.assertEqual(self.context.range(), [0.0, 30.0])


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
