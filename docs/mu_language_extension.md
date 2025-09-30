# Mu Language Extension for VS Code/Cursor

This directory contains a VS Code/Cursor extension that provides syntax highlighting and language support for the Mu programming language used in OpenRV.

## Quick Installation

### For VS Code:
```bash
cd mu_language_extension/mu-language
./install_vscode.sh
```

### For Cursor:
```bash
cd mu_language_extension/mu-language
./install_cursor.sh
```

### Using the main script:
```bash
cd mu_language_extension/mu-language
./_install.sh vscode    # For VS Code
./_install.sh cursor    # For Cursor
```

2. **Restart your editor** or reload the window (Ctrl+Shift+P → "Developer: Reload Window")

## Important: File Type Selection

After installation, you may need to manually select the "Mu" language mode for `.mu` files:

1. **Open a `.mu` file**
2. **Look at the bottom-right corner** of the editor - it may show "Plain Text" or "C++"
3. **Click on the language mode** (e.g., "Plain Text") in the bottom-right corner
4. **Select "Mu"** from the language list that appears
5. **If "Mu" doesn't appear in the list**, try:
   - Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on Mac)
   - Type "Change Language Mode"
   - Select "Mu" from the list
6. **Restart the editor** or reload the window (`Ctrl+Shift+P` → "Developer: Reload Window")

Once properly configured, `.mu` files will automatically be recognized as "Mu" language files and display full syntax highlighting.

## Manual Installation

If you prefer to install manually:

### For VS Code:
Copy the entire `mu_language_extension/mu-language` folder to your VS Code extensions directory:
- **Windows**: `%USERPROFILE%\.vscode\extensions\`
- **macOS**: `~/.vscode/extensions/`
- **Linux**: `~/.vscode/extensions/`

### For Cursor:
Copy the entire `mu_language_extension/mu-language` folder to your Cursor extensions directory:
- **Windows**: `%USERPROFILE%\.cursor\extensions\`
- **macOS**: `~/.cursor/extensions/`
- **Linux**: `~/.cursor/extensions/`

Then restart your editor.

## Features

- **Syntax Highlighting**: Full syntax highlighting for Mu files (.mu)
- **Code Completion**: Basic keyword and type completion
- **Language Configuration**: Proper comment handling, bracket matching, and indentation
- **Folding Support**: Code folding for regions and blocks

## What's Included

- `package.json` - Extension manifest
- `language-configuration.json` - Language configuration (comments, brackets, indentation)
- `syntaxes/mu.tmLanguage.json` - TextMate grammar for syntax highlighting
- `extension.js` - Main extension code with completion provider
- `README.md` - Detailed documentation
- `_install.sh` - Main installation script (takes vscode|cursor argument)
- `install_vscode.sh` - Convenience script for VS Code installation
- `install_cursor.sh` - Convenience script for Cursor installation
- `settings.json` - VS Code/Cursor settings template for file associations

## Usage

Once installed, the extension will automatically:
- Recognize `.mu` files as Mu language files
- Provide syntax highlighting for keywords, types, strings, comments, etc.
- Enable code completion for Mu keywords and types
- Handle proper indentation and bracket matching

## Contributing

This extension is part of the OpenRV project. If you find issues or have suggestions for improvements, please contribute to the OpenRV repository.

## License

Apache-2.0 License - see the main OpenRV project for details.
