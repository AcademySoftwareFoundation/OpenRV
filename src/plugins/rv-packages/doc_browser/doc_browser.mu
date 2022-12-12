//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
use qt;

documentation: """
This module can parse and convert a limited set of *asciidoc*
documentation markup into HTML. Here are some examples:

A table with some attributes:

[width="50%",options="header"]
|===================================================
| int64 | capacity of the cache in bytes | and some other stuff
| int64 | bytes used by cache | a
| int64 | look ahead bytes used | a
| float | look ahead seconds | a
| float | look ahead wait time in seconds | a
| float | audio cache time in seconds | a
| int[] | array of start-end frames of cached ranges | a
|===================================================

And here is *bold* _italic_ and 'emphisised' and +monospaced+ example.

A *larger bold example* that terminates.

And here is a shell command like `ls -al` which can also be an example like:

======
ls -al
======

A list looks like this:

 * One item
 * Two items
 * Three items
 * Etc

A heading should work like so:

== Things I'd like to fix ==

Look a the source for this function to see the actual markup syntax.

Here's ASCIIDOC's homepage: http://www.methods.co.nz/asciidoc/
And Google: http://www.google.com

""";


module: asciidoc_to_html {

    global regex listingRE = "^-+$";
    global regex exampleRE = "^=+$";
    global regex bulletRE = "^ *(- |\\*+ )";
    global regex enumRE = "^([a-zA-Z]\\.|[0-9]+\\.|\\.+)";
    global regex tableBoundsRE = "^[|]=+$";
    global regex tableRowRE = "^[|][^=]";
    global regex attrRE = "^[[].*]$";
    global regex urlRE = "^(https?)|(file):.*$";

    documentation: """
    Converts 'text' to HTML with possible formatting markup. Currently accepts
    bold (*) italic (_) or (') monospaced (+) and (`) surrounding words or
    phrases. 
   
    The text in HTML form is returned.
    """;

    \: formatting (string; string text)
    {
        use io;
        osstream str;

        for_each (word; text.split(" "))
        {
            if (word.size() == 0) continue;

            bool startbold = word[0] == '*';
            bool endbold = word[-1] == '*';
            bool startemph = word[0] == '_';
            bool endemph = word[-1] == '_';
            bool startemph2 = word[0] == '\'';
            bool endemph2 = word[-1] == '\'';
            bool startmono = word[0] == '+';
            bool endmono = word[-1] == '+';
            bool startpass = word[0] == '`';
            bool endpass = word[-1] == '`';

            if (startbold && !endbold) print(str, " <b>%s" % word.substr(1,word.size()-1));
            else if (startbold && endbold) print(str, " <b>%s</b>" % word.substr(1,word.size()-2));
            else if (endbold) print(str, " %s</b>" % word.substr(0,word.size()-1));

            else if (startemph && !endemph) print(str, " <i>%s" % word.substr(1,word.size()-1));
            else if (startemph && endemph) print(str, " <i>%s</i>" % word.substr(1,word.size()-2));
            else if (endemph) print(str, " %s</i>" % word.substr(0,word.size()-1));

            else if (startemph2 && !endemph2) print(str, " <i>%s" % word.substr(1,word.size()-1));
            else if (startemph2 && endemph2) print(str, " <i>%s</i>" % word.substr(1,word.size()-2));
            else if (endemph2) print(str, " %s</i>" % word.substr(0,word.size()-1));

            else if (startmono && !endmono) print(str, " <tt>%s" % word.substr(1,word.size()-1));
            else if (startmono && endmono) print(str, " <tt>%s</tt>" % word.substr(1,word.size()-2));
            else if (endmono) print(str, " %s</tt>" % word.substr(0,word.size()-1));

            else if (startpass && !endpass) print(str, " <tt>%s" % word.substr(1,word.size()-1));
            else if (startpass && endpass) print(str, " <tt>%s</tt>" % word.substr(1,word.size()-2));
            else if (endpass) print(str, " %s</tt>" % word.substr(0,word.size()-1));
            
            else if (urlRE.match(word)) 
            {
                print(str, " <a href=\"%s\">%s</a>" % (word, word));
            }
            else 
            {
                print(str, " %s" % word);
            }
        }

        return string(str);
    }

    documentation: """
    Convert 'text' in markup form into HTML. Returns the HTML string.
    """;

    \: toHTML (string; string text)
    {
        use io;
        osstream str;

        let lines = text.split("\n", true);
        bool inpre = false;
        bool inp = false;
        bool intable = false;
        bool inlist = false;
        (string,string)[] attrs;

        for_index (i; lines)
        {
            let line      = lines[i],
                islisting = listingRE.match(line),
                isexample = exampleRE.match(line),
                istable   = tableBoundsRE.match(line),
                isrow     = tableRowRE.match(line),
                isbullet  = bulletRE.match(line),
                isenum    = enumRE.match(line),
                isattr    = attrRE.match(line);

            if (inlist && !isbullet) 
            {
                print(str, "</ul>\n");
                inlist = false;
            }

            if (lines[i] == "" && !inpre)  
            {
                print(str, if inp then "</p>" else "<p>");
                inp = !inp;
                attrs.clear();
            }
            else if (isattr)
            {
                try
                {
                    let pairs = line.substr(1, line.size()-2).split("\",", true);
                    attrs.clear();
                
                    for_each (pair; pairs)
                    {
                        let parts = pair.split("=", true),
                            parts2 = parts[1].split("\"", true)[1];
                        attrs.push_back((parts[0], parts2));
                    }
                }
                catch (exception exc)
                {
                    print("ERROR: exception in attr line: %s\n" % line);
                }
            }
            else if (isbullet)
            {
                if (!inlist) print(str, "<ul>");
                inlist = true;
                print(str, "<li> %s </li>\n" % formatting(lines[i]));
            }
            else if (islisting || isexample)
            {
                if (inpre) 
                {
                    print(str, "</pre>");
                    inpre = false;
                }
                else 
                {
                    if (inp) 
                    { 
                        print(str, "</p>");
                        inp = false; 
                    }

                    string tclass;

                    if (lines[i][0] == '-') tclass = "listing";
                    else tclass = "example";
 
                    print(str, "<pre class=\"%s\">" % tclass);
                    inpre = true;
                }

                attrs.clear();
            }
            else if (istable)
            {
                if (intable) 
                {
                    print(str, "</table>");
                }
                else 
                {
                    string tclass = "basictable";
                    string attrlist = "";

                    for_each (a; attrs)
                    {
                        let (name, value) = a;

                        if (name == "width") attrlist += " width=" + value;
                        else if (name == "class") tclass = value;
                    }
                    
                    print(str, "<table class=\"%s\" %s" % (tclass, attrlist));
                    print(str, ">");
                }

                intable = !intable;

                attrs.clear();
            }
            else if (intable && isrow)
            {
                print(str, "<tr class=\"basictr\">");
                let cells = line.split("|", true);
                cells.erase(0, 1);

                for_each (cell; cells)
                {
                    print(str, "<td class=\"basictd\">");
                    print(str, cell);
                    print(str, "</td>");
                }

                print(str, "</tr>");

                attrs.clear();
            }
            else
            {
                print(str, formatting(lines[i]) + "\n");
                attrs.clear();
            }
        }

        return string(str);
    }

}


