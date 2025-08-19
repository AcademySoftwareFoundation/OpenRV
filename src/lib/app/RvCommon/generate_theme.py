#!/usr/bin/env python3
# Copyright (c) 2025 Autodesk.
#
# CONFIDENTIAL AND PROPRIETARY
#
# This work is provided "AS IS" and subject to the ShotGrid Pipeline Toolkit
# Source Code License included in this distribution package. See LICENSE.
# By accessing, using, copying or modifying this work you indicate your
# agreement to the ShotGrid Pipeline Toolkit Source Code License. All rights
# not expressly granted therein are reserved by ShotGrid Software Inc.
"""
Theme Generator for OpenRV
Generates platform-specific stylesheets from templates with interactive variable support

Automatically detects platform and uses:
    - macOS: rv_mac_dark.qss.template
    - Linux: rv_linux_dark.qss.template

Features:
    - Interactive variable collection for missing values
    - Command-line variable overrides
    - Platform auto-detection
    - Variable validation and prompting

Usage Examples:
    # Use config file (auto-detects platform template)
    python generate_theme.py --config rv_theme_variables.conf --output rv_dark.qss
    
    # Override variables from command line (will prompt for missing ones)
    python generate_theme.py --config rv_theme_variables.conf --output rv_blue.qss --set ACCENT_PRIMARY=rgb(0,100,200) --set ACCENT_SECONDARY=rgb(50,150,255)
    
    # No config file (will prompt for all required variables)
    python generate_theme.py --output rv_custom.qss --set PRIMARY_BACKGROUND=rgb(40,40,40)
"""

import sys
import os
import argparse
import platform

def get_platform_template():
    """Get the appropriate template file for the current platform"""
    system = platform.system().lower()
    template_name = 'rv_linux_dark.qss.template' if system == 'linux' else 'rv_mac_dark.qss.template'
    
    # Check if we're being run from OpenRV root directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    current_dir = os.getcwd()
    
    # If script is in src/lib/app/RvCommon and we're not in that directory
    if os.path.basename(script_dir) == 'RvCommon' and current_dir != script_dir:
        # Check if current directory looks like OpenRV root (has src/lib/app/RvCommon)
        rvcommon_path = os.path.join(current_dir, 'src', 'lib', 'app', 'RvCommon')
        if os.path.exists(rvcommon_path):
            template_path = os.path.join('src', 'lib', 'app', 'RvCommon', template_name)
            if os.path.exists(template_path):
                return template_path
    
    return template_name

def adjust_output_path(output_file):
    """Adjust output path to RvCommon directory if running from OpenRV root"""
    # Check if we're being run from OpenRV root directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    current_dir = os.getcwd()
    
    # If script is in src/lib/app/RvCommon and we're not in that directory
    if os.path.basename(script_dir) == 'RvCommon' and current_dir != script_dir:
        # Check if current directory looks like OpenRV root (has src/lib/app/RvCommon)
        rvcommon_path = os.path.join(current_dir, 'src', 'lib', 'app', 'RvCommon')
        if os.path.exists(rvcommon_path):
            # Only adjust if output path is just a filename (no directory)
            if not os.path.dirname(output_file):
                adjusted_path = os.path.join('src', 'lib', 'app', 'RvCommon', output_file)
                print(f"Placing output in RvCommon directory: {adjusted_path}")
                return adjusted_path
    
    return output_file

def adjust_config_path(config_file):
    """Adjust config file path to RvCommon directory if running from OpenRV root"""
    if not config_file:
        return config_file
        
    # Check if we're being run from OpenRV root directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    current_dir = os.getcwd()
    
    # If script is in src/lib/app/RvCommon and we're not in that directory
    if os.path.basename(script_dir) == 'RvCommon' and current_dir != script_dir:
        # Check if current directory looks like OpenRV root (has src/lib/app/RvCommon)
        rvcommon_path = os.path.join(current_dir, 'src', 'lib', 'app', 'RvCommon')
        if os.path.exists(rvcommon_path):
            # Only adjust if config path is just a filename (no directory)
            if not os.path.dirname(config_file):
                adjusted_path = os.path.join('src', 'lib', 'app', 'RvCommon', config_file)
                # Check if the adjusted path exists, if not, try the original
                if os.path.exists(adjusted_path):
                    print(f"Using config from RvCommon directory: {adjusted_path}")
                    return adjusted_path
    
    return config_file

