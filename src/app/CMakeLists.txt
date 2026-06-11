# UI classes live in a static lib so the smoke test can link them directly.
qt_add_library(xri_app_lib STATIC
    MainWindow.h MainWindow.cpp
    pages/AlgorithmsPage.h pages/AlgorithmsPage.cpp
    pages/ScanPage.h pages/ScanPage.cpp
    pages/SetupPage.h pages/SetupPage.cpp
    pages/ViewerPage.h pages/ViewerPage.cpp
    widgets/ImageView.h widgets/ImageView.cpp
    widgets/RoiEditorView.h widgets/RoiEditorView.cpp
    widgets/RoiGraphicsItem.h widgets/RoiGraphicsItem.cpp
)

target_link_libraries(xri_app_lib PUBLIC xri_core Qt6::Widgets)
target_include_directories(xri_app_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_resources(xri_app_lib "app_resources"
    PREFIX "/"
    FILES resources/dark.qss
)

qt_add_executable(xri_demo WIN32 main.cpp)
target_link_libraries(xri_demo PRIVATE xri_app_lib)
