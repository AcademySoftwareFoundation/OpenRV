# Open RV Theme Generator

`generate_theme.py` is a Python script that creates custom themes for Open RV. Instead of editing complex CSS files by hand, you can specify your colors in a config file or enter them interactively, and the generator creates a professional theme for you.

**Here's exactly how it works:**

1. **Template System**: The script uses pre-built template files (`rv_mac_dark.qss.template` and `rv_linux_dark.qss.template`) that contain the complete Open RV interface styling.

2. **Variable Substitution**: You provide color values either through a simple config file, command-line arguments, or interactive prompts. The script finds all `{{VARIABLE_NAME}}` placeholders in the template and replaces them with your actual color values.

3. **Platform Detection**: The script automatically detects whether you're on macOS, Windows, or Linux and selects the appropriate template file.

4. **Validation**: Before generating the final theme, the script validates that all your color values are proper CSS colors (like `rgb(255,0,0)`, `#ff0000`, or `red`) to prevent broken stylesheets.

5. **Output Generation**: The result is a complete, valid QSS file that you can use directly with Open RV using the `-qtcss` command-line option.

**The key benefit**: Instead of learning CSS and manually editing 1000+ line files, you just specify 20+ color variables in a simple format like `ACCENT_PRIMARY = rgb(0,150,255)`, and get a complete, professional theme that covers every UI element in Open RV - from buttons and menus to sliders and text inputs.

**Default Config File**: The repository includes `src/lib/app/RvCommon/rv_theme_variables.conf` which contains all available theme variables with example values. This file serves as both a working example and a starting point for your custom themes.

## Quick Start

**1. Create a config file with your colors:**
```ini
# my_theme.conf
PRIMARY_BACKGROUND = rgb(25,25,30)
ACCENT_PRIMARY = rgb(0,150,255)
TEXT_PRIMARY = rgb(220,220,220)
BORDER_DARK = black
```

**2. Generate your theme:**
```bash
python src/lib/app/RvCommon/generate_theme.py --config my_theme.conf --output my_custom_theme.qss
```
*Note: When run from Open RV root directory, config files are automatically looked for in `src/lib/app/RvCommon/` and output files are placed there to keep all theme files organized.*

**Command Reference:**
```bash
python src/lib/app/RvCommon/generate_theme.py [options]

Required:
  --output FILE             Output theme file (.qss)

Options:
  --config FILE             Config file with your colors
  --set VARIABLE=COLOR      Set a specific color
  --list-variables FILE     Show what's in a config file
```

**Examples:**
```bash
# Basic generation with config file (from RvCommon directory)
python src/lib/app/RvCommon/generate_theme.py --config my_colors.conf --output my_theme.qss

# Generate with command-line variables (no config file needed)
python src/lib/app/RvCommon/generate_theme.py --output blue_theme.qss --set ACCENT_PRIMARY=rgb(0,100,255)

# Mix config file with command-line overrides
python src/lib/app/RvCommon/generate_theme.py -c base.conf -o custom.qss -s ACCENT_PRIMARY=red -s TEXT_PRIMARY=white

# Check what variables are in the default config file
python src/lib/app/RvCommon/generate_theme.py --list-variables rv_theme_variables.conf
```

**3. Use your theme in Open RV:**
```bash
RV -qtcss src/lib/app/RvCommon/my_custom_theme.qss
```

## Writing Config Files

### Basic Format:
```ini
# Comments start with #
PRIMARY_BACKGROUND = rgb(34,34,34)
ACCENT_PRIMARY = rgb(7,75,120)
TEXT_PRIMARY = rgb(200,200,200)
BORDER_DARK = black
BORDER_LIGHT = #8c8c8c
```

### Color Formats You Can Use:
```ini
# RGB (most common)
PRIMARY_BACKGROUND = rgb(34,34,34)
ACCENT_PRIMARY = rgb(7,75,120)

# Hex codes
TEXT_PRIMARY = #c8c8c8
BORDER_LIGHT = #8c8c8c

# Named colors
BORDER_DARK = black
SELECTION_TEXT = white
BUTTON_PRESSED = red
```

## Available Theme Variables

### Main Background Colors:
- `PRIMARY_BACKGROUND` - Main application background (behind all widgets)
- `SECONDARY_BACKGROUND` - Buttons, tabs, group boxes, dock titles, menu separators, tree view alternating rows
- `TERTIARY_BACKGROUND` - Hover states for dock titles and tabs

### Accent Colors:
- `ACCENT_PRIMARY` - **Most important color** - Menu hovers/selections, combo box hovers, button hover borders, tool button checked states, tree view selections, all interactive highlight states
- `ACCENT_SECONDARY` - Secondary accent used for dock widget borders and group box outlines

