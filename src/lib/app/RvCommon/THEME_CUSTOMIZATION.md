# OpenRV Theme Customization

This directory contains a command-line theme customization system for OpenRV.

## Quick Start

1. **Edit theme variables:**
   ```bash
   cd src/lib/app/RvCommon
   nano rv_theme_variables.conf
   ```

2. **Generate custom stylesheet:**
   ```bash
   python generate_stylesheet.py
   ```

3. **Rebuild OpenRV:**
   ```bash
   cd ../../../../  # Back to project root
   make clean && make -j8
   ```

4. **Launch with custom theme:**
   ```bash
   ./build/rv
   ```

## Files

- **`rv_theme_variables.conf`** - Edit colors, fonts, and dimensions here
- **`rv_mac_dark.qss.template`** - Template stylesheet with {{variable}} placeholders
- **`generate_stylesheet.py`** - Script that converts template + variables → final QSS
- **`rv_mac_dark.qss`** - Generated final stylesheet (don't edit directly!)

## Customization Examples

### Change to Blue Theme
```ini
# In rv_theme_variables.conf
[colors]
accent=rgb(70, 130, 255)
accent_hover=rgb(100, 150, 255)
accent_pressed=rgb(40, 90, 200)
```

### Make UI Larger
```ini
# In rv_theme_variables.conf
[dimensions]
toolbar_height_top=50
toolbar_height_bottom=45
icon_size=20px
```

### Different Background
```ini
# In rv_theme_variables.conf
[colors]
background=rgb(30, 30, 35)
primary_text=rgb(220, 220, 220)
```

## Advanced Usage

### Custom Template
```bash
python generate_stylesheet.py my_variables.conf my_template.qss.template my_output.qss
```

### Multiple Themes
1. Create `blue_theme.conf`, `dark_theme.conf`, etc.
2. Generate different stylesheets for each
3. Switch between them by copying to `rv_mac_dark.qss`

## Tips

- **Backup your working theme** before experimenting
- **Colors use RGB format:** `rgb(red, green, blue)` where values are 0-255
- **Dimensions can use:** `px`, `pt`, `em` units
- **Font sizes:** Use `%1pt` and `%2pt` for dynamic OpenRV font sizing
- **Test changes incrementally** - change one section at a time

## Troubleshooting

**Q: Changes don't appear?**
- Make sure you ran `python generate_stylesheet.py`
- Rebuild OpenRV: `make clean && make -j8`
- Check that `rv_mac_dark.qss` was updated

**Q: Python script error?**
- Ensure Python 3 is installed
- Check that variable names in template match those in conf file

**Q: Invalid QSS syntax?**
- Validate your color values: `rgb(0,0,0)` to `rgb(255,255,255)`
- Check for typos in variable names 
