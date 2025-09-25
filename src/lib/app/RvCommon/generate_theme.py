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
    - Windows: rv_linux_dark.qss.template

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
import logging
import re
from typing import Dict, List, Optional, Set, Tuple


# Setup logging
def setup_logging() -> logging.Logger:
    """Setup logging with consistent formatting"""
    logging.basicConfig(
        level=logging.INFO,
        format="%(levelname)s: %(message)s",
        handlers=[logging.StreamHandler(sys.stdout)],
    )
    return logging.getLogger(__name__)


logger = setup_logging()


def get_platform_template() -> str:
    """Get the appropriate template file for the current platform"""
    system = platform.system().lower()
    # Windows and Linux both use the Linux template, only macOS uses the macOS template
    template_name = (
        "rv_mac_dark.qss.template"
        if system == "darwin"
        else "rv_linux_dark.qss.template"
    )

    # Check if we're being run from OpenRV root directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    current_dir = os.getcwd()

    # If script is in src/lib/app/RvCommon and we're not in that directory
    if os.path.basename(script_dir) == "RvCommon" and current_dir != script_dir:
        # Check if current directory looks like OpenRV root (has src/lib/app/RvCommon)
        rvcommon_path = os.path.join(current_dir, "src", "lib", "app", "RvCommon")
        if os.path.exists(rvcommon_path):
            template_path = os.path.join("src", "lib", "app", "RvCommon", template_name)
            if os.path.exists(template_path):
                return template_path

    return template_name


def adjust_output_path(output_file: str) -> str:
    """Adjust output path to RvCommon directory if running from OpenRV root"""
    # Check if we're being run from OpenRV root directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    current_dir = os.getcwd()

    # If script is in src/lib/app/RvCommon and we're not in that directory
    if os.path.basename(script_dir) == "RvCommon" and current_dir != script_dir:
        # Check if current directory looks like OpenRV root (has src/lib/app/RvCommon)
        rvcommon_path = os.path.join(current_dir, "src", "lib", "app", "RvCommon")
        if os.path.exists(rvcommon_path):
            # Only adjust if output path is just a filename (no directory)
            if not os.path.dirname(output_file):
                adjusted_path = os.path.join(
                    "src", "lib", "app", "RvCommon", output_file
                )
                logger.info(f"Placing output in RvCommon directory: {adjusted_path}")
                return adjusted_path

    return output_file


def adjust_config_path(config_file: Optional[str]) -> Optional[str]:
    """Adjust config file path to RvCommon directory if running from OpenRV root"""
    if not config_file:
        return config_file

    # Check if we're being run from OpenRV root directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    current_dir = os.getcwd()

    # If script is in src/lib/app/RvCommon and we're not in that directory
    if os.path.basename(script_dir) == "RvCommon" and current_dir != script_dir:
        # Check if current directory looks like OpenRV root (has src/lib/app/RvCommon)
        rvcommon_path = os.path.join(current_dir, "src", "lib", "app", "RvCommon")
        if os.path.exists(rvcommon_path):
            # Only adjust if config path is just a filename (no directory)
            if not os.path.dirname(config_file):
                adjusted_path = os.path.join(
                    "src", "lib", "app", "RvCommon", config_file
                )
                # Check if the adjusted path exists, if not, try the original
                if os.path.exists(adjusted_path):
                    logger.info(
                        f"Using config from RvCommon directory: {adjusted_path}"
                    )
                    return adjusted_path

    return config_file


def load_variables_from_config(config_file: Optional[str]) -> Dict[str, str]:
    """Load variables from a config file with validation"""
    variables: Dict[str, str] = {}
    invalid_count: int = 0

    if not config_file or not os.path.exists(config_file):
        return variables

    try:
        with open(config_file, "r", encoding="utf-8") as f:
            for line_num, line in enumerate(f, 1):
                original_line = line
                line = line.strip()
                if line and not line.startswith("#"):
                    if "=" in line:
                        key, value = line.split("=", 1)
                        key = key.strip()
                        value = value.strip()

                        # Check for empty key or value
                        if not key:
                            logger.warning(
                                f"Empty variable name in {config_file} line {line_num}: {original_line.strip()}"
                            )
                            invalid_count += 1
                            continue

                        if not value:
                            logger.warning(
                                f"Empty value in {config_file} line {line_num}: {key} = (empty)"
                            )
                            invalid_count += 1
                            continue

                        # Check for duplicate variables
                        if key in variables:
                            logger.warning(
                                f"Duplicate variable '{key}' in {config_file} line {line_num} (overwriting previous value)"
                            )

                        # Validate the value
                        if validate_css_value(key, value):
                            variables[key] = value
                        else:
                            logger.warning(
                                f"Skipping invalid value in {config_file} line {line_num}: {key} = {value}"
                            )
                            example = get_example_value(key)
                            logger.info(f"Example: {key} = {example}")
                            logger.info(
                                f"Variable will remain as {{{{{key}}}}} placeholder"
                            )
                            invalid_count += 1
                            # Skip invalid values - don't include them
                    else:
                        logger.warning(
                            f"Malformed line in {config_file} line {line_num}: {original_line.strip()}"
                        )
                        logger.info(f"Expected format: VARIABLE = value")
                        invalid_count += 1
    except UnicodeDecodeError as e:
        logger.error(f"Config file {config_file} has encoding issues: {e}")
        logger.info("Please save the file as UTF-8 encoding")
        return {}
    except Exception as e:
        logger.error(f"Failed to read config file {config_file}: {e}")
        return {}

    if invalid_count > 0:
        logger.warning(
            f"Found {invalid_count} invalid value(s) in config file. Please fix them to ensure proper CSS."
        )

    return variables


