// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
const vscode = require('vscode');

// this method is called when your extension is activated
// your extension is activated the very first time the command is executed
function activate(context) {
    console.log('Mu Language extension is now active!');
    
    // Register a provider for code completion
    const provider = vscode.languages.registerCompletionItemProvider('mu', {
        provideCompletionItems(document, position, token, context) {
            const completionItems = [];
            
            // Add basic Mu keywords
            const keywords = [
                'class', 'method', 'function', 'union', 'struct', 'enum', 'interface',
                'type', 'alias', 'typedef', 'namespace', 'module', 'require', 'use',
                'global', 'local', 'static', 'const', 'final', 'abstract', 'virtual',
                'override', 'public', 'private', 'protected', 'internal', 'extern',
                'volatile', 'transient', 'synchronized', 'native', 'strictfp',
                'if', 'else', 'for', 'while', 'do', 'break', 'continue', 'return',
                'try', 'catch', 'throw', 'finally', 'switch', 'case', 'default',
                'assert', 'let', 'var', 'and', 'or', 'not', 'in', 'is', 'as',
                'new', 'delete', 'sizeof', 'typeof', 'instanceof', 'this', 'super',
                'self', 'nil', 'true', 'false', 'void', 'null', 'undefined'
            ];
            
            keywords.forEach(keyword => {
                const item = new vscode.CompletionItem(keyword, vscode.CompletionItemKind.Keyword);
                item.documentation = new vscode.MarkdownString(`Mu keyword: \`${keyword}\``);
                completionItems.push(item);
            });
            
            // Add common Mu types
            const types = [
                'bool', 'int', 'float', 'double', 'char', 'byte', 'short', 'long',
                'string', 'void', 'Color', 'Vec2', 'Vec3', 'Vec4', 'Matrix4',
                'Rect', 'Point', 'Size', 'Event', 'Exception', 'MenuState',
                'MenuItem', 'Configuration'
            ];
            
            types.forEach(type => {
                const item = new vscode.CompletionItem(type, vscode.CompletionItemKind.TypeParameter);
                item.documentation = new vscode.MarkdownString(`Mu type: \`${type}\``);
                completionItems.push(item);
            });
            
            return completionItems;
        }
    });
    
    context.subscriptions.push(provider);
}

// this method is called when your extension is deactivated
function deactivate() {
    console.log('Mu Language extension is now deactivated');
}

module.exports = {
    activate,
    deactivate
};
