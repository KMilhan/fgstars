#!/usr/bin/env python3
import sys
import xml.etree.ElementTree as ET

def check_accessibility(ui_file):
    try:
        tree = ET.parse(ui_file)
        root = tree.getroot()
    except Exception as e:
        print(f"Error parsing {ui_file}: {e}")
        return False

    interactive_classes = ['QPushButton', 'QCheckBox', 'QRadioButton', 'QComboBox', 'QLineEdit', 'QSlider', 'QSpinBox', 'QToolButton']
    widgets = root.findall('.//widget')
    
    missing_accessibility = False
    
    for widget in widgets:
        w_class = widget.get('class', '')
        if w_class in interactive_classes:
            w_name = widget.get('name', 'unnamed')
            has_acc_name = widget.find('.//property[@name="accessibleName"]') is not None
            has_acc_desc = widget.find('.//property[@name="accessibleDescription"]') is not None
            
            if not (has_acc_name or has_acc_desc):
                print(f"[{ui_file}] Widget '{w_name}' (class {w_class}) is missing accessibleName or accessibleDescription.")
                missing_accessibility = True
                
    return not missing_accessibility

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: check_ui_accessibility.py <ui_file1> [ui_file2 ...]")
        sys.exit(0) # Don't fail if no files are provided
        
    all_passed = True
    for file in sys.argv[1:]:
        if file.endswith('.ui'):
            if not check_accessibility(file):
                all_passed = False
                
    if not all_passed:
        print("\nERROR: Accessibility check failed. Please add accessibleName properties to the widgets listed above.")
        # Currently we just warn so we don't break existing PRs until the entire codebase is fixed.
        # Once the backlog is cleared, change sys.exit(0) to sys.exit(1)
        print("WARNING: Treating as a warning during the transition period.")
        sys.exit(0)
    else:
        print("Accessibility check passed.")
        sys.exit(0)