def load_variables_from_config(config_file):
    """Load variables from a config file with validation"""
    variables = {}
    invalid_count = 0
    
    if not config_file or not os.path.exists(config_file):
        return variables
        
    try:
        with open(config_file, 'r', encoding='utf-8') as f:
            for line_num, line in enumerate(f, 1):
                original_line = line
                line = line.strip()
                if line and not line.startswith('#'):
                    if '=' in line:
                        key, value = line.split('=', 1)
                        key = key.strip()
                        value = value.strip()
                        
                        # Check for empty key or value
                        if not key:
                            print(f"WARNING: Empty variable name in {config_file} line {line_num}: {original_line.strip()}")
                            invalid_count += 1
                            continue
                        
                        if not value:
                            print(f"WARNING: Empty value in {config_file} line {line_num}: {key} = (empty)")
                            invalid_count += 1
                            continue
                        
                        # Check for duplicate variables
                        if key in variables:
                            print(f"WARNING: Duplicate variable '{key}' in {config_file} line {line_num} (overwriting previous value)")
                        
                        # Validate the value
                        if validate_css_value(key, value):
                            variables[key] = value
                        else:
                            print(f"WARNING: Skipping invalid value in {config_file} line {line_num}: {key} = {value}")
                            example = get_example_value(key)
                            print(f"         Example: {key} = {example}")
                            print(f"         Variable will remain as {{{{{{key}}}}}} placeholder")
                            invalid_count += 1
                            # Skip invalid values - don't include them
                    else:
                        print(f"WARNING: Malformed line in {config_file} line {line_num}: {original_line.strip()}")
                        print(f"         Expected format: VARIABLE = value")
                        invalid_count += 1
    except UnicodeDecodeError as e:
        print(f"ERROR: Config file {config_file} has encoding issues: {e}")
        print("       Please save the file as UTF-8 encoding")
        return {}
    except Exception as e:
        print(f"ERROR: Failed to read config file {config_file}: {e}")
        return {}
    
    if invalid_count > 0:
        print(f"\nFound {invalid_count} invalid value(s) in config file. Please fix them to ensure proper CSS.")
    
    return variables

def validate_css_value(var_name, value):
    """Validate CSS values to prevent broken stylesheets"""
    if not value or value.lower() == 'quit':
        return True
    
    # Color-related variables should be valid CSS colors
    color_vars = {'PRIMARY_BACKGROUND', 'SECONDARY_BACKGROUND', 'TERTIARY_BACKGROUND',
                  'ACCENT_PRIMARY', 'ACCENT_SECONDARY', 'TEXT_PRIMARY', 'TEXT_SECONDARY', 
                  'TEXT_BRIGHT', 'TEXT_DISABLED', 'BORDER_DARK', 'BORDER_LIGHT', 
                  'BORDER_MEDIUM', 'BUTTON_BORDER', 'BUTTON_BORDER_DISABLED',
                  'BUTTON_HOVER', 'BUTTON_PRESSED', 'CONTROL_BACKGROUND', 
                  'CONTROL_HANDLE', 'CONTROL_HANDLE_HOVER', 'INPUT_BACKGROUND',
                  'SELECTION_BACKGROUND', 'SELECTION_TEXT', 'SESSION_ITEM_BORDER'}
    
    if var_name in color_vars:
        # Check for valid CSS color formats
        import re
        
        # RGB format: rgb(r, g, b) with proper range validation
        rgb_match = re.match(r'^rgb\(\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(\d{1,3})\s*\)$', value)
        if rgb_match:
            r, g, b = map(int, rgb_match.groups())
            if 0 <= r <= 255 and 0 <= g <= 255 and 0 <= b <= 255:
                return True
            else:
                return False
        
        # Hex format: #rrggbb or #rgb
        if re.match(r'^#([0-9a-fA-F]{3}|[0-9a-fA-F]{6})$', value):
            return True
        
        # Named colors (basic validation)
        named_colors = {'black', 'white', 'red', 'green', 'blue', 'yellow', 'cyan', 
                       'magenta', 'gray', 'grey', 'transparent', 'none'}
        if value.lower() in named_colors:
            return True
        
        return False
    
    # Non-color variables: just check it's not obviously invalid
    if len(value.strip()) == 0:
        return False
    
    return True

def get_example_value(var_name):
    """Get example values for variables to help users"""
    examples = {
        'PRIMARY_BACKGROUND': 'rgb(34,34,34)',
        'ACCENT_PRIMARY': 'rgb(7,75,120)', 
        'TEXT_PRIMARY': 'rgb(200,200,200)',
        'BORDER_DARK': 'black',
        'BORDER_LIGHT': 'rgb(140,140,140)'
    }
    return examples.get(var_name, 'rgb(100,100,100)')

def collect_missing_variables(missing_vars):
    """Interactive collection of missing variables with validation"""
    print("\nMissing variables detected. Please provide values:")
    print("('quit' to abort, 'help <VAR>' for examples, Ctrl+C to cancel)")
    
    collected_vars = {}
    try:
        for var_name in sorted(missing_vars):
            while True:
                try:
                    prompt = f"{var_name} = "
                    value = input(prompt).strip()
                    
                    if value.lower() == 'quit':
                        print("Aborted by user.")
                        import sys
                        sys.exit(1)
                    elif value.lower().startswith('help'):
                        example = get_example_value(var_name)
                        print(f"Example for {var_name}: {example}")
                        continue
                    elif value == '':
                        print("Value cannot be empty. Please provide a valid CSS value.")
                        continue
                    else:
                        # Validate the value
                        if validate_css_value(var_name, value):
                            collected_vars[var_name] = value
                            print(f"Set: {var_name} = {value}")
                            break
                        else:
                            example = get_example_value(var_name)
                            print(f"Invalid CSS value '{value}'. Example: {example}")
                            continue
                            
                except KeyboardInterrupt:
                    print("\n\nOperation cancelled by user (Ctrl+C)")
                    import sys
                    sys.exit(1)
                except EOFError:
                    print("\n\nUnexpected end of input")
                    import sys
                    sys.exit(1)
                    
    except Exception as e:
        print(f"\nError during variable collection: {e}")
        import sys
        sys.exit(1)
    
    return collected_vars