module: doc_browser {
use runtime;
use asciidoc_to_html;

global string startHTML = """
<html>
%s
<body>
<h2 align="center">Mu API Documentation Browser</h2>
<h1 align="center"><img align="center" src=\"mu.png\"></h1>

<table align="center" class="starttable">
<tr> <td class="basictd"> <img src="lambda.png"> is a function. </td> </tr>
<tr> <td class="basictd"> <img src="method.png"> is a method on a class.</td> </tr>
<tr> <td class="basictd"> <img src="type.png"> is a type.</td> </tr>
<tr> <td class="basictd"> <img src="module.png"> is a module.</td> </tr>
<tr> <td class="basictd"> <img src="constant.png"> is a symbolic constant.</td> </tr>
<tr> <td class="basictd"> <img src="symbol.png"> is a generic symbol (not yet working).</td> </tr>
</table>
</body>
</html>
""";

global string css = """
<style type="text/css">

body { background-color: white; }
img { vertical-align: -35%; }

.hsymtype { color: grey; }

.basictable
{ 
   border-collapse: collapse;
   display: inline-block;
   padding: 1ex;
   margin-left: 10%;
   margin-right: auto;
   background: #fafafa;
}

.starttable
{ 
   border-collapse: collapse;
   margin-left: auto;
   margin-right: auto;
   background: #fafafa;
}

.basictd
{
    border: 1px solid #d0d0d0;
    padding: 4px;
    margin: 0px;
}

.basictr
{
    border: 1px solid #d0d0d0;
    padding: 4px;
    margin: 0px;
}

.typeconstrtable
{
   padding: 4px;
   border-collapse: collapse;
   display: inline-block;
   padding: 4ex;
   margin-left: 10%;
   margin-right: auto;
}

.mutypetable
{
   padding: 4px;
   border-collapse: collapse;
   display: inline-block;
   padding: 4ex;
   margin-left: 10%;
   margin-right: auto;
}

.typeKindCell { text-align: right; }
.typeNameCell { text-align: left; font-weight: bold; }

.muparamtable
{ 
   border-collapse: collapse;
   display: inline-block;
   padding: 1ex;
   margin-left: 10%;
   margin-right: auto;
   background: #fafafa;
   border: 1px solid #d0d0d0;
}

table.muparamtable td
{
    border: 1px solid #d0d0d0;
    padding: 8px;
}

.paramvartd { font-weight: bold; }
.paramtypetd { }
.paramdoctd { }

.methodtable
{
   border-collapse: collapse;
   display: inline-block;
   padding: 4ex;
   margin-left: 10%;
   margin-right: auto;
}

.functiontable
{
   border-collapse: collapse;
   display: inline-block;
   padding: 4ex;
   margin-left: 10%;
   margin-right: auto;
}

.functioncell { font-weight: bold; color: sienna; text-align: right; }
a:link { font-weight: bold; color: sienna; text-decoration: none; text-align: right; }
a:hover { font-weight: bold; background-color: #f0f060; color: sienna; text-decoration: none; text-align: right; }
.paramlistcell { text-align: left; }

.mutype { color: darkgreen; }
.muparam { font-weight: bold; }
.signature 
{ 
    font-family: Monaco, Monospace, Courier;
    margin-right: 10%; 
    margin-left: 10%; 
    background-color: #fafafa;
    padding-left: 10px;
    padding-right: 10px;
    padding-top: 10px;
    padding-bottom: 10px;
    border: thin solid #c0c0c0;
}

.muclassdef 
{ 
    font-family: Monaco, Monospace, Courier;
    margin-right: 10%; 
    margin-left: 10%; 
    background-color: #fafafa;
    padding-left: 10px;
    padding-right: 10px;
    padding-top: 10px;
    padding-bottom: 10px;
    border: thin solid #c0c0c0;
}

.muuniondef 
{ 
    font-family: Monaco, Monospace, Courier;
    margin-right: 10%; 
    margin-left: 10%; 
    background-color: #fafafa;
    padding-left: 10px;
    padding-right: 10px;
    padding-top: 10px;
    padding-bottom: 10px;
    border: thin solid #c0c0c0;
}

.nostate
{
    margin-right: 10%; 
    margin-left: 10%; 
    background-color: #fafafa;
    padding-left: 10px;
    padding-right: 10px;
    padding-top: 10px;
    padding-bottom: 10px;
    border: thin solid #c0c0c0;
    font-style: italic;
    text-align: center;
}

.subtext
{
    font-size: 80%;
    color: grey;
}

</style>
""";

SymbolPredicate := (bool; symbol);

\: map_over_symbol_tree ([symbol]; symbol s, SymbolPredicate F, [symbol] list)
{
    for_each (child; symbol_symbols_in_scope(s)) 
        list = map_over_symbol_tree(child, F, list);
    if F(s) then s : list else list;
}

\: search_for_regex (bool; symbol s, regex rexp)
{
    let n = symbol_name(s);
    return rexp.match(n) || rexp.match(symbol_documentation(s));
}

\: match_regex ([symbol]; regex re)
{
    map_over_symbol_tree(symbol_from_name(intern_name("")), 
                         search_for_regex(,re),
                         nil);
}

\: lexicalCompareBySymbolType (int; symbol a, symbol b)
{
    let atype   = symbol_is_type(a),
        btype   = symbol_is_type(b),
        amod    = symbol_is_module(a),
        bmod    = symbol_is_module(b),
        afunc   = symbol_is_function(a),
        bfunc   = symbol_is_function(b),
        amethod = symbol_is_method(a),
        bmethod = symbol_is_method(b),
        avar    = symbol_is_variable(a),
        bvar    = symbol_is_variable(b);

    int rval = 0;

    if (amod && !bmod) rval = 1;
    else if (bmod && !amod) rval = -1;
    else if (atype && !btype) rval = 1;
    else if (btype && !atype) rval = -1;
    else if (amethod && !bmethod) rval = 1;
    else if (bmethod && !amethod) rval = -1;
    else if (afunc && !bfunc) rval = 1;
    else if (bfunc && !afunc) rval = -1;
    else if (avar && !bvar) rval = 1;
    else if (bvar && !avar) rval = -1;

    return rval;
}

operator: <= (bool; symbol a, symbol b) 
{ 
    let c = lexicalCompareBySymbolType(a,b);

    if (c == -1) return false;
    else if (c == 1) return true;
    else return compare(symbol_fully_qualified_name(a), 
                        symbol_fully_qualified_name(b)) <= 0;
}

operator: >= (bool; symbol a, symbol b) 
{ 
    let c = lexicalCompareBySymbolType(a,b); 

    if (c == -1) return true;
    else if (c == 1) return false;
    else return compare(symbol_fully_qualified_name(a), 
                        symbol_fully_qualified_name(b)) >= 0;
}

function: sort (void; symbol[] array)
{
    function: qsort (void; symbol[] a, int lo, int hi)
    {
        if (lo < hi) 
        {
            let l = lo,
                h = hi,
                p = a[hi];

            do 
            {
                while (l < h && a[l] <= p) l++;
                while (h > l && a[h] >= p) h--;
                    
                if (l < h) 
                {
                    let t = a[l];
                    a[l] = a[h];
                    a[h] = t;
                }
                    
            } while (l < h);
                
            let t = a[l];
            a[l] = a[hi];
            a[hi] = t;
                
            qsort(a, lo, l-1);
            qsort(a, l+1, hi);
        }
    }

    qsort(array, 0, array.size() - 1);
}

\: filter (symbol[]; [symbol] syms)
{
    symbol[] outsyms;

    for_each (s; syms) 
    {
        let symname = symbol_name(s);

        let
            first = symname.substr(0,1),
            last = symname.substr(-1,1),
            first2 = symname.substr(0,2),
            typemod = symbol_is_type_modifier(s);

        if (last != "]" && last != ")" && last != "&" && 
            first != "?" && last != "*" && first2 != "__" &&
            !typemod)
        {
            outsyms.push_back(s);
        }
    }

    sort(outsyms);
    outsyms;
}

\: filter (symbol[]; symbol[] syms)
{
    symbol[] outsyms;

    for_each (s; syms) 
    {
        let symname = symbol_name(s),
            first = symname.substr(0,1),
            last = symname.substr(-1,1),
            first2 = symname.substr(0,2);

        if (last != "]" && last != ")" && last != "&" && 
            first != "?" && last != "*" && first2 != "__")
        {
            outsyms.push_back(s);
        }
    }

    sort(outsyms);
    outsyms;
}

class: DocPage : QWebEnginePage
{
    (void; qt.QUrl) _f;

    method: DocPage (DocPage; QObject parent, (void;QUrl) F)
    {
        QWebEnginePage.QWebEnginePage(this, parent);
        this._f = F;
    }

    method: acceptNavigationRequest (bool; QUrl url, int type, bool isMainFrame)
    {
        this._f(url);

        let scheme = url.scheme();

        // If the scheme is not http, Qt5 will try to resolve
        // the URL using Operating system handler. To prevent that,
        // return false to say that this URL will be handled by our linkHandler.
        // https://doc.qt.io/qt-5/qwebenginepage.html#acceptNavigationRequest
        if (scheme == "mudoc")
        {
            return false;
        }

        return true;
    }
}

class: DocModel : QAbstractItemModel
{
    ConstantSymbol    := 1;
    VariableSymbol    := 2;
    FunctionSymbol    := 3;
    MethodSymbol      := 4;
    TypeSymbol        := 5;
    ModuleSymbol      := 6;

    class: SymbolNode
    {
        int          _row;
        symbol       _symbol;
        SymbolNode   _parent;
        SymbolNode[] _children;
        int          _symbolType;
    }

    \: childrenOfNode (SymbolNode[]; SymbolNode node)
    {
        if (node._children eq nil)
        {
            node._children = SymbolNode[]();
            
            if (node._symbolType != FunctionSymbol &&
                node._symbolType != MethodSymbol)
            {
                let ss = symbol_symbols_in_scope(node._symbol);

                for_each (s; filter(ss))
                {
                    let child = SymbolNode(node._children.size(), s, node);
                    node._children.push_back(child);

                    for_each (o; symbol_overloaded_symbols(s))
                    {
                        for_each (Ft; [(symbol_is_method, MethodSymbol),
                                       (symbol_is_function, FunctionSymbol),
                                       (symbol_is_symbolic_constant, ConstantSymbol),
                                       (symbol_is_type, TypeSymbol),
                                       (symbol_is_module, ModuleSymbol),
                                       (symbol_is_variable, VariableSymbol)])
                        {
                            let (F, v) = Ft;

                            if (F(o) && v > child._symbolType ) 
                            {
                                child._symbolType = v; 
                                child._symbol = o;
                                break;
                            }
                        }
                    }
                }
            }
        }
        
        return node._children;
    }

    SymbolNode _root;
    QVariant _typeIcon;
    QVariant _moduleIcon;
    QVariant _functionIcon;
    QVariant _methodIcon;
    QVariant _symbolIcon;
    QVariant _constantIcon;

    method: DocModel (DocModel; QObject parent, string iconLocation)
    {
        QAbstractItemModel.QAbstractItemModel(this, parent);
        _root = SymbolNode(0, symbol_from_name(intern_name("")), nil);

        \: icon (QVariant; string file)
        {
            QVariant(QIcon("%s/%s" % (iconLocation, file)));
        }

        _typeIcon     = icon("type.png");
        _moduleIcon   = icon("module.png");
        _functionIcon = icon("lambda.png");
        _methodIcon   = icon("method.png");
        _symbolIcon   = icon("symbol.png");
        _constantIcon = icon("constant.png");
    }

    method: findInSymbolNode (SymbolNode; SymbolNode root, symbol s)
    {
        if (root eq nil && s == _root._symbol) return _root;
        let overloads = symbol_overloaded_symbols(s);

        for_each (child; childrenOfNode(root)) 
        {
            for_each (o; overloads) if (child._symbol == o) return child;
        }

        return nil;
    }

    method: indexOfSymbol (QModelIndex; symbol s)
    {
        [symbol] path;
        let root = _root._symbol;

        for (symbol sym = s; !symbol_is_nil(sym); sym = symbol_scope(sym))
        {
            path = sym : path;
        }

        QModelIndex current = QModelIndex();
        SymbolNode node = _root;

        for_each (sym; tail(path))
        {
            node = findInSymbolNode(node, sym);
            if (node eq nil) return QModelIndex(); 
            current = index(node._row, 0, current);
        }

        return current;
    }

    method: columnCount (int; QModelIndex parent) { 1; }

    method: data (QVariant; QModelIndex index, int role)
    {
        if (index.isValid())
        {
            SymbolNode node = index.internalPointer();

            if (role == Qt.DisplayRole)
            {
                return QVariant(symbol_name(node._symbol));
            }
            else if (role == Qt.DecorationRole)
            {
                if (node._symbolType == FunctionSymbol) return _functionIcon;
                if (node._symbolType == MethodSymbol) return _methodIcon;
                if (node._symbolType == ModuleSymbol) return _moduleIcon;
                if (node._symbolType == ConstantSymbol) return _constantIcon;
                if (node._symbolType == TypeSymbol) return _typeIcon;
                if (node._symbolType == VariableSymbol) return _symbolIcon;
                // case (node._symbolType)
                // {
                //     FunctionSymbol  -> { return _functionIcon; }
                //     MethodSymbol    -> { return _methodIcon; }
                //     ModuleSymbol    -> { return _moduleIcon; }
                //     ConstantSymbol  -> { return _constantIcon; }
                //     TypeSymbol      -> { return _typeIcon; }
                //     VariableSymbol  -> { return _symbolIcon; }
                //     _               -> { return QVariant(); }
                // }
            }
        }

        return QVariant();
    }
    
    method: index (QModelIndex; int row, int column, QModelIndex parent)
    {
        if (column != 0) 
        {
            return QModelIndex();
        }
        else if (!parent.isValid())
        {
            let children = childrenOfNode(_root);

            return if row >= children.size()
                then QModelIndex()
                else createIndex(row, column, children[row]);
        }
        else
        {
            SymbolNode node = parent.internalPointer();
            let children = childrenOfNode(node);

            return if row >= children.size() 
                      then QModelIndex()
                      else createIndex(row, column, children[row]);
        }
    }

    method: parent (QModelIndex; QModelIndex index) 
    {
        if (index.isValid())
        {
            SymbolNode node = index.internalPointer();

            return if (node._parent eq _root || node._parent eq nil) 
                    then QModelIndex()
                    else createIndex(node._parent._row, 0, node._parent);
        }
        else
        {
            return QModelIndex();
        }
    }

    method: rowCount (int; QModelIndex parent) 
    { 
        if (parent.column() > 0) return 0;

        if (parent.isValid())
        {
            SymbolNode node = parent.internalPointer();
            return childrenOfNode(node).size();
        }
        else
        {
            return childrenOfNode(_root).size();
        }
    }
}

documentation: """
A QWidget which holds a navigation column view to select symbols and a
QWebEngineView area to show documentation on the selected symbol(s).
""";

class: DocBrowser : QWidget
{
    use io;
    use encoding;

    QSplitter       _splitter;
    QColumnView     _columnView;
    QWebEngineView  _webView;
    QWebEnginePage  _webPage;
    DocModel        _model;
    QUrl            _baseUrl;
    QUrl            _lastHandledLink;
    [(QUrl,QPoint)] _backHistory;
    [(QUrl,QPoint)] _forwardHistory;

    \: span (string; string s, string sclass) { "<span class=\"%s\">%s</span>" % (sclass, s); }
    \: typespan (string; string s) { span(s, "mutype"); }
    \: paramspan (string; string s) { span(s, "muparam"); }
    \: symbolspan (string; string s) { span(s, "musymbol"); }
    \: defaultvaluespan (string; string s) { span(s, "mudefault"); }
    \: functionspan (string; string s) { span(s, "mufunction"); }
    \: methodspan (string; string s) { span(s, "mumethod"); }

    \: findFirstSentence (string; string s)
    {
        let array = string_to_utf8(s);
        int stop = 0;

        for_index (i; array)
        {
            if (array[i] == byte('.') ||
                (i > 0 && array[i-1] == '\n' && array[i] == '\n')) break;
            stop = i;
        }

        s.substr(0, stop+2);
    }

    method: symbolInfo (void; symbol s, osstream str)
    {
        print(str, "<h2><img src=\"%s\"><span class=\"hsymtype\">Symbol:</span> %s</h2>\n" % ("symbol.png", symbolspan(symbol_name(s))));
        let d = symbol_documentation(s);
        if (d != "") print(str, "%s\n" % toHTML(d));
    }

    method: functionInfo (void; symbol s, osstream str, string imgname, string title)
    {
        let f = function_from_symbol(s),
            (rtype, ptypes, params) = function_signature(f);


        if (symbol_name(s) == symbol_name(rtype))
        {
            title = "Constructor";
        }

        print(str, "<h2><img src=\"%s\"><span class=\"hsymtype\">%s:</span> %s</h2>\n" % (imgname, title, functionspan(symbol_name(s))));
        print(str, "<p>Path: <b>%s</b></p>\n" % symbol_fully_qualified_name(s));

        print(str, "<pre class=\"signature\">");


        print(str, "%s (<a href=\"mudoc:///%s\">%s</a>; " % 
              (functionspan(symbol_name(f)), 
               symbol_fully_qualified_name(rtype),
               typespan(symbol_fully_qualified_name(rtype))));

        if (params neq nil)
        {
            let indent = symbol_name(f).size() + 2,
                indstr = ("%%%ds" % indent) % " ";
            runtime.parameter_symbol[] parray;
            runtime.type_symbol[] tarray;

            for_each (p; params) parray.push_back(p);
            for_each (t; ptypes) tarray.push_back(t);

            for_index (i; parray) 
            {
                let p = parray[i],
                    t = tarray[i];

                if (i == 0) print(str, "\n");

                if (symbol_is_nil(p))
                {
                    print(str, "%s<a href=\"mudoc:///%s\">%s</a>" 
                          % (indstr, 
                             symbol_fully_qualified_name(t),
                             typespan(symbol_fully_qualified_name(t))));

                    if (i < tarray.size() - 1) print(str, ",\n");
                }
                else
                {
                    print(str, "%s<a href=\"mudoc:///%s\">%s</a> %s" 
                          % (indstr,
                             symbol_fully_qualified_name(t),
                             typespan(symbol_fully_qualified_name(t)),
                             paramspan(symbol_name(p))));

                    let dv = parameter_default_value_as_string(p);
                    if (dv neq nil) print(str, " = %s" % defaultvaluespan(dv));
                    if (i < parray.size() - 1) print(str, ",\n");
                }
            }
        }

        print(str, ")\n");
        print(str, "</pre>");

        let d = symbol_documentation(s);

        if (d != "") 
        {
            print(str, "<h2>Description</h2>\n");
            print(str, "%s\n" % toHTML(d));
        }
    }

    method: outputFuncTable (void; 
                             osstream str,
                             string title,
                             string cssclass, 
                             bool hidethis,
                             bool hidereturn,
                             [function_symbol] funcs)
    {
        print(str, "<h2>%s</h2>\n" % title);
        print(str, "<table class=\"%s\">" % cssclass);

        for_each (f; funcs)
        {
            let (rtype, ptypes, params) = function_signature(f);

            print(str, "<tr>");
            print(str, "<td class=\"functioncell\"><a href=\"mudoc:///%s\">" % symbol_fully_qualified_name(f));
            print(str, functionspan(symbol_name(f)));
            print(str, "</a></td>");
            print(str, "<td class=\"paramlistcell\">");
            if (hidereturn) print(str, "  ( ");
            else print(str, "  ( <a href=\"mudoc:///%s\">%s</a> ; " 
                       % (symbol_fully_qualified_name(rtype),
                          typespan(symbol_fully_qualified_name(rtype))));

            \: printParam (void; 
                           [type_symbol] typelist,
                           [parameter_symbol] paramlist)
            {
                if (typelist neq nil)
                {
                    let t : trest = typelist,
                        p : prest = paramlist;
                    
                    print(str, "<a href=\"mudoc:///%s\">%s</a>" %
                          (symbol_fully_qualified_name(t), 
                           typespan(symbol_fully_qualified_name(t))));
                    
                    if (!symbol_is_nil(p))
                    {
                        print(str, " %s" % paramspan(symbol_name(p)));
                        let dv = parameter_default_value_as_string(p);
                        if (dv neq nil) print(str, " = %s" % dv);
                    }
                    
                    if (trest neq nil) print(str, ", ");
                    printParam(trest, prest);
                }
            }
                    
            try
            {
                printParam(if hidethis then tail(ptypes) else ptypes, 
                           if hidethis then tail(params) else params);
            }
            catch (...)
            {
                print(str, " *bad* ");
            }
            
            print(str, " )");
            print(str, "</td>");
            print(str, "</tr>");
        }

        print(str, "</table>\n");
    }

    method: outputFuncTable (void; 
                             osstream str,
                             string title,
                             string cssclass, 
                             bool hidethis,
                             bool hidereturn,
                             function_symbol[] funcs)
    {
        [function_symbol] list = nil;
        for_each (f; funcs) list = f : list;
        outputFuncTable(str, title, cssclass, hidethis, hidereturn, list);
    }

    method: outputTypeTable (void;
                             ostream str,
                             type_symbol[] types)
    {
        if (!types.empty())
        {
            print(str, "<h2>Types</h2>");
            print(str, "<table class=\"mutypetable\">");
            
            for_each (t; types)
            {
                print(str, "<tr>");
                print(str, "<td class=\"typeKindCell\">%s</td>" % kindNameOfType(t));
                print(str, "<td class=\"typeNameCell\"><a href=\"mudoc:///%s\">%s</a></td>" 
                      % (symbol_fully_qualified_name(t), typespan(symbol_name(t))));
                print(str, "</tr>");
            }
            
            print(str, "</table>");
        }
    }

    method: kindNameOfType (string; type_symbol t)
    {
        for_each (T; [(type_is_union, "Union"),
                      (type_is_union_tag, "Union Tag"),
                      (type_is_class, "Class"),
                      (type_is_interface, "Interface"),
                      (type_is_opaque, "Opaque Type")])
        {
            let (F, n) = T;
            if (F(t)) return n;
        }

        return "Type";
    }

    method: typeInfo (void; symbol s, osstream str)
    {
        let t           = type_from_symbol(s),
            isclass     = type_is_class(t),
            isunion     = type_is_union(t),
            istag       = type_is_union_tag(t),
            isinterface = type_is_interface(t),
            typekind    = kindNameOfType(t);

        print(str, "<h2><img src=\"type.png\"><span class=\"hsymtype\">%s:</span> %s</h2>\n" % (typekind, symbol_name(s)));
        print(str, "<p>Path: <b>%s</b></p>" % symbol_fully_qualified_name(s));

        let d = symbol_documentation(s);

        if (d != "")
        {
            let first = findFirstSentence(d);
            print(str, "<p>");
            print(str, toHTML(first));

            if (d.size() <= first.size() + 3) d = "";
            else print(str, " <a href=\"mudoc:///%s#Desc\"> More Below...</a></p>" 
                       % symbol_fully_qualified_name(s));
        }

        let (constructors, labels) = type_structure_info(t);

        if (isunion || istag || isclass || isinterface)
        {
            let children = symbol_symbols_in_scope(t);
            type_symbol[] types;

            for_each (sym; children)
            {
                if (symbol_is_type(sym)) 
                {
                    let t = type_from_symbol(sym);
                    if (!type_is_reference_type(t)) types.push_back(t);
                }
            }

            if (isclass)
            {
                if (labels neq nil)
                {
                    print(str, "<pre class=\"muclassdef\">\n");
                    print(str, "class: %s\n" % typespan(symbol_name(s)));
                    print(str, "{\n");

                    bool hasdocs = false;

                    for_each (l; labels)
                    {
                        let d = symbol_documentation(l);
                        if (d != "") hasdocs = true;
                        print(str, "    <a href=\"mudoc:///%s\">%s</a> %s\n" 
                              % (symbol_fully_qualified_name(variable_type(l)),
                                 typespan(symbol_name(variable_type(l))), 
                                 symbol_name(l)));
                    }

                    print(str, "}\n");
                    print(str, "</pre>\n");

                    if (hasdocs)
                    {
                        print(str, "<h3>Field Descriptions</h3>");
                        print(str, "<table class=\"muparamtable\">");

                        for_each (l; labels)
                        {
                            let d = symbol_documentation(l);

                            print(str, 
                                  ("<tr>" + 
                                   "<td class=\"paramvartd\">%s</td>" +
                                   "<td class=\"paramtypetd\">%s</td>" +
                                   "<td class=\"paramdoctd\">%s</td>" +
                                   "</tr>")
                                  % (symbol_name(l),
                                     typespan(symbol_name(variable_type(l))), 
                                     d));
                        }

                        print(str, "</table>");
                    }
                }
                else
                {
                    print(str, "<div class=\"nostate\">No visible state</div>");
                }
            }
            else if (isunion)
            {
                print(str, "<pre class=\"muuniondef\">\n");
                print(str, "union: %s\n" % typespan(symbol_name(s)));
                print(str, "{\n");
                
                bool first = true;

                for_each (c; constructors)
                {
                    let (rtype, ptypes, params) = function_signature(c);

                    print(str, "    %s %s %s\n" % (if first then " " else "|",
                                                   functionspan(symbol_name(c)), 
                                                   if ptypes neq nil 
                                                     then typespan(symbol_name(head(ptypes)))
                                                         else ""));

                    first = false;
                }

                print(str, "}\n");
                print(str, "</pre>\n");
            }

            outputTypeTable(str, types);

            if (!isinterface)
                outputFuncTable(str, 
                                "Constructors", 
                                "typeconstrtable",
                                isclass,
                                true,
                                constructors);

            if (isclass || isinterface)
            {
                function_symbol[] methods;
                function_symbol[] functions;
                function_symbol[] callableFunctions;

                for_each (sym0; symbol_symbols_in_scope(s))
                {                
                    for_each (sym; symbol_overloaded_symbols(sym0))
                    {
                        if (symbol_is_function(sym))
                        {
                            let f = function_from_symbol(sym),
                                ismethod = symbol_is_method(sym),
                                isconstr = symbol_name(sym) == symbol_name(s);

                            if (symbol_name(sym) == "__allocate") continue;
                        
                            if (!isconstr)
                            {
                                let (rtype, ptypes, params) = function_signature(f);
                            
                                bool iscallable = params neq nil &&
                                    isclass &&
                                    !symbol_is_nil(head(params)) &&
                                    symbol_name(head(params)) == "this" &&
                                    symbol_name(variable_type(head(params))) == symbol_name(t);
                            
                                if (ismethod) methods.push_back(f);
                                else if (iscallable) callableFunctions.push_back(f);
                                else functions.push_back(f);
                            }
                        }
                    }
                }
                    
                if (!methods.empty())
                    outputFuncTable(str,
                                    "Methods",
                                    "methodtable",
                                    isclass,
                                    false,
                                    methods);

                if (!callableFunctions.empty())
                    outputFuncTable(str,
                                    "Method-Like Functions",
                                    "methodtable",
                                    isclass,
                                    false,
                                    callableFunctions);

                if (!functions.empty())
                    outputFuncTable(str,
                                    "Functions",
                                    "functiontable",
                                    false,
                                    false,
                                    functions);
            }
        }

        if (d != "") 
        {
            print(str, "<h2><a name=\"Desc\">Description</a></h2>\n");
            print(str, "%s\n" % toHTML(d));
        }
    }

    method: moduleInfo (void; symbol s, osstream str)
    {
        print(str, "<h2><img src=\"module.png\"><span class=\"hsymtype\">Module:</span> %s</h2>\n" % symbol_name(s));
        print(str, "<p>Path: <b>%s</b></p>" % symbol_fully_qualified_name(s));

        function_symbol[] funcs;
        type_symbol[] types;
        variable_symbol[] vars;
        let d = symbol_documentation(s);

        if (d != "")
        {
            let first = findFirstSentence(d);
            print(str, "<p>");
            print(str, toHTML(first));

            if (d.size() <= first.size() + 3) d = "";
            else print(str, " <a href=\"mudoc:///%s#Desc\"> More Below...</a></p>" 
                       % symbol_fully_qualified_name(s));
        }

        for_each (sym0; symbol_symbols_in_scope(s))
        {                
            for_each (sym; symbol_overloaded_symbols(sym0))
            {
                if (symbol_is_function(sym))
                {
                    let f = function_from_symbol(sym),
                        (rtype, ptypes, params) = function_signature(f);

                    if (symbol_name(rtype) != symbol_name(f)) funcs.push_back(f);
                }
                else if (symbol_is_type(sym))
                {
                    let t = type_from_symbol(sym);
                    if (!type_is_reference_type(t)) types.push_back(t);
                }
                else if (symbol_is_variable(sym))
                {
                    let v = variable_from_symbol(sym);
                    vars.push_back(v);
                }
            }
        }

        outputTypeTable(str, types);

        if (!funcs.empty())
            outputFuncTable(str,
                            "Functions",
                            "functiontable",
                            false,
                            false,
                            funcs);

        if (d != "") 
        {
            print(str, "<h2><a name=\"Desc\">Description</a></h2>\n");
            print(str, "%s\n" % toHTML(d));
        }
    }

    method: updateWebView (void; DocModel.SymbolNode node)
    {
        if (node eq nil) return;
        let syms = symbol_overloaded_symbols(node._symbol);

        osstream str;

        print(str, "<html>");
        print(str, css);
        print(str, "<body>");

        let outputlist = [(symbol_is_method, functionInfo(,,"method.png", "Method")),
                          (symbol_is_function, functionInfo(,,"lambda.png", "Function")),
                          (symbol_is_module, moduleInfo),
                          (symbol_is_type, typeInfo),
                          (symbol_is_symbolic_constant, symbolInfo),
                          (symbol_is_variable, symbolInfo)],
            typelist    = [(symbol_is_type, typeInfo)];

        for_each (s; syms)
        {
            for_each (Ft; if node._symbolType == DocModel.TypeSymbol then typelist else outputlist)
            {
                let (Ftest, Finfo) = Ft;

                if (Ftest(s)) 
                {
                    Finfo(s, str);
                    print(str, "<hr>");
                    break;
                }
            }
        }

        print(str, "</html>");
        print(str, "</body>");

        _webView.setHtml(string(str), _baseUrl);
    }

    method: updateWebView2 (void; QModelIndex current, QModelIndex previous)
    {
        DocModel.SymbolNode node = current.internalPointer();

        if (node neq nil)
        {
            handleLink(QUrl("mudoc:///%s" % symbol_fully_qualified_name(node._symbol)));
        }
        else
        {
            print("ERROR: internal error in updateWebView2\n");
        }
    }


    method: handleLink (void; QUrl url)
    {
        let scheme = url.scheme();

        //
        //  As we "leave" a link, remember the current scroll position so we
        //  can reset it if/when we return.
        //

/*
        if (_webView neq nil && _webView.page() neq nil && _webView.page().currentFrame() neq nil)
        {
            if (_backHistory neq nil &&
                _lastHandledLink.toString(QUrl.None) == head(_backHistory)._0.toString(QUrl.None))
            {
                let l : rest = _backHistory;
                l = (l._0, _webView.page().currentFrame().scrollPosition());
                _backHistory = l : rest;
            }
            if (_forwardHistory neq nil &&
                _lastHandledLink.toString(QUrl.None) == head(_forwardHistory)._0.toString(QUrl.None))
            {
                let l : rest = _forwardHistory;
                l = (l._0, _webView.page().currentFrame().scrollPosition());
                _forwardHistory = l : rest;
            }
        }
*/

        _lastHandledLink = url;

        if (scheme == "mudoc")
        {
            let symname = url.path().substr(1,10000),
                parts = symname.split("#"),
                sym = symbol_from_name(intern_name(parts[0])),
                anchor = if parts.size() > 1 then parts[1] else "";

            if (!symbol_is_nil(sym))
            {
                let i = _model.indexOfSymbol(sym);

                if (!i.isValid()) 
                {
                    //print("ERROR: i isn't valid for %s\n" % symbol_fully_qualified_name(sym));
                    return;
                }

                _columnView.setCurrentIndex(i);
                DocModel.SymbolNode node = i.internalPointer();
                updateWebView(node);

                if (_backHistory eq nil ||
                    url.toString(QUrl.None) != head(_backHistory)._0.toString(QUrl.None))
                {
                    _backHistory = (url,QPoint()) : _backHistory;
                }
            }
            else
            {
                print("WARNING: couldn't find \"%s\" from \"%s\"\n" 
                      % (symname, url.toString(QUrl.None)));
            }
        }
        else if (scheme == "http" || scheme == "https" || scheme == "file")
        {
            _webView.setUrl(url);

            if (_backHistory eq nil ||
                url.toString(QUrl.None) != head(_backHistory)._0.toString(QUrl.None))
            {
                _backHistory = (url,QPoint()) : _backHistory;
            }
        }
        else if (scheme == "musearch")
        {
            let text = url.path().substr(1,10000);
            search(text, url);

            if (_backHistory eq nil ||
                url.toString(QUrl.None) != head(_backHistory)._0.toString(QUrl.None))
            {
                _backHistory = (url,QPoint()) : _backHistory;
            }
        }
    }

    method: back (void;)
    {
        if (_backHistory neq nil && tail(_backHistory) neq nil)
        {
            let c : l : rest = _backHistory;
            _backHistory = rest;
            _forwardHistory = c : _forwardHistory;

            handleLink(l._0);
/*
            if (_webView neq nil && _webView.page() neq nil && _webView.page().currentFrame() neq nil)
            {
                _webView.page().currentFrame().setScrollPosition(l._1);
            }
*/
        }
    }

    method: search (void; string text, QUrl url)
    {
        if (text eq nil || text == "")
        {
            _webView.setHtml("", _baseUrl);
            return;
        }

        try
        {
            let re = regex(string.join(text.split(" "), "|"), regex.IgnoreCase);
            let syms = match_regex(re);

            osstream str;
            print(str, "<html>");
            print(str, css);
            print(str, "<body>");
            print(str, "<h2>Search Results for \"%s\"</h2>" % text);

            if (syms neq nil)
            {

                print(str, "<table class=\"basictable\">");

                for_each (s; syms)
                {
                    if (symbol_is_type(s))
                    {
                        let t = type_from_symbol(s);
                        if (type_is_reference_type(t)) continue;
                        let c = symbol_name(s)[-1];
                        if (c == ')' || c == ']') continue;
                    }

                    print(str, "<tr>");
                    print(str, "<td>");

                    for_each (Ft; [(symbol_is_method, "method.png"),
                                   (symbol_is_function, "lambda.png"),
                                   (symbol_is_symbolic_constant, "constant.png"),
                                   (symbol_is_type, "type.png"),
                                   (symbol_is_module, "module.png"),
                                   (symbol_is_variable, "symbol.png")])
                    {
                        let (F, img) = Ft;
                        if (F(s)) { print(str, "<img src=\"%s\">" % img); break; }
                    }

                    print(str, "</td>");
                    print(str, "<td>");

                    print(str, "<a href=\"mudoc:///%s\">%s</a>\n" %
                          (symbol_fully_qualified_name(s),
                           symbol_fully_qualified_name(s)));

                    if (re.match(symbol_documentation(s)))
                    {
                        print(str, "<br><span class=\"subtext\">Match found in documentation</span>");
                    }

                    print(str, "</td>");

                    print(str, "</tr>");
                }

                print(str, "</table>");
            }

            print(str, "</body>");
            print(str, "</html>");
            _webView.setHtml(string(str), _baseUrl);
        }
        catch (...)
        {
            _webView.setHtml("<h1>Error during search</h1>", _baseUrl);
        }

    }

    method: forward (void;)
    {
        if (_forwardHistory neq nil)
        {
            let l : rest = _forwardHistory;
            _forwardHistory = rest;
            handleLink(l._0);
/*
            if (_webView neq nil && _webView.page() neq nil && _webView.page().currentFrame() neq nil)
            {
                _webView.page().currentFrame().setScrollPosition(l._1);
            }
*/
        }
    }

    method: DocBrowser (DocBrowser; QWidget parent, QUrl baseUrl)
    {
        QWidget.QWidget(this, parent, Qt.Widget);
        _model = DocModel(this, baseUrl.toLocalFile());
        _baseUrl = baseUrl;

        _splitter = QSplitter(this);
        _splitter.setOrientation(Qt.Vertical);
        
        _columnView = QColumnView(_splitter);
        _columnView.setModel(_model);
        _columnView.setSelectionMode(QAbstractItemView.SingleSelection);
        _columnView.setSelectionBehavior(QAbstractItemView.SelectRows);
        
        connect(_columnView.selectionModel(), 
                QItemSelectionModel.currentChanged, 
                updateWebView2);

        _webView = QWebEngineView(_splitter);
        _webPage = DocPage(_webView, \: (void; QUrl url) { this.handleLink(url); } );
        _webView.setPage(_webPage);
        _webPage.setHtml(startHTML % css, baseUrl);

        _splitter.addWidget(_columnView);
        _splitter.addWidget(_webView);

        let vbox = QVBoxLayout(this);
        vbox.addWidget(_splitter);

        setMinimumWidth(800);
        setMinimumHeight(600);
        _splitter.setStretchFactor(1, 3);
    }
}

use rvtypes;
use commands;
require frameburn;
require simpleslate;
require matte;
require watermark;
require lutgen;
require bug;


class: DocBrowserMode : MinorMode
{ 
    QMainWindow _window;
    DocBrowser _browser;
    QLayout     _searchLayout;
    QWidget     _searchWidget;
    QLabel      _searchLabel;
    QLineEdit   _searchEdit;
    
    method: close (void; Event event)
    {
        _window.close();
        event.reject();
    }

    documentation: "Needed to sub-class just to get hideEvent()";

    class: DocBrowserWindow : QMainWindow
    {
        DocBrowserMode  _mode;

        method: DocBrowserWindow(QWidget parent, int flags, DocBrowserMode mode)
        {
            _mode = mode;
            QMainWindow.QMainWindow(this, parent, flags);
        }

        method: hideEvent (void; QHideEvent event)
        {
            if (_mode._active) _mode.toggle();
        }
    }

    method: search (void;) 
    { 
        let text = _searchEdit.text();

        if (regex("^https?://").match(text) ||
            regex("^mudoc://").match(text))
        {
            _browser.handleLink(QUrl(text));
        }
        else
        {
            _browser.handleLink(QUrl("musearch:///" + text)); 
        }
    }

    method: DocBrowserMode (DocBrowserMode; string n)
    {
        init(n, 
             [("before-session-deletion", close, "Close browser")],
             nil, 
             nil,
             "z",
             9);  // even more "always last"

        let path = supportPath("doc_browser");

        _window = DocBrowserWindow(nil, Qt.Window, this);
        _browser = DocBrowser(_window, QUrl.fromLocalFile(path + "/"));
        _window.setObjectName("docBrowser");

        let toolbar       = _window.addToolBar("docBrowserToolbar"),
            backAction    = QAction(QIcon(":images/back_out.png"), "Back", _window),
            forwardAction = QAction(QIcon(":images/forwd_out.png"), "Forward", _window),
            backButton    = QToolButton(_window),
            forwardButton = QToolButton(_window);

        toolbar.setMovable(false);

        backButton.setObjectName("backButton");
        backButton.setDefaultAction(backAction);
        backButton.setProperty("tbstyle", QVariant("left"));

        forwardButton.setObjectName("forwardButton");
        forwardButton.setDefaultAction(forwardAction);
        forwardButton.setProperty("tbstyle", QVariant("right"));

        toolbar.addWidget(backButton);
        toolbar.addWidget(forwardButton);

        _searchWidget = QWidget(_window, 0); 
        _searchLabel = QLabel("Search:", _searchWidget, 0);
        _searchEdit = QLineEdit(_searchWidget);
        _searchLayout = QHBoxLayout(_searchWidget);
        _searchWidget.setMinimumWidth(120);
        _searchWidget.setLayout(_searchLayout);
        _searchLayout.addWidget(_searchLabel);
        _searchLayout.addWidget(_searchEdit);
        _searchLayout.setContentsMargins(10, 1, 10, 1);
        toolbar.addWidget(_searchWidget);

        _searchWidget.setObjectName("searchWidget");
        _searchLabel.setObjectName("searchLabel");

        connect(QApplication.instance(), QGuiApplication.lastWindowClosed, \: (void; ){ this._window.deleteLater();});

        connect(_searchEdit, QLineEdit.returnPressed, search);
        connect(backAction, QAction.triggered, \: (void; bool t) { this._browser.back(); });
        connect(forwardAction, QAction.triggered, \: (void; bool t) { this._browser.forward(); });

        _window.setCentralWidget(_browser);
        _window.show();
        _window.raise();
    }

    method: deactivate (void;)
    {
        if (_window neq nil) _window.hide();
    }

    method: activate (void;)
    {
        if (_window neq nil)
        {
            if (!_window.visible()) _window.show();
            _window.raise();
        }
    }
}

\: createMode (Mode;)
{
    return DocBrowserMode("doc_browser");
}

}