def validate_css_value(var_name: str, value: str) -> bool:
    """Validate CSS values to prevent broken stylesheets"""
    if not value or value.lower() == "quit":
        return True

    # Color-related variables should be valid CSS colors
    color_vars: Set[str] = {
        "PRIMARY_BACKGROUND",
        "SECONDARY_BACKGROUND",
        "TERTIARY_BACKGROUND",
        "ACCENT_PRIMARY",
        "ACCENT_SECONDARY",
        "TEXT_PRIMARY",
        "TEXT_SECONDARY",
        "TEXT_BRIGHT",
        "TEXT_DISABLED",
        "BORDER_DARK",
        "BORDER_LIGHT",
        "BORDER_MEDIUM",
        "BUTTON_BORDER",
        "BUTTON_BORDER_DISABLED",
        "BUTTON_HOVER",
        "BUTTON_PRESSED",
        "CONTROL_BACKGROUND",
        "CONTROL_HANDLE",
        "CONTROL_HANDLE_HOVER",
        "INPUT_BACKGROUND",
        "SELECTION_BACKGROUND",
        "SELECTION_TEXT",
        "SESSION_ITEM_BORDER",
    }

    if var_name in color_vars:
        # Check for valid CSS color formats

        # RGB format: rgb(r, g, b) with proper range validation
        rgb_match = re.match(
            r"^rgb\(\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(\d{1,3})\s*\)$", value
        )
        if rgb_match:
            r, g, b = map(int, rgb_match.groups())
            return 0 <= r <= 255 and 0 <= g <= 255 and 0 <= b <= 255

        # Named colors (basic validation)
        named_colors: Set[str] = {
            "black",
            "white",
            "red",
            "darkred",
            "green",
            "darkgreen",
            "blue",
            "darkblue",
            "yellow",
            "darkyellow",
            "cyan",
            "darkcyan",
            "magenta",
            "darkmagenta",
            "gray",
            "darkgray",
            "lightgray",
            "transparent",
            "color0",
            "color1",
        }

        # Check Hex format or named colors (case insensitive)
        return (
            re.match(r"^#([0-9a-fA-F]{3}|[0-9a-fA-F]{6})$", value) is not None
            or value.lower() in named_colors
        )

    # Non-color variables: for now all variables are colors
    return len(value.strip()) > 0


def get_example_value(var_name: str) -> str:
    """Get example values for variables to help users"""
    examples: Dict[str, str] = {
        "PRIMARY_BACKGROUND": "rgb(34,34,34)",
        "ACCENT_PRIMARY": "rgb(7,75,120)",
        "TEXT_PRIMARY": "rgb(200,200,200)",
        "BORDER_DARK": "black",
        "BORDER_LIGHT": "rgb(140,140,140)",
    }
    return examples.get(var_name, "rgb(100,100,100)")


def collect_missing_variables(missing_vars: Set[str]) -> Dict[str, str]:
    """Interactive collection of missing variables with validation"""
    print("\nMissing variables detected. Please provide values:")
    print(
        "('quit' to abort, 'default' to use default value, 'help' for examples, Ctrl+C to cancel)"
    )

    # Read the config file and get the default value for the variable
    config_file = adjust_config_path("rv_theme_variables.conf")
    variables = load_variables_from_config(config_file)

    collected_vars: Dict[str, str] = {}
    try:
        for var_name in sorted(missing_vars):
            while True:
                try:
                    # Show default value in prompt if available
                    if var_name in variables:
                        prompt = f"{var_name} (default: {variables[var_name]}) = "
                    else:
                        prompt = f"{var_name} (no default) = "
                    value = input(prompt).strip()

                    if value.lower() == "quit":
                        logger.info("Aborted by user.")
                        sys.exit(1)
                    elif value.lower().startswith("help"):
                        example = get_example_value(var_name)
                        print(f"Example for {var_name}: {example}")
                        continue
                    elif value.lower() in ["default", ""]:
                        # Use example value as default since variable is missing from config
                        if var_name in variables:
                            collected_vars[var_name] = variables[var_name]
                            print(f"Set: {var_name} = {variables[var_name]}")
                            break
                        else:
                            print(
                                f"No default value found for {var_name}. Please provide a valid CSS value."
                            )
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
                    logger.info("\n\nOperation cancelled by user (Ctrl+C)")
                    sys.exit(1)
                except EOFError:
                    logger.info("\n\nUnexpected end of input")
                    sys.exit(1)

    except Exception as e:
        logger.error(f"Error during variable collection: {e}")
        sys.exit(1)

    return collected_vars


