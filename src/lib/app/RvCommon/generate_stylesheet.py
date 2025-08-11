#!/usr/bin/env python3
"""
Stylesheet Generator for OpenRV
Reads variables from rv_theme_variables.conf and generates final QSS
Usage: python generate_stylesheet.py [variables_file] [template_file] [output_file]
"""

import configparser
import re
import sys
import os

def load_variables(config_file):
    """Load variables from config file"""
    config = configparser.ConfigParser()
    config.read(config_file)
    
    variables = {}
    for section in config.sections():
        for key, value in config[section].items():
            variables[key] = value
    
    return variables

def process_template(template_file, variables):
    """Replace {{variable}} placeholders in template"""
    with open(template_file, 'r') as f:
        content = f.read()
    
    # Replace {{variable_name}} with actual values
    for var_name, var_value in variables.items():
        placeholder = f"{{{{{var_name}}}}}"
        content = content.replace(placeholder, var_value)
    
    return content

def main():
    # Default files
    variables_file = "rv_theme_variables.conf"
    template_file = "rv_mac_dark.qss.template"
    output_file = "rv_mac_dark.qss"
    
    # Allow command line override
    if len(sys.argv) >= 2:
        variables_file = sys.argv[1]
    if len(sys.argv) >= 3:
        template_file = sys.argv[2]
    if len(sys.argv) >= 4:
        output_file = sys.argv[3]
    
    # Check if files exist
    if not os.path.exists(variables_file):
        print(f"Error: Variables file '{variables_file}' not found")
        return 1
    
    if not os.path.exists(template_file):
        print(f"Error: Template file '{template_file}' not found")
        return 1
    
    try:
        # Load variables
        print(f"Loading variables from {variables_file}...")
        variables = load_variables(variables_file)
        print(f"Loaded {len(variables)} variables")
        
        # Process template
        print(f"Processing template {template_file}...")
        result = process_template(template_file, variables)
        
        # Write output
        with open(output_file, 'w') as f:
            f.write(result)
        
        print(f"Generated stylesheet: {output_file}")
        return 0
        
    except Exception as e:
        print(f"Error: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main()) 
