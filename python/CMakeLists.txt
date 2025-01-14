# SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause

set(bindings_library "KStatusNotifierItem")

set(wrapped_header ${CMAKE_CURRENT_SOURCE_DIR}/bindings.h)
set(typesystem_file ${CMAKE_CURRENT_SOURCE_DIR}/bindings.xml)

set(generated_sources
    ${CMAKE_CURRENT_BINARY_DIR}/KStatusNotifierItem/kstatusnotifieritem_module_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/KStatusNotifierItem/kstatusnotifieritem_wrapper.cpp
)

ecm_generate_python_bindings(
    PACKAGE_NAME ${bindings_library}
    VERSION ${KF_VERSION}
    WRAPPED_HEADER ${wrapped_header}
    TYPESYSTEM ${typesystem_file}
    GENERATED_SOURCES ${generated_sources}
    DEPENDENCIES KF6::StatusNotifierItem
    QT_VERSION ${REQUIRED_QT_VERSION}
    HOMEPAGE_URL "https://invent.kde.org/frameworks/kstatusnotifieritem"
    ISSUES_URL "https://bugs.kde.org/describecomponents.cgi?product=frameworks-kstatusnotifieritem"
    AUTHOR "The KDE Community"
    README ${CMAKE_SOURCE_DIR}/README.md
)

target_link_libraries(${bindings_library} PRIVATE KF6::StatusNotifierItem)

execute_process(COMMAND ${Python_EXECUTABLE} -Esc "import sysconfig; print(sysconfig.get_path('platlib', vars={'platbase': '${CMAKE_INSTALL_PREFIX}', 'base': '${CMAKE_INSTALL_PREFIX}'}))" OUTPUT_VARIABLE sysconfig_output)
string(STRIP ${sysconfig_output} PYTHON_INSTALL_DIR)

install(TARGETS ${bindings_library} LIBRARY DESTINATION "${PYTHON_INSTALL_DIR}")