def process_template(template_file: str, variables: Dict[str, str]) -> str:
    """Process template file replacing {{variable}} placeholders"""
    if not os.path.exists(template_file):
        raise FileNotFoundError(f"Template file '{template_file}' not found")

    try:
        with open(template_file, "r", encoding="utf-8") as f:
            content = f.read()
    except UnicodeDecodeError as e:
        raise ValueError(
            f"Template file {template_file} has encoding issues: {e}. Please save as UTF-8."
        )
    except Exception as e:
        raise ValueError(f"Failed to read template file {template_file}: {e}")

    if not content.strip():
        raise ValueError(f"Template file {template_file} is empty")

    # Find all template variables in the file
    template_vars = set(re.findall(r"\{\{(\w+)\}\}", content))

    # Check for variables in config that don't exist in template
    extra_vars = set(variables.keys()) - template_vars
    if extra_vars:
        logger.warning(
            f"Found {len(extra_vars)} variables in config that don't exist in template:"
        )
        for var in sorted(extra_vars):
            logger.info(f"  {var} = {variables[var]}")
        logger.info("These variables will be ignored.")

    # Check for missing variables
    missing_vars = template_vars - set(variables.keys())
    if missing_vars:
        logger.info(
            f"Found {len(missing_vars)} missing variables: {', '.join(sorted(missing_vars))}"
        )

        # Collect missing variables interactively
        new_vars = collect_missing_variables(missing_vars)

        # Add collected variables (this will overwrite existing ones if provided)
        variables.update(new_vars)

        if new_vars:
            logger.info(f"Collected {len(new_vars)} additional variables.")

    # Replace all {{VARIABLE}} placeholders (only for variables that exist in template)
    for var_name in template_vars:
        if var_name in variables:
            placeholder = f"{{{{{var_name}}}}}"
            content = content.replace(placeholder, variables[var_name])

    return content


def parse_variable_override(var_string: str) -> Tuple[str, str]:
    """Parse a variable override in format KEY=VALUE"""
    if "=" not in var_string:
        raise ValueError(f"Invalid variable format: {var_string}. Use KEY=VALUE")
    key, value = var_string.split("=", 1)
    return key.strip(), value.strip()


def generate_theme(
    config_file: Optional[str],
    output_file: str,
    variable_overrides: Optional[List[str]] = None,
) -> str:
    """Generate a theme file"""
    logger.info(f"Generating theme: {output_file}")

    # Adjust output path if running from OpenRV root
    output_file = adjust_output_path(output_file)

    # Adjust config path if running from OpenRV root
    config_file = adjust_config_path(config_file)

    # Automatically select template based on platform
    template_file = get_platform_template()
    logger.info(f"Platform detected: {platform.system()}")
    logger.info(f"Using template: {template_file}")

    # Load base variables from config
    variables = load_variables_from_config(config_file)
    if config_file and variables:
        logger.info(f"Loaded {len(variables)} variables from {config_file}")
    elif config_file:
        logger.info(f"Config file {config_file} not found or empty")

    # Apply command-line overrides (these can overwrite config variables)
    if variable_overrides:
        for override in variable_overrides:
            key, value = parse_variable_override(override)
            if key in variables:
                logger.info(f"Overwriting: {key} = {value} (was: {variables[key]})")
            else:
                logger.info(f"Setting: {key} = {value}")
            variables[key] = value

    # Process template (will prompt for missing variables interactively)
    logger.info(f"Processing template: {template_file}")
    result = process_template(template_file, variables)

    # Write output
    os.makedirs(
        os.path.dirname(output_file) if os.path.dirname(output_file) else ".",
        exist_ok=True,
    )
    with open(output_file, "w", encoding="utf-8") as f:
        f.write(result)

    logger.info(f"Generated: {output_file}")
    return output_file


def main():
    parser = argparse.ArgumentParser(
        description="Generate an OpenRV theme from template (auto-detects platform)",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )

    parser.add_argument("--config", "-c", help="Variables config file")

    parser.add_argument("--output", "-o", required=True, help="Output stylesheet file")

    parser.add_argument(
        "--set",
        "-s",
        action="append",
        dest="variables",
        help="Set variable (format: KEY=VALUE). Can be used multiple times",
    )

    parser.add_argument(
        "--list-variables", help="List all variables found in a config file"
    )

    args = parser.parse_args()

    try:
        # Utility command
        if args.list_variables:
            variables = load_variables_from_config(args.list_variables)
            if variables:
                logger.info(f"Variables in {args.list_variables}:")
                for key, value in variables.items():
                    logger.info(f"  {key} = {value}")
            else:
                logger.info(f"No variables found in {args.list_variables}")
            return 0

        # Check if output file ends with .qss
        if not args.output.endswith(".qss"):
            logger.error(f"Output file must end with .qss: {args.output}")
            return 1

        # Generate theme
        result = generate_theme(args.config, args.output, args.variables)
        if result:
            logger.info("Theme generation complete!")
            return 0
        else:
            return 1

    except Exception as e:
        logger.error(f"Error: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
