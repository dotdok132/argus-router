#ifndef STYLES_H
#define STYLES_H

#include <QString>

namespace Styles {

inline QString darkTheme() {
    return QString(R"(
        QWidget {
            background-color: #1e1e1e;
            color: #cccccc;
            font-family: 'Inter', 'Segoe UI', system-ui, sans-serif;
            font-size: 13px;
        }

        QMainWindow {
            background-color: #1e1e1e;
        }

        /* Top Header Bar */
        #HeaderBar {
            background-color: #252526;
            border-bottom: 1px solid #3c3c3c;
            padding: 8px 16px;
        }

        #AppTitle {
            font-size: 14px;
            font-weight: 700;
            color: #ffffff;
            letter-spacing: 0.3px;
        }

        #StatusBadge {
            background-color: #1b382b;
            border: 1px solid #23543d;
            border-radius: 4px;
            color: #4ec9b0;
            font-weight: 600;
            font-size: 11px;
            font-family: monospace;
            padding: 3px 8px;
        }

        /* Tab Widget Styling */
        QTabWidget::pane {
            border: 1px solid #3c3c3c;
            background-color: #1e1e1e;
            top: -1px;
        }

        QTabBar::tab {
            background-color: #2d2d2d;
            border: 1px solid #3c3c3c;
            border-bottom: none;
            color: #969696;
            padding: 7px 16px;
            margin-right: 2px;
            font-size: 12px;
            font-weight: 500;
        }

        QTabBar::tab:selected {
            background-color: #1e1e1e;
            color: #ffffff;
            border-top: 2px solid #007acc;
        }

        QTabBar::tab:hover:!selected {
            background-color: #333333;
            color: #cccccc;
        }

        /* Group Boxes */
        QGroupBox {
            background-color: #252526;
            border: 1px solid #3c3c3c;
            border-radius: 4px;
            margin-top: 10px;
            padding-top: 14px;
            font-weight: 600;
            font-size: 12px;
            color: #cccccc;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 10px;
            padding: 0 4px;
            color: #569cd6;
        }

        /* Progress Bars */
        QProgressBar {
            background-color: #333333;
            border: 1px solid #3c3c3c;
            border-radius: 3px;
            height: 14px;
            text-align: center;
            color: #ffffff;
            font-size: 10px;
            font-weight: 600;
        }

        QProgressBar::chunk {
            background-color: #007acc;
            border-radius: 2px;
        }

        /* Table Styling - Explicit Dark Alternating Colors */
        QTableWidget {
            background-color: #1e1e1e;
            alternate-background-color: #252526;
            border: 1px solid #3c3c3c;
            gridline-color: #2d2d2d;
            border-radius: 4px;
            selection-background-color: #04395e;
            selection-color: #ffffff;
            color: #cccccc;
            font-family: 'Inter', 'Segoe UI', system-ui, sans-serif;
            font-size: 12px;
        }

        QTableWidget::item {
            padding: 4px 8px;
        }

        QTableWidget::item:selected {
            background-color: #04395e;
            color: #ffffff;
        }

        QHeaderView::section {
            background-color: #2d2d2d;
            color: #969696;
            padding: 8px 10px;
            border: none;
            border-right: 1px solid #3c3c3c;
            border-bottom: 1px solid #3c3c3c;
            font-weight: 600;
            font-size: 11px;
            font-family: 'Inter', 'Segoe UI', system-ui, sans-serif;
        }

        /* Action Cell Widgets */
        #ActionCellWidget {
            background-color: transparent;
        }

        /* Input Controls */
        QLineEdit, QSpinBox, QComboBox {
            background-color: #3c3c3c;
            border: 1px solid #555555;
            border-radius: 3px;
            padding: 5px 8px;
            color: #ffffff;
            font-size: 12px;
        }

        QLineEdit:focus, QSpinBox:focus, QComboBox:focus {
            border-color: #007acc;
        }

        /* Buttons */
        QPushButton {
            background-color: #3c3c3c;
            border: 1px solid #555555;
            color: #cccccc;
            padding: 5px 14px;
            border-radius: 3px;
            font-size: 12px;
            font-weight: 500;
        }

        QPushButton:hover {
            background-color: #464646;
            color: #ffffff;
            border-color: #007acc;
        }

        QPushButton#TableGhostAccent {
            background-color: transparent;
            border: 1px solid #1177bb;
            color: #4fc1ff;
            padding: 3px 10px;
            border-radius: 4px;
            font-size: 11.5px;
            font-weight: 500;
        }

        QPushButton#TableGhostAccent:hover {
            background-color: #0e4f7a;
            color: #ffffff;
            border-color: #569cd6;
        }

        QPushButton#TableGhostButton {
            background-color: transparent;
            border: 1px solid #3c3c3c;
            color: #cccccc;
            padding: 3px 10px;
            border-radius: 4px;
            font-size: 11.5px;
            font-weight: 500;
        }

        QPushButton#TableGhostButton:hover {
            background-color: #2a2d2e;
            color: #ffffff;
            border-color: #007acc;
        }

        QPushButton#TableGhostDanger {
            background-color: transparent;
            border: 1px solid #5a2626;
            color: #f14c4c;
            padding: 3px 10px;
            border-radius: 4px;
            font-size: 11.5px;
            font-weight: 500;
        }

        QPushButton#TableGhostDanger:hover {
            background-color: #4a1c1c;
            color: #ffffff;
            border-color: #f14c4c;
        }

        QPushButton#PrimaryButton {
            background-color: #0e639c;
            border: 1px solid #1177bb;
            color: #ffffff;
        }

        QPushButton#PrimaryButton:hover {
            background-color: #1177bb;
        }

        QPushButton#DangerButton {
            background-color: #481a1a;
            border: 1px solid #702626;
            color: #f14c4c;
            padding: 4px 12px;
            border-radius: 3px;
            font-size: 11.5px;
            font-weight: 500;
        }

        QPushButton#DangerButton:hover {
            background-color: #702626;
            color: #ffffff;
        }

        QCheckBox {
            color: #cccccc;
            spacing: 6px;
        }

        QCheckBox::indicator {
            width: 14px;
            height: 14px;
            background-color: #3c3c3c;
            border: 1px solid #555555;
            border-radius: 2px;
        }

        QCheckBox::indicator:checked {
            background-color: #007acc;
            border-color: #007acc;
        }
    )");
}

} // namespace Styles

#endif // STYLES_H
