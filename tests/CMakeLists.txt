# Every test runs on the offscreen platform plugin so ctest behaves the same
# on developer machines and headless CI runners.
function(add_xri_test name)
    qt_add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE xri_core Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

add_xri_test(tst_syntheticxray core/tst_syntheticxray.cpp)
add_xri_test(tst_scansimulator core/tst_scansimulator.cpp)
add_xri_test(tst_inspectionplan core/tst_inspectionplan.cpp)
add_xri_test(tst_algorithms core/tst_algorithms.cpp)
add_xri_test(tst_exporter core/tst_exporter.cpp)

# UI smoke test: drives the real MainWindow headlessly.
add_xri_test(tst_smoke_mainwindow app/tst_smoke_mainwindow.cpp)
target_link_libraries(tst_smoke_mainwindow PRIVATE xri_app_lib Qt6::Widgets)