### Text Colors:
- `TEXT_PRIMARY` - Main text throughout the interface (labels, buttons, combo boxes)
- `TEXT_SECONDARY` - View menu text and combo box text
- `TEXT_BRIGHT` - Tab selected text and emphasized text states
- `TEXT_DISABLED` - Grayed out disabled text and controls

### Border Colors:
- `BORDER_DARK` - Dark borders on toolbars, input fields, and separators
- `BORDER_LIGHT` - Light borders on toolbars and UI dividers
- `BORDER_MEDIUM` - Medium tone borders for various UI elements

### Button States:
- `BUTTON_BORDER` - Default button borders (push buttons)
- `BUTTON_BORDER_DISABLED` - Borders for disabled/inactive buttons
- `BUTTON_HOVER` - Background color when hovering over tool buttons in play controls and view menus
- `BUTTON_PRESSED` - Background color when clicking/pressing buttons

### Input Controls:
- `CONTROL_BACKGROUND` - Combo box private scrollers and control track backgrounds
- `CONTROL_HANDLE` - Slider handles, scroll bar handles, dock widget close/float buttons
- `CONTROL_HANDLE_HOVER` - Slider and scroll bar handles when hovering
- `INPUT_BACKGROUND` - Text inputs, line edits, spin boxes, combo boxes, tool buttons, text edit areas

### Selection States:
- `SELECTION_BACKGROUND` - Background color for selected text in input fields
- `SELECTION_TEXT` - Text color for selected text in input fields

### Session Manager:
- `SESSION_ITEM_BORDER` - Borders around session manager items and tree elements

## Tips & Advanced Usage

### Start Simple:
Begin with just a few key colors and let the generator ask for the rest:
```ini
# minimal.conf
PRIMARY_BACKGROUND = rgb(30,30,35)
ACCENT_PRIMARY = rgb(0,150,255)
TEXT_PRIMARY = rgb(220,220,220)
```

### Copy Existing Themes:
Look at `src/lib/app/RvCommon/rv_theme_variables.conf` for a complete example with all variables.

### Interactive Help:
When prompted for a color, type `help` to see an example:
```bash
ACCENT_PRIMARY = help
Example for ACCENT_PRIMARY: rgb(7,75,120)
```

### Quick Testing Workflow:
Use command-line overrides to test color changes without editing files:
```bash
# Test a different accent color quickly
python src/lib/app/RvCommon/generate_theme.py --config my_theme.conf --output test.qss \
  --set ACCENT_PRIMARY=rgb(255,100,0)
```

### Recommended Workflow:
1. **Copy the default config to RvCommon directory:**
   ```bash
   cp src/lib/app/RvCommon/rv_theme_variables.conf src/lib/app/RvCommon/my_theme.conf
   ```

2. **Edit your colors:**
   ```bash
   nano src/lib/app/RvCommon/my_theme.conf
   # Change PRIMARY_BACKGROUND, ACCENT_PRIMARY, etc.
   ```

3. **Generate and test (using just the filename):**
   ```bash
   python src/lib/app/RvCommon/generate_theme.py --config my_theme.conf --output my_theme.qss
   RV -qtcss src/lib/app/RvCommon/my_theme.qss
   ```

4. **Refine and repeat!**

### Inspect Config Files:
Check what variables are defined in any config file:
```bash
python src/lib/app/RvCommon/generate_theme.py --list-variables existing_theme.conf
```

## Troubleshooting

### Common Errors:

**"Template file 'rv_mac_dark.qss.template' not found"**
- **Solution**: Run the script from the Open RV root directory (not from src/lib/app/RvCommon)
- **Check**: Make sure you're in the Open RV root with `ls src/lib/app/RvCommon/*.template`

**"Invalid CSS value 'badcolor'"**
- **Solution**: Use proper CSS colors: `rgb(255,0,0)`, `#ff0000`, or `red`
- **Valid formats**: RGB values 0-255, hex codes, or named colors (black, white, red, etc.)

**Variables missing or prompting unexpectedly**
- **Check**: Your config file syntax with `python src/lib/app/RvCommon/generate_theme.py --list-variables myfile.conf`
- **Common issue**: Missing `=` sign or extra spaces around variable names

**"No such file or directory" for config file**
- **Solution**: Make sure your config file is in `src/lib/app/RvCommon/` directory
- **Check**: `ls -la src/lib/app/RvCommon/your_config.conf` to verify the file exists
- **Note**: When running from Open RV root, the script automatically looks for config files in the RvCommon directory

## Platform Support

The generator automatically detects your platform and uses the appropriate template:
- **macOS/Windows**: Uses `rv_mac_dark.qss.template`
- **Linux**: Uses `rv_linux_dark.qss.template`

No configuration needed - it just works!
