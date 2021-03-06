########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.8.9)
project(PothosGui)
enable_language(CXX)

find_package(Pothos CONFIG REQUIRED)

########################################################################
# Local includes
########################################################################
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${Pothos_INCLUDE_DIRS})

########################################################################
# QT4 devel setup
########################################################################
#find_package(Qt4 COMPONENTS QtCore QtGui)
#if(NOT QT4_FOUND)
#    return()
#endif()
#include(${QT_USE_FILE})
#list(APPEND Pothos_LIBRARIES ${QT_LIBRARIES})

########################################################################
# QT5 devel setup
########################################################################
#http://www.kdab.com/using-cmake-with-qt-5/
# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Widgets finds its own dependencies.
find_package(Qt5Widgets)

if(NOT Qt5Widgets_FOUND)
    return()
endif()
include_directories(${Qt5Widgets_INCLUDE_DIRS})
add_definitions(${Qt5Widgets_DEFINITIONS})
list(APPEND Pothos_LIBRARIES ${Qt5Widgets_LIBRARIES})

# Widgets finds its own dependencies.
find_package(Qt5Concurrent)

if(NOT Qt5Concurrent_FOUND)
    return()
endif()
include_directories(${Qt5Concurrent_INCLUDE_DIRS})
add_definitions(${Qt5Concurrent_DEFINITIONS})
list(APPEND Pothos_LIBRARIES ${Qt5Concurrent_LIBRARIES})

########################################################################
# Color picker library - LGPL separate library
########################################################################
include_directories(qtcolorpicker/src)
add_library(QtColorPicker SHARED qtcolorpicker/src/qtcolorpicker.cpp)
set_target_properties(QtColorPicker PROPERTIES DEFINE_SYMBOL "QT_QTCOLORPICKER_EXPORT")
target_link_libraries(QtColorPicker ${Pothos_LIBRARIES})
install(TARGETS QtColorPicker
    LIBRARY DESTINATION lib${LIB_SUFFIX} COMPONENT pothos_gui # .so file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} COMPONENT pothos_gui # .lib file
    RUNTIME DESTINATION bin              COMPONENT pothos_gui # .dll file
)
list(APPEND Pothos_LIBRARIES QtColorPicker)

########################################################################
# GUI Main App
########################################################################
set(SOURCES
    PothosGui.cpp
    PothosGuiUtils.cpp
    MainWindow.cpp

    ColorUtils/ColorUtils.cpp
    ColorUtils/ColorsDialog.cpp

    PropertiesPanel/PropertiesPanelDock.cpp
    PropertiesPanel/ConnectionPropertiesPanel.cpp
    PropertiesPanel/BlockPropertiesPanel.cpp
    PropertiesPanel/BreakerPropertiesPanel.cpp

    MessageWindow/MessageWindowDock.cpp
    MessageWindow/LoggerDisplay.cpp
    MessageWindow/LoggerChannel.cpp

    BlockTree/BlockTreeDock.cpp
    BlockTree/BlockTreeWidget.cpp
    BlockTree/BlockTreeWidgetItem.cpp
    BlockTree/BlockCache.cpp

    AffinitySupport/AffinityZoneEditor.cpp
    AffinitySupport/AffinityZonesMenu.cpp
    AffinitySupport/AffinityZonesComboBox.cpp
    AffinitySupport/AffinityZonesDock.cpp
    AffinitySupport/CpuSelectionWidget.cpp

    HostExplorer/PluginModuleTree.cpp
    HostExplorer/PluginRegistryTree.cpp
    HostExplorer/SystemInfoTree.cpp
    HostExplorer/HostSelectionTable.cpp
    HostExplorer/HostExplorerDock.cpp

    GraphEditor/GraphState.cpp
    GraphEditor/GraphEditorTabs.cpp
    GraphEditor/GraphEditor.cpp
    GraphEditor/GraphEditorSerialization.cpp
    GraphEditor/GraphEditorDeserialization.cpp
    GraphEditor/GraphEditorRenderedDialog.cpp
    GraphEditor/GraphDraw.cpp
    GraphEditor/GraphDrawSelection.cpp
    GraphEditor/GraphActionsDock.cpp

    GraphObjects/GraphEndpoint.cpp
    GraphObjects/GraphObject.cpp
    GraphObjects/GraphBlock.cpp
    GraphObjects/GraphBlockUpdate.cpp
    GraphObjects/GraphBreaker.cpp
    GraphObjects/GraphConnection.cpp
    GraphObjects/GraphWidget.cpp
    GraphObjects/GraphWidgetContainer.cpp

    EvalEngine/EvalEngine.cpp
    EvalEngine/EvalEngineImpl.cpp
    EvalEngine/BlockEval.cpp
    EvalEngine/ThreadPoolEval.cpp
    EvalEngine/EnvironmentEval.cpp
    EvalEngine/TopologyEval.cpp
    EvalEngine/TopologyTraversal.cpp
)
add_executable(PothosGui ${SOURCES})
target_link_libraries(PothosGui Pothos ${Pothos_LIBRARIES})
install(
    TARGETS PothosGui
    RUNTIME DESTINATION bin
    COMPONENT pothos_gui
)

install(
    DIRECTORY icons
    DESTINATION share/Pothos
    COMPONENT pothos_gui
)

########################################################################
# Edit widgets module
########################################################################
add_subdirectory(EditWidgets)