def process_template(template_file, variables):
    """Process template file replacing {{variable}} placeholders"""
    if not os.path.exists(template_file):
        raise FileNotFoundError(f"Template file '{template_file}' not found")
        
    try:
        with open(template_file, 'r', encoding='utf-8') as f:
            content = f.read()
    except UnicodeDecodeError as e:
        raise ValueError(f"Template file {template_file} has encoding issues: {e}. Please save as UTF-8.")
    except Exception as e:
        raise ValueError(f"Failed to read template file {template_file}: {e}")
        
    if not content.strip():
        raise ValueError(f"Template file {template_file} is empty")
    
    # Find all template variables in the file
    import re
    template_vars = set(re.findall(r'\{\{(\w+)\}\}', content))
    
    # Check for variables in config that don't exist in template
    extra_vars = set(variables.keys()) - template_vars
    if extra_vars:
        print(f"\nWARNING: Found {len(extra_vars)} variables in config that don't exist in template:")
        for var in sorted(extra_vars):
            print(f"  {var} = {variables[var]}")
        print("These variables will be ignored.\n")
    
    # Check for missing variables
    missing_vars = template_vars - set(variables.keys())
    if missing_vars:
        print(f"\nFound {len(missing_vars)} missing variables: {', '.join(sorted(missing_vars))}")
        
        # Collect missing variables interactively
        new_vars = collect_missing_variables(missing_vars)
        
        # Add collected variables (this will overwrite existing ones if provided)
        variables.update(new_vars)
        
        if new_vars:
            print(f"\nCollected {len(new_vars)} additional variables.")
    
    # Replace all {{VARIABLE}} placeholders (only for variables that exist in template)
    for var_name in template_vars:
        if var_name in variables:
            placeholder = f"{{{{{var_name}}}}}"
            content = content.replace(placeholder, variables[var_name])
    
    return content

def parse_variable_override(var_string):
    """Parse a variable override in format KEY=VALUE"""
    if '=' not in var_string:
        raise ValueError(f"Invalid variable format: {var_string}. Use KEY=VALUE")
    key, value = var_string.split('=', 1)
    return key.strip(), value.strip()

def generate_theme(config_file, output_file, variable_overrides=None):
    """Generate a theme file"""
    print(f"Generating theme: {output_file}")
    
    # Adjust output path if running from OpenRV root
    output_file = adjust_output_path(output_file)
    
    # Adjust config path if running from OpenRV root
    config_file = adjust_config_path(config_file)
    
    # Automatically select template based on platform
    template_file = get_platform_template()
    print(f"Platform detected: {platform.system()}")
    print(f"Using template: {template_file}")
    
    # Load base variables from config
    variables = load_variables_from_config(config_file)
    if config_file and variables:
        print(f"Loaded {len(variables)} variables from {config_file}")
    elif config_file:
        print(f"Config file {config_file} not found or empty")
    
    # Apply command-line overrides (these can overwrite config variables)
    if variable_overrides:
        for override in variable_overrides:
            key, value = parse_variable_override(override)
            if key in variables:
                print(f"Overwriting: {key} = {value} (was: {variables[key]})")
            else:
                print(f"Setting: {key} = {value}")
            variables[key] = value
    
    # Process template (will prompt for missing variables interactively)
    print(f"Processing template: {template_file}")
    result = process_template(template_file, variables)
    
    # Write output
    os.makedirs(os.path.dirname(output_file) if os.path.dirname(output_file) else '.', exist_ok=True)
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(result)
    
    print(f"Generated: {output_file}")
    return output_file

def main():
    parser = argparse.ArgumentParser(
        description='Generate an OpenRV theme from template (auto-detects platform)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    
    parser.add_argument('--config', '-c',
                        help='Variables config file')
    
    parser.add_argument('--output', '-o', required=True,
                        help='Output stylesheet file')
    
    parser.add_argument('--set', '-s', action='append', dest='variables',
                        help='Set variable (format: KEY=VALUE). Can be used multiple times')
    
    parser.add_argument('--list-variables',
                        help='List all variables found in a config file')
    
    args = parser.parse_args()
    
    try:
        # Utility command
        if args.list_variables:
            variables = load_variables_from_config(args.list_variables)
            if variables:
                print(f"Variables in {args.list_variables}:")
                for key, value in variables.items():
                    print(f"  {key} = {value}")
            else:
                print(f"No variables found in {args.list_variables}")
            return 0
        
        # Generate theme
        result = generate_theme(args.config, args.output, args.variables)
        if result:
            print("Theme generation complete!")
            return 0
        else:
            return 1
        
    except Exception as e:
        print(f"Error: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main())
