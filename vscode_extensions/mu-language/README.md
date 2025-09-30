# Mu Language Support for VS Code

This extension provides syntax highlighting and basic language support for the Mu programming language used in OpenRV.

## Features

- **Syntax Highlighting**: Full syntax highlighting for Mu files (.mu)
- **Code Completion**: Basic keyword and type completion
- **Language Configuration**: Proper comment handling, bracket matching, and indentation
- **Folding Support**: Code folding for regions and blocks

## Supported Syntax Elements

- **Keywords**: `class`, `method`, `function`, `union`, `struct`, `enum`, `interface`, `type`, `alias`, `typedef`, `namespace`, `module`, `require`, `use`, `global`, `local`, `static`, `const`, `final`, `abstract`, `virtual`, `override`, `public`, `private`, `protected`, `internal`, `extern`, `volatile`, `transient`, `synchronized`, `native`, `strictfp`
- **Control Flow**: `if`, `else`, `for`, `while`, `do`, `break`, `continue`, `return`, `try`, `catch`, `throw`, `finally`, `switch`, `case`, `default`, `assert`, `let`, `var`
- **Operators**: `and`, `or`, `not`, `in`, `is`, `as`, `new`, `delete`, `sizeof`, `typeof`, `instanceof`, `this`, `super`, `self`, `nil`, `true`, `false`, `void`, `null`, `undefined`
- **Types**: `bool`, `int`, `float`, `double`, `char`, `byte`, `short`, `long`, `string`, `void`, `Color`, `Vec2`, `Vec3`, `Vec4`, `Matrix4`, `Rect`, `Point`, `Size`, `Event`, `Exception`, `MenuState`, `MenuItem`, `Configuration`
- **Comments**: Single-line (`//`) and multi-line (`/* */`) comments
- **Strings**: Single and double-quoted strings with escape sequences
- **Numbers**: Integers, floats, and hexadecimal numbers
- **Functions**: Function definitions and lambda expressions (`\: functionName`)

## Installation

1. Copy this extension folder to your VS Code extensions directory:
   - **Windows**: `%USERPROFILE%\.vscode\extensions\`
   - **macOS**: `~/.vscode/extensions/`
   - **Linux**: `~/.vscode/extensions/`

2. Restart VS Code or reload the window (Ctrl+Shift+P → "Developer: Reload Window")

## Usage

Once installed, the extension will automatically:
- Recognize `.mu` files as Mu language files
- Provide syntax highlighting
- Enable code completion for Mu keywords and types
- Handle proper indentation and bracket matching

## Language Features

### Comments
- Single-line comments: `// This is a comment`
- Multi-line comments: `/* This is a multi-line comment */`
- Region markers: `// #region` and `// #endregion` for code folding

### Strings
- Double-quoted strings: `"Hello World"`
- Single-quoted strings: `'Hello World'`
- String interpolation: `"Value: %s" % variable`
- Escape sequences: `\n`, `\t`, `\"`, `\\`

### Classes and Methods
```mu
class: MyClass
{
    method: myMethod (string; int param)
    {
        return "Hello " + param;
    }
}
```

### Functions and Lambdas
```mu
\: myFunction (void; int x, int y)
{
    print("x: " + x + ", y: " + y);
}

\: lambda = \: (int; int x) { x * 2; };
```

### Unions and Enums
```mu
union: MyUnion { Option1 | Option2 | Option3 }
enum: MyEnum { Value1, Value2, Value3 }
```

## Contributing

This extension is designed specifically for the Mu language used in OpenRV. If you find issues or have suggestions for improvements, please feel free to contribute to the OpenRV project.

## License

Apache-2.0 License - see the OpenRV project for details.
