# Tests for PythonWorkflowAlgorithms

set(TEST_PY_FILES
    AddSampleLogMultipleTest.py
    ApplyPaalmanPingsCorrectionTest.py
    BayesQuasiTest.py
    BayesStretchTest.py
    CalculateMonteCarloAbsorptionTest.py
    CorelliPowderCalibrationCreateTest.py
    DetectorFloodWeightingTest.py
    DirectILLApplySelfShieldingTest.py
    DirectILLCollectDataTest.py
    DirectILLDiagnosticsTest.py
    DirectILLIntegrateVanadiumTest.py
    DirectILLReductionTest.py
    DirectILLSelfShieldingTest.py
    ElasticWindowMultipleTest.py
    EnergyWindowScanTest.py
    FitGaussianPeaksTest.py
    FindPeakAutomaticTest.py
    FlatPlatePaalmanPingsCorrectionTest.py
    IndirectAnnulusAbsorptionTest.py
    IndirectAnnulusAbsorption2Test.py
    IndirectCalibrationTest.py
    IndirectCylinderAbsorptionTest.py
    IndirectCylinderAbsorption2Test.py
    IndirectDiffScanTest.py
    IndirectFlatPlateAbsorptionTest.py
    IndirectFlatPlateAbsorption2Test.py
    IndirectILLEnergyTransferTest.py
    IndirectILLReductionFWSTest.py
    IndirectILLReductionQENSTest.py
    IndirectReplaceFitResultTest.py
    IndirectResolutionTest.py
    IndirectSampleChangerTest.py
    IndirectTransmissionMonitorTest.py
    IndirectTwoPeakFitTest.py
    IqtFitMultipleTest.py
    IqtFitSequentialTest.py
    ISISIndirectDiffractionReductionTest.py
    ISISIndirectEnergyTransferTest.py
    ISISIndirectEnergyTransferWrapperTest.py
    D7YIGPositionCalibrationTest.py
    D7AbsoluteCrossSectionsTest.py
    LoadWANDTest.py
    MDNormSCDPreprocessIncoherentTest.py
    MolDynTest.py
    MSDFitTest.py
    OSIRISDiffractionReductionTest.py
    PaalmanPingsMonteCarloAbsorptionTest.py
    PolDiffILLReductionTest.py
    PowderILLDetectorScanTest.py
    PowderILLParameterScanTest.py
    ReflectometryILLAutoProcessTest.py
    ReflectometryILL_commonTest.py
    ReflectometryILLConvertToQTest.py
    ReflectometryILLPolarizationCorTest.py
    ReflectometryILLPreprocessTest.py
    ReflectometryILLSumForegroundTest.py
    ReflectometryISISLoadAndProcessTest.py
    ResNorm2Test.py
    SANSDarkRunBackgroundCorrectionTest.py
    SANSFitShiftScaleTest.py
    SANSILLAutoProcessTest.py
    SANSILLIntegrationTest.py
    SANSILLReductionTest.py
    SANSILLParameterScanTest.py
    SANSMaskTest.py
    SANSStitchTest.py
    SavePlot1DTest.py
    SaveVulcanGSSTest.py
    SimpleShapeMonteCarloAbsorptionTest.py
    SimulatedDensityOfStatesTest.py
    SofQWMomentsTest.py
    SofQWMomentsScanTest.py
    SwapWidthsTest.py
    TimeSliceTest.py
    TOSCABankCorrectionTest.py
    TransformToIqtTest.py
    VesuvioDiffractionReductionTest.py
    WANDPowderReductionTest.py)
check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test name=PythonWorkflowAlgorithms
set(PYUNITTEST_PYTHONPATH_EXTRA
    ${PYTHONINTERFACE_PLUGINS_DIR}/algorithms/WorkflowAlgorithms)
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.WorkflowAlgorithms
                    ${TEST_PY_FILES})
add_subdirectory(sans)
