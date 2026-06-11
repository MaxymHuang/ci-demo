# Headless domain library: must stay free of Qt6::Widgets so unit tests can
# run without a display.
qt_add_library(xri_core STATIC
    Algorithm.h
    AlgorithmRegistry.h AlgorithmRegistry.cpp
    Algorithms.h Algorithms.cpp
    InspectionEngine.h InspectionEngine.cpp
    InspectionPlan.h InspectionPlan.cpp
    InspectionSession.h InspectionSession.cpp
    ReportExporter.h ReportExporter.cpp
    Roi.h Roi.cpp
    RoiListModel.h RoiListModel.cpp
    ScanSimulator.h ScanSimulator.cpp
    SyntheticXraySource.h SyntheticXraySource.cpp
)

target_link_libraries(xri_core PUBLIC Qt6::Core Qt6::Gui)
target_include_directories(xri_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
