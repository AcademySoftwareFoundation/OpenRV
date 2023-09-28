//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
module: session_manager {
use rvtypes;
use commands;
use extra_commands;
use qt;
use autodoc;
use rvui;
use app_utils;
require io;
require system;

NotASubComponent    := 0;
MediaSubComponent   := 1;
ViewSubComponent    := 2;
LayerSubComponent   := 3;
ChannelSubComponent := 4;

\: itemNode (string; QStandardItem item) 
{ 
    let d = item.data(Qt.UserRole + 2);
    if !d.isValid() || d.isNull() then "" else d.toString(); 
}

\: itemSubComponentTypeForName (int; string n)
{
    case (n)
    {
        "view"    -> { return ViewSubComponent; }
        "layer"   -> { return LayerSubComponent; }
        "channel" -> { return ChannelSubComponent; }
    }

    return NotASubComponent;
}


\: componentMatch (bool; string n, int c)
{
    return itemSubComponentTypeForName(n) == c;
}

\: itemSubComponentStringData (string; QStandardItem item, int n)
{
    let d = item.data(Qt.UserRole + n);
    if !d.isValid() || d.isNull() then "" else d.toString();
}

\: itemSubComponentMedia (string; QStandardItem item) { itemSubComponentStringData(item, 7); } 
\: itemSubComponentHash (string; QStandardItem item) { itemSubComponentStringData(item, 6); } 
\: itemSubComponentValue (string; QStandardItem item) { itemSubComponentStringData(item, 5); } 
\: itemParentNode (string; QStandardItem item) { itemSubComponentStringData(item, 1); }

\: itemSubComponentType (int; QStandardItem item)
{
    let d = item.data(Qt.UserRole + 4);
    if !d.isValid() || d.isNull() then NotASubComponent else d.toInt();
}

\: itemIsSubComponent (bool; QStandardItem item) 
{ 
    let d = item.data(Qt.UserRole + 4);
    !(!d.isValid() || d.isNull() || d.toInt() == 0);
}

\: includes (bool; QModelIndex[] array, QModelIndex item)
{
    for_each (a; array) if (a.row() == item.row()) return true;
    false;
}

\: contains (bool; string[] array, string value)
{
    for_each (x; array) if (x == value) return true;
    false;
}

\: indexOf (int; string[] array, string value)
{
    for_index (i; array) if (array[i] == value) return i;
    -1;
}

\: remove (string[]; string[] array, string value)
{
    string[] newArray;
    for_each (a; array) if (a != value) newArray.push_back(a);
    newArray;
}

\: sourceNodeOfGroup (string; string group)
{
    for_each (node; nodesInGroup(group))
    {
        let t = nodeType(node);
        if (t == "RVFileSource" || t == "RVImageSource") return node;
    }

    return nil;
}

\: hashedSubComponent (string; string media, string view, string layer)
{
    let v = if view neq nil && view == "" then "@." else view,
        l = if layer neq nil && layer == "" then "@." else layer;

    if v eq nil && l eq nil 
        then "%s!~!~" % media
        else if v eq nil 
            then "%s!~%s!~" % (media, l)
            else if l eq nil 
                    then "%s!~!~%s" % (media, v)
                    else "%s!~%s!~%s" % (media, l, v);
}

\: hashedSubComponent (string; QStandardItem item)
{
    let value   = itemSubComponentValue(item),
        subType = itemSubComponentType(item),
        parent  = item.parent(),
        pvalue  = itemSubComponentValue(parent);

    case (subType)
    {
        MediaSubComponent ->
        {
            return hashedSubComponent(value, nil, nil);
        }

        LayerSubComponent ->
        {
            let grandParent = parent.parent(),
                psubType    = itemSubComponentType(parent),
                arg0        = if psubType == ViewSubComponent 
                                 then itemSubComponentValue(grandParent)
                                 else pvalue,
                arg1        = if psubType == ViewSubComponent 
                                 then pvalue
                                 else nil;
            
            return hashedSubComponent(arg0, arg1, value);
        }

        ViewSubComponent ->
        {
            return hashedSubComponent(pvalue, value, nil);
        }
    }

    return "";
}

\: isSubComponentExpanded (bool; string node, QStandardItem item)
{
    let propName = "%s.sm_state.expandedSubState" % node,
        key = hashedSubComponent(item);

    if (propertyExists(propName))
    {
        let p = getStringProperty(propName);
        return contains(p, key);
    }

    false;
}

\: setSubComponentExpanded (void; string node, QStandardItem item, bool expanded)
{
    let propName = "%s.sm_state.expandedSubState" % node,
             key = hashedSubComponent(item);

    if (propertyExists(propName))
    {
        let p = getStringProperty(propName),
            hasit = contains(p, key);

        if (hasit && !expanded)
        {
            set(propName, remove(p, key));
        }
        else if (!hasit && expanded)
        {
            p.push_back(key);
            set(propName, p);
        }
    }
    else
    {
        set(propName, string[] {key});
    }
}

\: isExpandedInParent (bool; string node, string parent)
{
    let propName = "%s.sm_state.expandState" % node;

    if (propertyExists(propName))
    {
        let p = getStringProperty(propName);
        return contains(p, parent);
    }

    false;
}

\: setExpandedInParent (void; string node, string parent, bool expanded)
{
    let propName = "%s.sm_state.expandState" % node;

    if (propertyExists(propName)) 
    {
        let p = getStringProperty(propName),
            hasNode = contains(p, parent);

        if (hasNode && !expanded)
        {
            set(propName, remove(p, parent));
        }
        else if (!hasNode && expanded)
        {
            p.push_back(parent);
            set(propName, p);
        }
    }
    else
    {
        set(propName, parent);
    }
}

\: setToolTipProp (void; string node, string toolTip)
{
    let propName = "%s.sm_state.toolTip" % node;
    set (propName, toolTip);
}

\: toolTipFromProp (string; string node)
{
    string tip = nil;

    let propName = "%s.sm_state.toolTip" % node;

    if (propertyExists(propName))
    {
        try
        {
            tip = getStringProperty(propName).front();
        }
        catch (...) {;}
    }

    return tip;
}

\: sortKeyInParent (int; string node, string parent)
{
    let propNameParent = "%s.sm_state.sortKeyParent" % node,
        propNameKey    = "%s.sm_state.sortKey" % node,
        undefinedKey   = int.max - 100;

    if (propertyExists(propNameParent) && propertyExists(propNameKey))
    {
        try
        {
            let p    = getStringProperty(propNameParent),
                keys = getIntProperty(propNameKey),
                i    = indexOf(p, parent);
            
            return if i == -1 || keys.size() != p.size() then undefinedKey else keys[i];
        }
        catch (...)
        {
            ;
        }
    }

    return undefinedKey;
}

\: setSortKeyInParent (void; string node, string parent, int value)
{
    let propNameParent = "%s.sm_state.sortKeyParent" % node,
        propNameKey    = "%s.sm_state.sortKey" % node;

    if (propertyExists(propNameParent) && propertyExists(propNameKey))
    {
        try
        {
            let p    = getStringProperty(propNameParent),
                keys = getIntProperty(propNameKey),
                i    = indexOf(p, parent);

            if (p.size() == keys.size())
            {
                if (i == -1)
                {
                    p.push_back(parent);
                    keys.push_back(value);
                    set(propNameParent, p);
                    set(propNameKey, keys);
                }
                else
                {
                    keys[i] = value;
                    set(propNameKey, keys);
                }

                return;
            }
        }
        catch (...)
        {
            ;
        }
    }

    set(propNameParent, parent);
    set(propNameKey, value);
}

\: indexOfItem (int; string[] array, string item)
{
    for_index (i; array) if (array[i] == item) return i;
    return -1;
}

\: nodeFromIndex (string; 
                  QModelIndex index, 
                  QStandardItemModel model)
{
    let item = model.itemFromIndex(index);
    itemNode(item);
}

\: nodeInputs (string[]; string node)
{
    nodeConnections(node, false)._0;
}

\: map (void; [QStandardItem] items, (void; QStandardItem, bool) F, bool value)
{
    for_each (i; items) F(i, value);
}

\: map (void; [QStandardItem] items, (void; QStandardItem, int) F, int value)
{
    for_each (i; items) F(i, value);
}

\: addRow (void; QStandardItem item, [QStandardItem] children)
{
    let row = item.rowCount();
    int count = 0;
    for_each (i; children) item.setChild(row, count++, i);
}

\: setInputs (bool; string node, string[] inputs)
{
    let msg = testNodeInputs(node, inputs);

    if (msg neq nil)
    {
        alertPanel(false,
                   ErrorAlert,
                   "Some inputs are not allowed here",
                   msg,
                   "Ok", nil, nil);
    }
    else
    {
        setNodeInputs(node, inputs);
    }

    return msg eq nil;
}

\: removeInput (bool; string node, string inputNode)
{
    if (node != "")
    {
        let ins = nodeConnections(node)._0;
        string[] newInputs;
        for_each (node; ins) if (node != inputNode) newInputs.push_back(node);
        return setInputs(node, newInputs);
    }
    else
    {
        return true;
    }
}

\: hasInput (bool; string node, string inputNode)
{
    if (node eq nil || node == "") return true;
    let ins = nodeConnections(node)._0;
    for_each (n; ins) if (inputNode == n) return true;
    return false;
}

\: addInput (bool; string node, string inputNode)
{
    if (nodeExists(node))
    {
        let ins = nodeConnections(node)._0;
        string[] newInputs;
        for_each (n; ins) newInputs.push_back(n);
        newInputs.push_back(inputNode);
        return setInputs(node, newInputs);
    }
    else
    {
        return true;
    }
}

\: map ([QStandardItem]; QStandardItemModel model, (bool; QStandardItem) F, QStandardItem root = nil)
{
    \: mapOverItem ([QStandardItem]; 
                    QStandardItem item,
                    QStandardItemModel model,
                    (bool; QStandardItem) F,
                    [QStandardItem] list)
    {
        for (int i = 0, s = item.rowCount(); i < s; i++)
        {
            list = mapOverItem(item.child(i, 0), model, F, list);
        }

        if itemNode(item) != "" && F(item) then item : list else list;
    }

    [QStandardItem] list = nil;

    if (root eq nil)
    {
        for (int i = 0, s = model.rowCount(QModelIndex()); i < s; i++)
        {
            list = mapOverItem(model.item(i, 0), model, F, list);
        }
    }
    else
    {
        list = mapOverItem(root, model, F, list);
    }

    list;
}

\: itemOfNode (QStandardItem; QStandardItemModel model, string node)
{
    let items = map(model, \: (bool; QStandardItem i) { itemNode(i) == node && !itemIsSubComponent(i); });
    if items eq nil then nil else head(items);
}

\: subComponentItemsOfNode ([QStandardItem]; 
                            QStandardItemModel model, 
                            string node)
{
    map(model, \: (bool; QStandardItem i) 
        { 
            let subType = itemSubComponentType(i);
            return itemNode(i) == node && 
                   subType != NotASubComponent &&
                   subType != MediaSubComponent &&
                   i.index().column() == 0;
        });
}

\: assignSortOrder (void; QStandardItem root)
{
    if (root neq nil) 
    {
        try
        {
            let rootNode = itemNode(root);

            for (int i = 0, s = root.rowCount(), index = 0; i < s; i++)
            {
                let item = root.child(i, 0);

                if (item neq nil)
                {
                    let node = itemNode(item);
                    setSortKeyInParent(node, rootNode, index++);
                }
            }
        }
        catch (exception exc)
        {
            print("CAUGHT %s\n" % exc);
        }
    }
}

\: resizeColumns (void; QTreeView treeView, QStandardItemModel model)
{
    for (int i = 0, s = model.columnCount(QModelIndex()); i < s; i++)
    {
        treeView.resizeColumnToContents(i);
    }
}

\: isImageRequestPropEqual (bool; string name, string[] array)
{
    let pname = "#RVSource.request." + name,
        current = getStringProperty(pname);
    return contentsEqual(current, array);
}

\: setImageRequestProp (void; string name, string[] array)
{
    let pname = "#RVSource.request." + name,
        current = getStringProperty(pname);

    if (!contentsEqual(current, array))
    {
        set(pname, array);
        reload();
    }
}

\: setImageRequest (void; string[] value, bool toggle = true)
{
    let pname = "imageComponent";

    if (toggle && isImageRequestPropEqual(pname, value))
    {
        //
        // The purpose of this code is to deselect all selections on the
        // repetition of an image request i.e. if a selection is clicked a
        // second time.  We do this by clearing the selection properties
        //

        setImageRequestProp(pname, string[]());
    }
    else
    {
        setImageRequestProp(pname, value);
    }
}

\: subComponentPropValue (string[]; QStandardItem item)
{
    let t = itemSubComponentType(item);

    let result = string[]();
    case (t)
    {
        MediaSubComponent ->
        {
            ;
        }

        ViewSubComponent ->
        {
            result = string[] {"view", itemSubComponentValue(item)};
        }

        LayerSubComponent ->
        {
            let parent = item.parent();

            result = string[] {"layer", 
                             if itemSubComponentType(parent) == ViewSubComponent
                                then itemSubComponentValue(parent)
                                else "",
                             itemSubComponentValue(item) };
        }

        ChannelSubComponent ->
        {
            let parent = item.parent(),
                pvalue = subComponentPropValue(parent),
                s      = pvalue.size(),
                value  = itemSubComponentValue(item);

            assert(s == 0 || s == 2 || s == 3);

            case (s)
            {
                0 -> { result = string[] {"channel", "", "", value}; }
                2 -> { result = string[] {"channel", pvalue[1], "", value}; }
                3 -> { result = string[] {"channel", pvalue[1], pvalue[2], value}; }
            }
        }
    }

    result;
}

\: setNodeRequest (void; string node, string[] value)
{
    setStringProperty(node + ".request.imageComponent", value, true);
}

documentation: """
QStandardItemModel with modified drag and drop mime types.
""";

class: NodeModel : QStandardItemModel
{
    method: NodeModel (NodeModel; QObject parent)
    {
        QStandardItemModel.QStandardItemModel(this, parent);
    }

    method: mimeTypes (string[];) 
    {
        let m = QStandardItemModel.mimeTypes(this);
        string[] newTypes;
        for_each (x; m) newTypes.push_back(x);
        newTypes.push_back("text/uri-list");
        newTypes.push_back("text/plain");
        newTypes;
    }

    method: mimeData (QMimeData; QModelIndex[] indices)
    {
        let d = QStandardItemModel.mimeData(this, indices);
        QUrl[] urls;
        use io;
        osstream text;

        //
        //  rvnode URL looks like:
        //
        //      rvnode://RVID/NODETYPE/NODENAME/PATH/TO/MEDIA
        //
        //  RVID can be nothing or an open port on a machine and
        //  possibly user like rvnode://me@foo:12332/....  right now
        //  we only support the empty RVID
        //

        try
        {
            for_each (index; indices)
            {
                let n     = nodeFromIndex(index, this),
                    ntype = nodeType(n),
                    rvid  = "%s@%s:%s" % (remoteLocalContactName(), myNetworkHost(), myNetworkPort());

                if (ntype == "RVSourceGroup")
                {
                    let media = getStringProperty("%s_source.media.movie" % n);
                    print(text, "RVFileSource %s.media.movie = %s\n" % (n, media));

                    for_each (m; media)
                    {
                        urls.push_back(QUrl("rvnode://%s/%s/%s/%s" % (rvid, ntype, n, m)));
                    }
                }
                else
                {
                    print(text, "%s %s\n" % (ntype, n));
                    urls.push_back(QUrl("rvnode://%s/%s/%s" % (rvid, ntype, n)));
                }
            }

            d.setText(string(text));
            d.setUrls(urls);
        }
        catch (exception exc)
        {
            print("CAUGHT %s\n" % exc);
        }

        return d;
    }
}

documentation: """
In order to constraint what can be dragged/dropped to/from the session
manager the QTreeView is overriden to weed out QStandardItemModel
behavior we don't like.

For example, the worst case currently is unconditionally accepting
QStandardItems from models that have nothing to do with the session
manager.

Another issue is that QStandardItemModel seems to be in varying states of
inconsistancy during the whole drag and drop process. Esp when emitting its
itemChanged() signal. To get around this you have to set a timer to examine
its state after whenever it is that it becomes sane.
""";

class: NodeTreeView : QTreeView
{
    int                _dropAction;
    string[][]         _draggedNodePaths;
    bool               _draggingNonFolders;
    QStandardItemModel _viewModel;
    string[]           _sortFolders;
    QTimer             _sortTimer;
    QStandardItem      _foldersItem;

    method: sortFolderChildren (void; string folder)
    {
        if (nodeType(folder) == "RVFolderGroup") 
        {
            bool exists = false;
            for_each (i; _sortFolders) if (i == folder) { exists = true; break; }
            if (!exists) _sortFolders.push_back(folder);
        }
    }

    method: selectedNodePaths (string[][]; )
    {
        let indices = selectionModel().selectedIndexes();
        string[][] paths;

        for_each (index; indices)
        {
            if (index.column() == 0)
            {
                paths.push_back(string[]());

                do
                {
                    let item = _viewModel.itemFromIndex(index),
                        node = itemNode(item);
                    
                    paths.back().push_back(node);
                    index = index.parent();
                } 
                while (index != QModelIndex());
            }
        }

        return paths;
    }

    method: filteredDraggedPaths (string[][]; (bool; string[]) F )
    {
        string[][] filteredPaths;
        for_each (path; _draggedNodePaths)
            if (F(path)) filteredPaths.push_back(path);
        filteredPaths;
    }

    method: dragEnterEvent (void; QDragEnterEvent event)
    {
        let sourceWidget = event.source(),
            mimeData = event.mimeData();

        if (sourceWidget == this) 
        {
            _draggedNodePaths = selectedNodePaths();
            _draggingNonFolders = false;

            for_each (path; _draggedNodePaths)
            {
                if (nodeExists(path[0]) && nodeType(path[0]) != "RVFolderGroup")
                {
                    _draggingNonFolders = true;
                }
            }

            _foldersItem.setFlags(if _draggingNonFolders 
                                  then Qt.ItemIsEnabled
                                  else Qt.ItemIsDropEnabled | Qt.ItemIsEnabled);

            QAbstractItemView.dragEnterEvent(this, event);
        }
        else if (exists(sourceWidget))
        {
            ; // allow it to be rejected
        }
        else
        {
            print("No like source: %s\n" % string(sourceWidget));
            ; // don't accept it
            let formats = mimeData.formats();

            print("--formats--\n");
            for_each (f; formats) print("%s\n" % f);
            
            if (mimeData.hasUrls())
            {
                print("--urls--\n");
                for_each (u; event.mimeData().urls()) print("%s\n" % u.toString(QUrl.None));
            }

            if (mimeData.hasText())
            {
                print("--text--\n");
                print("%s\n" % mimeData.text());
            }
        }
    }

    method: dragMoveEvent (void; QDragMoveEvent event)
    {
        let index = indexAt(event.pos()),
            item  = _viewModel.itemFromIndex(index);

        if (item eq nil) 
        {
            event.ignore();
            return;
        }

        let node  = itemNode(item);

        if (item.column() != 0)
        {
            event.ignore();
            return;
        }

        if (event.dropAction() == Qt.CopyAction && nodeExists(node))
        {
            let outs = nodeConnections(node)._1,
                ntype = nodeType(node);

            for_each (path; _draggedNodePaths)
            {
                if (nodeExists(path[1]))
                {
                    if (ntype != "RVFolderGroup")
                    {
                        //
                        //  don't allow dropping on a non-folder sibling either 
                        //  this is basically a reorder/copy
                        //

                        for_each (out; outs) 
                        {
                            if (out == path[1])
                            {
                                event.ignore();
                                return;
                            }
                        }
                    }

                    if (path[1] == node)
                    {
                        event.ignore();
                        return;
                    }
                }
            }
        }

        QTreeView.dragMoveEvent(this, event);
    }

    method: dropEvent (void; QDropEvent event)
    {
        _dropAction = event.dropAction();
        QTreeView.dropEvent(this, event);

        _draggedNodePaths = nil;
        _dropAction = Qt.IgnoreAction;
        // yet another timer to wait for the model to be in a sane state
        // for some reason you can use 0 for the timeout
        _sortTimer.start(if globalConfig.os == "WINDOWS" then 200 else 100);
    }

    method: sortFolders (void;)
    {
        for_each (folder; _sortFolders)
        {
            let item = itemOfNode(_viewModel, folder);
            if (item neq nil) assignSortOrder(item);
        }

        _sortFolders.clear();
    }

    method: NodeTreeView (NodeTreeView; QWidget parent)
    {
        QTreeView.QTreeView(this, parent);
        _sortFolders = string[]();
        _sortTimer = QTimer(this);
        _sortTimer.setSingleShot(true);
        connect(_sortTimer, QTimer.timeout, sortFolders);
    }
}

documentation: """
Override the QListView in order to force certain drop behaviors. Esp
when the drop is coming from the views in the session manager (it
should always be a copy from there).
""";


class: InputsView : QListView
{
    QAbstractItemView _viewTreeView;
    QTimer            _dropTimer;

    method: dragEnterEvent (void; QDragEnterEvent event)
    {
        if (event.source() == _viewTreeView) 
        {
            // force a copy from the tree view
            event.setDropAction(Qt.CopyAction);
        }

        QAbstractItemView.dragEnterEvent(this, event);
    }

    method: dropEvent (void; QDropEvent event)
    {
        QListView.dropEvent(this, event);
        if (event.source() == _viewTreeView)
        {
            // update the tree if the drop came from there
            _dropTimer.start(if globalConfig.os == "WINDOWS" then 200 else 100);
        }
    }

    method: InputsView (InputsView; 
                        QAbstractItemView treeView,
                        QWidget parent,
                        (void;) dropCleanup = nil)
    {
        _viewTreeView = treeView;
        QListView.QListView(this, parent);
        _dropTimer = QTimer(this);
        _dropTimer.setSingleShot(true);
        if (dropCleanup neq nil) connect(_dropTimer, QTimer.timeout, dropCleanup);
    }
}

class: EventFilter : QObject
{
    method: EventFilter (EventFilter; QWidget parent)
    {
        QObject.QObject(this, parent);
    }

    method: eventFilter (bool; QObject obj, QEvent event)
    {
        let view = mainViewWidget();
        return view.eventFilter(obj, event);
    }
}

documentation: """
SessionManagerMode controls UI for managing viewable nodes in the
session and a user interface to edit the currently viewed node. New
viewables can be created and their inputs changed and reordered. 
""";

class: SessionManagerMode : MinorMode
{ 
    QDockWidget        _dockWidget;
    EventFilter        _eventFilter;
    QWidget            _baseWidget;
    QSplitter          _splitter;
    QWidget            _treeViewBase;
    NodeTreeView       _viewTreeView;
    QStandardItemModel _viewModel;
    QStandardItemModel _inputsModel;
    QToolButton        _addButton;
    QToolButton        _folderButton;
    QToolButton        _deleteButton;
    QToolButton        _configButton;
    QToolButton        _editViewInfoButton;
    QToolButton        _homeButton;
    QWidget            _inputsViewBase;
    InputsView         _inputsView;
    QTabWidget         _tabWidget;
    QToolButton        _orderUpButton;
    QToolButton        _orderDownButton;
    QToolButton        _sortAscButton;
    QToolButton        _sortDescButton;
    QToolButton        _inputsDeleteButton;
    QTreeWidget        _uiTreeWidget;
    QTreeWidgetItem[]  _editors;
    (string,QIcon)[]   _typeIcons;
    QIcon              _unknownTypeIcon;
    QIcon              _viewIcon;
    QIcon              _layerIcon;
    QIcon              _channelIcon;
    QIcon              _videoIcon;
    bool               _inputOrderLock;
    bool               _disableUpdates;
    bool               _progressiveLoadingInProgress;
    QTimer             _lazySetInputsTimer;
    QTimer             _lazyUpdateTimer;
    QTimer             _mainWinVisTimer;
    string             _css;
    bool               _darkUI;
    QDialog            _createImageDialog;
    QColorDialog       _colorDialog;
    QMenu              _viewContextMenu;
    QAction[]          _viewContextMenuActions;
    QMenu              _createMenu;
    QMenu              _folderMenu;

    QDialog            _newNodeDialog;
    QComboBox          _nodeTypeCombo;

    QToolButton        _prevViewButton;
    QToolButton        _nextViewButton;
    QLabel             _viewLabel;

    QColor             _selectedSubComp;

    QLineEdit          _cidWidth;
    QLineEdit          _cidHeight;
    QLineEdit          _cidFPS;
    QLineEdit          _cidLength;
    QLabel             _cidPic;
    QGroupBox          _cidGroupBox;
    string             _cidName;
    string             _cidFMTSpec;
    QPushButton        _cidColorButton;
    QLabel             _cidColorLabel;
    QColor             _cidColor;
    bool               _quitting;

    //
    //  Some helper functions. Some of the Qt interface is a bit
    //  verbose and/or I'm too inexperienced to know how to do this in
    //  a more succinct way.
    //

    method: colorAdjustedIcon (QIcon; string rpath, bool invertSense)
    {
        let bg = QApplication.palette().color(QPalette.Active, QPalette.Background),
            icon0 = QImage(regex.replace("48x48", rpath, "out"), nil),
            icon1 = QImage(rpath, nil),
            swap = invertSense != _darkUI,
            qimage = if swap then icon0 else icon1;

        let icon = QIcon(QPixmap.fromImage(qimage, Qt.AutoColor));

        //if (false)
        if (swap)
        {
            icon.addPixmap(QPixmap.fromImage(icon1, Qt.AutoColor),
                           QIcon.Selected,
                           QIcon.Off);
        }
        else
        {
            icon.addPixmap(QPixmap.fromImage(icon1, Qt.AutoColor),
                           QIcon.Selected,
                           QIcon.Off);
        }

        return icon;
    }

    method: auxFilePath (string; string icon)
    {
        io.path.join(supportPath("session_manager", "session_manager"), icon);
    }

    method: auxIcon (QIcon; string name, bool colorAdjust = false)
    {
        if colorAdjust 
            then colorAdjustedIcon(":images/" + name, false)
            else QIcon(":images/" + name);
    }


    method: splitterMoved (void; int pos, int index)
    {
        let propName = "#Session.sm_window.splitter",
            fpos = float(pos) / float(_splitter.height());

        if (!propertyExists(propName)) 
        {
            newProperty(propName, FloatType, 1);
        }

        set(propName, fpos);
    }

    method: selectInputsRange (void; int[] selectionList)
    {
        let smodel = _inputsView.selectionModel();

        for_each (row; selectionList)
        {
            let index = _inputsModel.index(row, 0, QModelIndex());
            smodel.select(index, QItemSelectionModel.Select);
        }
    }

    method: iconForNode (QIcon; string node)
    {
        let typeName = nodeType(node),
            cprop    = node + ".sm_state.componentSubType";

        if (propertyExists(cprop))
        {
            let prop = getIntProperty(cprop);

            if (!prop.empty())
            {
                case (prop.front())
                {
                    ViewSubComponent    -> { return _viewIcon; }
                    LayerSubComponent   -> { return _layerIcon; }
                    ChannelSubComponent -> { return _channelIcon; }
                }
            }
        }

        for_each (i; _typeIcons) if (i._0 == typeName) return i._1;
        return _unknownTypeIcon;
    }

    method: viewEditModeActivated (void; Event event)
    {
        event.reject();
        sendInternalEvent("session-manager-load-ui", viewNode());
    }

    method: enterQuittingState(void; Event event)
    {
        //
        // Set quitting flag in response to imminent session deletion. Note
        // that this relies on the fact that this mode receives the 
        // before-session-deletion event prior to the ModeManager mode.
        //

        _quitting = true;
        event.reject();
    }

    method: activate (void;) 
    { 
        if (_dockWidget neq nil) _dockWidget.installEventFilter(_eventFilter);

        use SettingsValue;

        try
        {
            let String s = readSetting("SessionManager", "showOnStartup", String("no"));

            if (s == "last")
            {
                writeSetting ("Tools", "show_session_manager", Bool(true));
            }
        }
        catch (...)
        {
            writeSetting("SessionManager", "showOnStartup", String("no"));
            writeSetting("Tools", "show_session_manager", Bool(false));
        }

        _dockWidget.show();
        updateTree();
        sendInternalEvent("session-manager-load-ui", viewNode());
    }

    method: deactivate (void;) 
    { 
        if (_dockWidget neq nil) _dockWidget.removeEventFilter(_eventFilter);

        use SettingsValue;

        try
        {
            let String s = readSetting("SessionManager", "showOnStartup", String("no"));

            if (s == "last" && !_quitting)
            {
                writeSetting ("Tools", "show_session_manager", Bool(false));
            }
        }
        catch (...)
        {
            writeSetting ("SessionManager", "showOnStartup", String("no"));
            writeSetting("Tools", "show_session_manager", Bool(false));
        }

        _lazySetInputsTimer.stop();
        _lazyUpdateTimer.stop();
        _dockWidget.hide(); 
    }

    method: setNodeStatus (void; string node, string status)
    {
        let items = map(_viewModel, 
                        \: (bool; QStandardItem i) { itemNode(i) == node && !itemIsSubComponent(i); });

        for_each (i; items)
        {
            let sitem = i.parent().child(i.row(), 2);

            if (sitem eq nil) 
            {
                i.parent().setChild(i.row(), 2, QStandardItem(status));
            }
            else
            {
                sitem.setText(status);
            }
        }
    }

    method: viewByIndex (void; QModelIndex index, QStandardItemModel model)
    {
        let item    = model.itemFromIndex(index),
            node    = itemNode(item),
            subType = itemSubComponentType(item);

        _disableUpdates = true;

        try
        {
            bool viewChange = false;
            if (viewNode() != node)
            {
                setViewNode(node);
                viewChange = true;
            }

            if (subType != NotASubComponent)
            {
                setImageRequest(subComponentPropValue(item), !viewChange);
            }
        }
        catch (...)
        {
            ;
        }

        _disableUpdates = false;
        updateInputs(viewNode());
        //redraw();
    }

    method: itemPressed (void; QModelIndex index, QStandardItemModel model)
    {
        let item0   = model.itemFromIndex(index),
            sindex  = index.sibling(index.row(), 0),
            item    = model.itemFromIndex(sindex),
            node    = itemNode(item),
            subType = itemSubComponentType(item);

        if (item0.column() == 1 &&
            subType != NotASubComponent &&
            subType != MediaSubComponent)
        {
            viewByIndex(sindex, model);
        }
    }

    method: viewItemChanged (void; QStandardItem item)
    {
        let node        = itemNode(item),
            subType     = itemSubComponentType(item),
            parentItem  = item.parent(),
            parent      = if parentItem eq nil then nil else itemNode(parentItem),
            nodePaths   = _viewTreeView.filteredDraggedPaths(\: (bool; string[] p) { p[0] == node; });

        if (_viewTreeView._dropAction == Qt.CopyAction)
        {
            //
            //  You can get called *twice* here if you have multiple
            //  columns, but it will be giving you the 0th column
            //  only! so Just don't allow input copies from dnd
            //

            if (!hasInput(parent, node))
            {
                addInput(parent, node);
                item.setData(QVariant(parent), Qt.UserRole + 1);
                if (parent neq nil && 
                    nodeExists(parent) && 
                    nodeType(parent) == "RVFolderGroup")
                {
                    _viewTreeView.sortFolderChildren(parent);
                }
            }
        }
        else if (_viewTreeView._dropAction == Qt.MoveAction && !nodePaths.empty())
        {
            let parentExists = nodeExists(parent);

            if (parentExists)
            {
                if (!hasInput(parent, node))
                {
                    addInput(parent, node);
                }
            }

            item.setData(QVariant(if parentExists then parent else ""), Qt.UserRole + 1);

            for_each (path; nodePaths)
            {
                if (path.size() > 1)
                {
                    let n = path[0],
                        p = path[1];
                    
                    if (nodeExists(p) && (!nodeExists(parent) || p != parent)) removeInput(p, n);
                }
            }

            if (nodeExists(parent) && nodeType(parent) == "RVFolderGroup")
            {
                _viewTreeView.sortFolderChildren(parent);
            }
        }
        else if (node != "" && subType == NotASubComponent)
        {
            _disableUpdates = true;

            try
            {
                setUIName(node, item.text());
            }
            catch (...)
            {
                print("failed to set name on %s to %s\n" % (node, item.text())); // bad
            }

            _disableUpdates = false;
        }
    }

    method: viewSelectionChanged (void; QItemSelection selected, QItemSelection deselected)
    {
        let indices = selected.indexes();

        if (!indices.empty()) 
        {
            let index = indices.front();

            //
            //  Only consider top-level items
            //

            if (index.parent().parent().row() == -1) 
            {
                let indices = _viewTreeView.selectionModel().selectedRows(0);
                if (!indices.empty()) viewByIndex(indices.front(), _viewModel);
            }
        }
    }

    method: updateInputs (void; string node)
    {
        if (_disableUpdates || _progressiveLoadingInProgress) return;

        _inputOrderLock = true;

        _inputsModel.clear();
        let connections = nodeInputs(node),
            vnodes = viewNodes();

        for_index (i; connections)
        {
            let innode = connections[i],
                item   = QStandardItem(iconForNode(innode), uiName(innode)),
                vindex = indexOfItem(vnodes, innode);

            item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsDragEnabled | Qt.ItemIsEnabled);
            item.setData(QVariant(innode), Qt.UserRole + 2);
            item.setEditable(false);
            
            _inputsModel.appendRow(item);
        }

        _inputOrderLock = false;
    }

    method: selectViewableNode (void;)
    {
        if (viewNode() eq nil) return;
        
        let node   = viewNode(),
            uiname = uiName(node),
            cols   = _viewModel.columnCount(QModelIndex()),
            smodel = _viewTreeView.selectionModel(),
            items  = map(_viewModel, \: (bool; QStandardItem i) { itemNode(i) == node; });

        smodel.clear();

        for_each (item; items)
        {
            let index = _viewModel.indexFromItem(item),
                selection = QItemSelection(index, index.sibling(index.row(), cols-1));

            smodel.select(selection, QItemSelectionModel.SelectCurrent);
            updateInputs(node);
            _viewTreeView.scrollTo(index, QAbstractItemView.EnsureVisible);
            break;
        }
    }

    method: selectCurrentViewSlot (void; bool checked)
    {
        selectViewableNode();
    }

    method: updateNavUI (void; )
    {
        let n = viewNode();

        if (n eq nil) return;

        _viewLabel.setText(uiName(n));
        _prevViewButton.setEnabled(previousViewNode() neq nil);
        _nextViewButton.setEnabled(nextViewNode() neq nil);
    }

    method: afterGraphViewChange (void; Event event)
    {
        event.reject();

        let n = viewNode(),
            t = nodeType(n);

        if (n eq nil) return;
        selectViewableNode();
        setNodeStatus(viewNode(), "\u2714");

        updateNavUI();
        restoreTabState();

        //
        //  Disable inputs for the types we know don't allow any
        //

        _inputsView.setEnabled(t != "RVSource" && 
                               t != "RVFileSource" &&
                               t != "RVImageSource" &&
                               t != "RVSourceGroup");

        sendInternalEvent("session-manager-load-ui", viewNode());
    }

    method: addEditor (void; string name, QWidget widget)
    {
        let item = QTreeWidgetItem(string[] {name}, QTreeWidgetItem.Type),
            child = QTreeWidgetItem(string[] {""}, QTreeWidgetItem.Type);

        widget.setAutoFillBackground(true);
        item.setIcon(0, QIcon(":/images/radio_button_blue_on.png"));
        item.setFlags(Qt.ItemIsEnabled);

        item.addChild(child);
        _uiTreeWidget.addTopLevelItem(item);
        _uiTreeWidget.setItemWidget(child, 0, widget);
        widget.show();
        item.setExpanded(true);

        _editors.push_back(item);
    }

    method: useEditor (void; string name)
    {
        for_each (e; _editors) if (name == e.text(0)) e.setHidden(false);
    }

    method: reloadEditorTab (void;)
    {
        for_each (e; _editors) e.setHidden(true);
        sendInternalEvent("session-manager-load-ui", viewNode());
    }

    method: beforeGraphViewChange (void; Event event)
    {
        for_each (e; _editors) e.setHidden(true);
        event.reject();
        saveTabState();
        setNodeStatus(viewNode(), "");
    }

    method: nodeInputsChanged (void; Event event)
    {
        if (viewNode() eq nil) return;
        let node = event.contents();
        if (node == viewNode()) updateInputs(node);

        if (nodeType(node) == "RVFolderGroup" && 
            _viewTreeView._dropAction == Qt.IgnoreAction) 
        {
            _lazyUpdateTimer.start(0);
        }

        event.reject();
    }
 
    method: propertyChanged (void; Event event)
    {
        let prop  = event.contents(),
            parts = prop.split("."),
            node  = parts[0],
            comp  = parts[1],
            name  = parts[2];

        //
        //  If a UI name changes we need to update the tree 
        //  Or if someone else resorts the nodes.
        //

        if (comp == "ui" && name == "name") 
        {
            _lazyUpdateTimer.start(0);
            updateNavUI();
        }
        else if (comp == "sm_state" && (name == "sortKey" || name == "sortKeyParent")) 
        {
            _lazyUpdateTimer.start(0);
        }
        else if (comp == "request" && name == "imageComponent")
        {
            let topNode = nodeGroup(node),
                pval    = getStringProperty(prop);
                

            for_each (item; subComponentItemsOfNode(_viewModel, topNode))
            {
                let selected  = contentsEqual(pval, subComponentPropValue(item)),
                    checkitem = item.parent().child(item.row(), 1);

                checkitem.setIcon(if selected
                                    then QIcon(":/images/radio_button_blue_on.png")
                                    else QIcon(":/images/radio_button_dark.png"));
            }
        }

        event.reject();
    }

    method: setItemExpandedState (void; QModelIndex index, int value)
    {
        let item    = _viewModel.itemFromIndex(index),
            node    = itemNode(item),
            subComp = itemIsSubComponent(item);

        if (subComp)
        {
            setSubComponentExpanded(node, item, value == 1);
        }
        else
        {
            if (nodeExists(node))
            {
                let parent = itemNode(item.parent());
                setExpandedInParent(node, parent, value == 1);
            }
            else
            {
                let propName = "#Session.sm_view.%s" % item.text();
                set(propName, value);
            }
        }

        resizeColumns(_viewTreeView, _viewModel);
    }

    method: viewContextMenuSlot (void; QPoint pos)
    {
        if (_viewContextMenu eq nil)
        {
            _viewContextMenu = QMenu(_viewTreeView);
            
            let folderMenu = _viewContextMenu.addMenu(_folderMenu);
            folderMenu.setIcon(auxIcon("foldr_48x48.png", true));

            let createMenu = _viewContextMenu.addMenu(_createMenu);
            createMenu.setIcon(auxIcon("add_48x48.png", true));

            for_each (a; _viewContextMenuActions) _viewContextMenu.addAction(a);
        }

        _viewContextMenu.exec(_viewTreeView.mapToGlobal(pos), nil);
    }

    method: newNodeStatusColumns ([QStandardItem]; string node)
    {
        [QStandardItem] list = nil;

        repeat (2)
        {
            let item = QStandardItem("");
            item.setFlags(Qt.ItemIsEnabled | Qt.ItemIsSelectable);
            list = item : list;
        }

        list;
    }

    method: newNodeSubComponent (QStandardItem;
                                 int subComponent,
                                 QStandardItem parentItem,
                                 string media,
                                 string fullName,
                                 string node,
                                 string parent,
                                 bool selected)
    {

        let name = if subComponent == MediaSubComponent then io.path.basename(fullName) else fullName,
            item = QStandardItem(if name == "" then "default" else name);

        if (name == "")
        {
            let font = item.font();
            font.setItalic(true);
            item.setFont(font);
        }

        item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsDragEnabled | Qt.ItemIsEnabled);
        item.setData(QVariant(parent), Qt.UserRole + 1);
        item.setData(QVariant(node), Qt.UserRole + 2);
        item.setData(QVariant(subComponent), Qt.UserRole + 4);
        item.setData(QVariant(fullName), Qt.UserRole + 5);
        item.setData(QVariant(media), Qt.UserRole + 7);
        item.setEditable(true);

        case (subComponent)
        {
            MediaSubComponent   -> {;}
            ViewSubComponent    -> { item.setIcon(_viewIcon); }
            LayerSubComponent   -> { item.setIcon(_layerIcon); }
            ChannelSubComponent -> { item.setIcon(_channelIcon); }
        }
        
        let sitems = newNodeStatusColumns(node),
            selitem = head(sitems);

        if (subComponent != MediaSubComponent)
        {
            selitem.setIcon(QIcon(if selected 
                                      then ":/images/radio_button_blue_on.png"
                                      else ":/images/radio_button_dark.png"));
        }

        addRow(parentItem, item : sitems);

        item.setData(QVariant(hashedSubComponent(item)), Qt.UserRole + 6);

        if (subComponent != ChannelSubComponent && 
            isSubComponentExpanded(node, item)) 
        {
            _viewTreeView.setExpanded(_viewModel.indexFromItem(item), true);
        }

        // VS 2010 may have a bug: it looks like using Mu's "return"
        // statement (which uses _longjmp) results in the runtime
        // calling a destructor of a stack object twice. It looks like
        // the MS compiler is trying to unwind from longjmp, but is
        // doing it badly.
        item;
    }

    method: newNodeRow (void; 
                        QStandardItem parentItem,
                        string node,
                        string parent,
                        bool recursive = false)
    {
        let ntype = nodeType(node),
            item  = QStandardItem(uiName(node)),
            folder = ntype == "RVFolderGroup",
            source = ntype == "RVSourceGroup",
            //keyprop = "%s.sm_state.%s_sortKey" % (node, parent),
            //sortKey = if propertyExists(keyprop) then getIntProperty(keyprop).front() else int.max - 100;
            sortKey = sortKeyInParent(node, parent),
            toolTip = toolTipFromProp(node),
            icon    = iconForNode(node);
        
        item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsDragEnabled | Qt.ItemIsEnabled |
                      (if folder then Qt.ItemIsDropEnabled else Qt.NoItemFlags));
        item.setData(QVariant(parent), Qt.UserRole + 1);
        item.setData(QVariant(node), Qt.UserRole + 2);
        item.setData(QVariant(sortKey), Qt.UserRole + 3);
        item.setData(QVariant(NotASubComponent), Qt.UserRole + 4);
        item.setEditable(true);
        item.setIcon(icon);
        item.setRowCount(0);

        let statusItems = newNodeStatusColumns(node);
        if (node == viewNode()) head(tail(statusItems)).setText("\u2714");
        addRow(parentItem, item : statusItems);

        //
        //  Tabs in tooltips make win32 Qt crash.
        //

        item.setToolTip(if toolTip eq nil 
                            then "" 
                            else regex.replace("\t", toolTip, " "));

        if (folder && recursive)
        {
            for_each (n; nodeConnections(node)._0)
            {
                newNodeRow(item, n, node, recursive);
            }
        }

//        item.sortChildren(0, Qt.AscendingOrder);

        if (isExpandedInParent(node, parent))
        {
            _viewTreeView.setExpanded(_viewModel.indexFromItem(item), true);
        }
            
        if (source)
        {
            if (!propertyExists(node + ".sm_state.componentHash"))
            {
                //
                //  Don't do this for nodes representing parts of existing
                //  nodes. Those will have the property
                //  node.sm_state.componentHash
                //

                let sourceNode = sourceNodeOfGroup(node),
                    pval       = getStringProperty(sourceNode + ".request.imageComponent"),
                    hasPval    = pval.size() > 1,
                    iname      = if hasPval then pval.back() else string(nil),
                    itype      = if hasPval
                                    then itemSubComponentTypeForName(pval[0])
                                    else NotASubComponent;

                try
                {
                    for_each (info; sourceMediaInfoList(sourceNode))
                    {
                        let fileItem = newNodeSubComponent(MediaSubComponent, item,
                                                           info.file, info.file, 
                                                           node, parent, false);

                        let font = fileItem.font();
                        font.setBold(true);
                        fileItem.setFont(font);
                        QStandardItem topItem = fileItem;

                        for_each (v; info.viewInfos)
                        {
                            if (info.viewInfos.size() > 1 && v.name != "")
                            {
                                let selected = itype == ViewSubComponent && 
                                               iname == v.name;

                                topItem = newNodeSubComponent(ViewSubComponent, fileItem,
                                                              info.file, v.name, node, parent, 
                                                              selected);
                            }
                            else
                            {
                                topItem = fileItem;
                            }
                    
                            let nlayers = v.layers.size();
                    
                            for_each (l; v.layers)
                            {
                                QStandardItem layerItem = nil;
                                let unnamed = l.name == "";
                                let selected = itype == LayerSubComponent && iname == l.name;

                                if (nlayers > 1 && unnamed)
                                {
                                    layerItem = newNodeSubComponent(LayerSubComponent, topItem,
                                                                    info.file, "", node, parent, 
                                                                    selected);
                                }
                                else if (!unnamed)
                                {
                                    layerItem = newNodeSubComponent(LayerSubComponent, topItem,
                                                                    info.file, l.name, node, parent, 
                                                                    selected);
                                }
                                else 
                                {
                                    layerItem = topItem;
                                }
                        
                                for_each (c; l.channels)
                                {
                                    let selected = itype == ChannelSubComponent && iname == c.name;

                                    newNodeSubComponent(ChannelSubComponent, layerItem,
                                                        info.file, c.name, node, parent, 
                                                        selected);
                                }
                            }
                    
                            if (!v.layers.empty() && !v.noLayerChannels.empty())
                            {
                                let selected = itype == LayerSubComponent && iname == "";

                                topItem = newNodeSubComponent(LayerSubComponent, topItem,
                                                              info.file, "", node, parent, 
                                                              selected);
                            }
                    
                            for_each (c; v.noLayerChannels)
                            {
                                let selected = itype == ChannelSubComponent && iname == c.name;

                                newNodeSubComponent(ChannelSubComponent, topItem,
                                                    info.file, c.name, node, parent, 
                                                    selected);
                            }
                        }
                    }
                }
                catch (...)
                {
                    ; // ignore
                }
            }
            else
            {
                try
                {
                    let pname     = node + ".sm_state.componentOfNode",
                        cnode     = getStringProperty(pname).front(),
                        emptyItem = QStandardItem("(subcompoment of %s)" % uiName(cnode)),
                        font      = emptyItem.font();

                    font.setItalic(true);
                    emptyItem.setFont(font);
                    addRow(item, [emptyItem, QStandardItem("")]);
                }
                catch (...)
                {
                    ;
                }
            }
        }
    }

    method: updateTree (void;)
    {
        if (_disableUpdates) return;
        _viewModel.clear();
        _viewModel.setHorizontalHeaderLabels(string[] {"Name", "*", "*"});
        _viewTreeView.header().setMinimumSectionSize(-1);
        if (viewNode() eq nil) return;

        string[] topLevelNodes;

        try
        {
            _viewModel.setSortRole(Qt.UserRole + 3); // the sort key is an int

            let viewNodes     = viewNodes(),
                currentNode   = viewNode(),
                foldersItem   = QStandardItem("FOLDERS"),
                sourcesItem   = QStandardItem("SOURCES"),
                sequencesItem = QStandardItem("SEQUENCES"),
                stackItem     = QStandardItem("STACKS"),
                layoutItem    = QStandardItem("LAYOUTS"),
                otherItem     = QStandardItem("OTHER"),
                categoryItems = [foldersItem, sourcesItem, sequencesItem, stackItem, layoutItem, otherItem],
                fgMac         = QBrush(QColor(80,80,80,255), Qt.SolidPattern),
                fgOther       = QBrush(QColor(125,125,125,255), Qt.SolidPattern),
                foreground    = if _darkUI then fgOther else fgMac;
                
            for_each (item; categoryItems)
            {
                item.setFlags(Qt.ItemIsEnabled);
                item.setForeground(foreground);
                item.setSizeHint(QSize(-1, 25));
                item.setData(QVariant(""), Qt.UserRole + 1);
                item.setData(QVariant(""), Qt.UserRole + 2);
                item.setData(QVariant(int.max), Qt.UserRole + 3);
            }

            foldersItem.setFlags(Qt.ItemIsEnabled | Qt.ItemIsDropEnabled);
            _viewTreeView._foldersItem = foldersItem;
            
            for_index (i; viewNodes)
            {
                let node  = viewNodes[i],
                    ntype = nodeType(node),
                    vis   = currentNode == node,
                    outs  = nodeConnections(node)._1;

                bool folderParent = false;
                for_each (o; outs) if (nodeType(o) == "RVFolderGroup") folderParent = true;

                if (!folderParent) 
                {
                    case (ntype)
                    {
                        "RVFileSource"    -> { newNodeRow(sourcesItem, node, "", true); }
                        "RVImageSource"   -> { newNodeRow(sourcesItem, node, "", true); }
                        "RVSourceGroup"   -> { newNodeRow(sourcesItem, node, "", true); }
                        "RVSequenceGroup" -> { newNodeRow(sequencesItem, node, "", true); }
                        "RVStackGroup"    -> { newNodeRow(stackItem, node, "", true); }
                        "RVLayoutGroup"   -> { newNodeRow(layoutItem, node, "", true); }
                        "RVFolderGroup"   -> { newNodeRow(foldersItem, node, "", true); }
                        _                 -> { newNodeRow(otherItem, node, "", true); }
                    }
                }
            }

            for_each (item; categoryItems)
            {
                if (item.rowCount() != 0)
                {
                    let text = item.text(),
                        propName = "#Session.sm_view.%s" % text;

                    if (!propertyExists(propName)) 
                    {
                        newProperty(propName, IntType, 1);
                        setIntProperty(propName, int[]{1}, true);
                    }

                    let dummy1 = QStandardItem(""),
                        dummy2 = QStandardItem("");
                    dummy1.setFlags(Qt.ItemIsEnabled);
                    dummy2.setFlags(Qt.ItemIsEnabled);
                    _viewModel.appendRow(QStandardItem[] {item, dummy1, dummy2});
                    _viewTreeView.setExpanded(_viewModel.indexFromItem(item), 
                                              getIntProperty(propName).front() == 1);
                }
            }

            _viewModel.sort(0, Qt.AscendingOrder);
            _viewModel.invisibleRootItem().setFlags(Qt.ItemIsEnabled);
            selectViewableNode();

            resizeColumns(_viewTreeView, _viewModel);
        }
        catch (exception exc)
        {
            print("%s\n" % exc);
        }
    }

    method: updateTreeEvent (void; Event event)
    {
        updateTree();
        event.reject();
    }

    method: beforeProgressiveLoading (void; Event event)
    {
        event.reject();
        _progressiveLoadingInProgress = true;
    }

    method: afterProgressiveLoading (void; Event event)
    {
        event.reject();
        _progressiveLoadingInProgress = false;
        updateTree();
        updateInputs(viewNode());
    }

    method: newColorSlot (void; QColor color)
    {
        let css = "QPushButton{background-color:rgb(%d,%d,%d);}"
                                      % (color.red(), color.green(), color.blue());
        _cidColorButton.setStyleSheet(css);
        _cidColor = color;
    }

    method: chooseColorSlot (void; bool checked)
    {
        _colorDialog.open();
        _colorDialog.setCurrentColor(_cidColor);
    }

    method: renameByType (void; string node, string[] inputs)
    {
        let n = inputs.size(),
            basename = nodeType(node);

	if (regex.match("^RV", basename))    basename = basename.substr(2, 0);
	if (regex.match("Group$", basename)) basename = basename.substr(0, -5);

        string name = "";
        bool hasspaces = false;

        if (n == 0)
        {
            name = "Empty %s" % basename;
        }
        else if (n < 3)
        {
            name = "%s of " % basename;

            for_index (i; inputs)
            {
                if (i > 0 && n > 2) name += ",";
                if (i > 0) name += " ";
                if (i == n-1 && n > 1) name += "and ";
                name += uiName(inputs[i]);
            }
        }
        else
        {
            name = "%s of %d views " % (basename, n);
        }

        setUIName(node, name);
    }

    method: componentAndFolderNodeFromHash ((string,string); string hash, string node)
    {
        string folder = nil;
        string cnode = nil;

        for_each (n; nodes())
        {
            if (nodeType(n) == "RVSourceGroup" && cnode eq nil)
            {
                let propName = n + ".sm_state.componentHash";

                if (propertyExists(propName))
                {
                    try
                    {
                        let p = getStringProperty(propName),
                            pn = getStringProperty(n + ".sm_state.componentOfNode");

                        if (!p.empty() && p[0] == hash &&
                            !pn.empty() && pn[0] == node)
                        {
                            return (cnode, folder);
                        }
                    }
                    catch (...)
                    {
                        ;
                    }
                }
            }
            else if (nodeType(n) == "RVFolderGroup")
            {
                let pname = n + ".sm_state.componentFolderOfNode";
                if (propertyExists(pname))
                {
                    let p = getStringProperty(pname);
                    if (!p.empty() && p[0] == node) folder = n;
                }
            }
        }

        return (cnode, folder);
    }

    method: newSubComponentNode (string; 
                                 string hash,
                                 int subType,
                                 string filename, 
                                 string fullName, 
                                 string[] compPropValue,
                                 string node, 
                                 string folder)
    {
        let snode     = addSourceVerbose(string[] {filename}),
            nodeName  = uiName(node),
            groupNode = nodeGroup(snode),
            dname     = if fullName == "" then "default" else fullName;

        if (folder eq nil) 
        {
            folder = newNode("RVFolderGroup", "%s_components" % node);
            setUIName(folder, "Components of %s" % uiName(node));
            set(folder + ".sm_state.componentFolderOfNode", node);
            setExpandedInParent(folder, "", false);
        }

        let inputs = nodeInputs(folder);
        inputs.push_back(groupNode);
        setNodeInputs(folder, inputs);

        set(groupNode + ".sm_state.componentOfNode", node);
        set(groupNode + ".sm_state.componentHash", hash);
        set(groupNode + ".sm_state.componentSubType", subType);

        case (subType)
        {
            MediaSubComponent -> 
            { 
                setUIName(groupNode, nodeName + " (Media %s)" % dname); 
            }

            ViewSubComponent -> 
            { 
                setUIName(groupNode, nodeName + " (View %s)" % dname); 
                setNodeRequest(snode, compPropValue);
            }

            LayerSubComponent -> 
            { 
                setUIName(groupNode, nodeName + " (Layer %s)" % dname); 
                setNodeRequest(snode, compPropValue);
            }

            ChannelSubComponent -> 
            { 
                setUIName(groupNode, nodeName + " (Channel %s)" % dname); 
                setNodeRequest(snode, compPropValue);
            }

            _ ->
            {
                ; // nothing
            }
        }

        displayFeedback("NOTE: Created %s" % uiName(groupNode), 5);
        groupNode;
    }

    method: sourceFromSubComponent (string; QStandardItem item, string node)
    {
        let hash = hashedSubComponent(item),
            (cnode, folder) = componentAndFolderNodeFromHash(hash, node);

        if (cnode neq nil) return cnode;

        QStandardItem mediaItem = nil;
        QStandardItem viewItem  = nil;
        QStandardItem layerItem = nil;

        for (QStandardItem i = item; 
             i neq nil && itemSubComponentType(i) != NotASubComponent;
             i = i.parent())
        {
            case (itemSubComponentType(i))
            {
                MediaSubComponent -> { mediaItem = i; break; }
                LayerSubComponent -> { layerItem = i;  }
                ViewSubComponent  -> { viewItem = i;  }
                _ -> {;}
            }
        }

        let subType   = itemSubComponentType(item),
            filename  = itemSubComponentValue(mediaItem),
            fullName  = itemSubComponentValue(item);

        newSubComponentNode(hash, subType, filename, fullName, 
                            subComponentPropValue(item), node, folder); 
    }

    method: selectedConvertedSubComponents (string[];)
    {
        let indices = _viewTreeView.selectionModel().selectedIndexes(),
            nodes   = string[]();

        for_each (index; indices) 
        {
            if (index.column() == 0)
            {
                let item = _viewModel.itemFromIndex(index),
                    n    = itemNode(item);

                if (nodeExists(n))
                {
                    if (itemIsSubComponent(item))
                    {
                        _disableUpdates = true;
                        let snode = sourceFromSubComponent(item, n);
                        _disableUpdates = false;
                        nodes.push_back(snode);
                    }
                    else
                    {
                        nodes.push_back(n);
                    }
                }
            }
        }

        return nodes;
    }

    method: selectedNodes (string[];)
    {
        let indices = _viewTreeView.selectionModel().selectedIndexes(),
            nodes = string[]();

        for_each (index; indices) 
        {
            if (index.column() == 0)
            {
                let n = itemNode(_viewModel.itemFromIndex(index));
                if (nodeExists(n)) nodes.push_back(n);
            }
        }

        return nodes;
    }

    method: selectedItems (QStandardItem[]; )
    {
        let indices = _viewTreeView.selectionModel().selectedIndexes();
        QStandardItem[] items;

        for_each (index; indices) 
        {
            if (index.column() == 0)
            {
                items.push_back(_viewModel.itemFromIndex(index));
            }
        }

        items;
    }

    method: addNodeOfType (string; string typename)
    {
        let nodes = selectedConvertedSubComponents(),
            n = newNode(typename, "");

        if (n eq nil || !setInputs(n, nodes))
        {
            if (n neq nil) deleteNode(n);
        }
        else
        {
            renameByType(n, nodes);
            setViewNode(n);
        }

        return n;
    }

    method: addNodeByTypeName (void;)
    {
        if (_newNodeDialog eq nil)
        {
            let m = mainWindowWidget();

            _newNodeDialog = loadUIFile(auxFilePath("new_node.ui"), m);
            _nodeTypeCombo = _newNodeDialog.findChild("comboBox");
            _nodeTypeCombo.addItems(commands.nodeTypes(true));
            QIcon icon = auxIcon("new_48x48.png", true); 
            QLabel label = _newNodeDialog.findChild("pictureLabel");
            label.setPixmap(icon.pixmap(QSize(48,48), QIcon.Normal, QIcon.Off));

            \: makeNewNodeOfType (void;)
            {
                this.addNodeOfType(this._nodeTypeCombo.currentText());
            }

            connect(_newNodeDialog, QDialog.accepted, makeNewNodeOfType);
        }

        _newNodeDialog.show();
    }

    method: addMovieProc (void; string fmtspec)
    {
        if (_createImageDialog eq nil)
        {
            let m = mainWindowWidget();

            _createImageDialog = loadUIFile(auxFilePath("create_image_dialog.ui"), m);
            _cidWidth          = _createImageDialog.findChild("widthEdit");
            _cidHeight         = _createImageDialog.findChild("heightEdit");
            _cidFPS            = _createImageDialog.findChild("fpsEdit");
            _cidLength         = _createImageDialog.findChild("lengthEdit");
            _cidPic            = _createImageDialog.findChild("pictureLabel");
            _cidGroupBox       = _createImageDialog.findChild("groupBox");
            _cidColorButton    = _createImageDialog.findChild("colorButton");
            _cidColorLabel     = _createImageDialog.findChild("colorLabel");

            let SettingsValue.Float f1 = readSetting ("General", "fps", SettingsValue.Float(24.0));

            _cidFPS.setText("%g" % float(f1));

            //_createImageDialog.setModal(true);


            \: makeImage (void;)
            {
                let mp = this._cidFMTSpec % ("width=%s,height=%s,fps=%s,start=1,end=%s,red=%g,green=%g,blue=%g" %
                                     (this._cidWidth.text(),
                                      this._cidHeight.text(),
                                      this._cidFPS.text(),
                                      this._cidLength.text(),
                                      float(this._cidColor.redF()),
                                      float(this._cidColor.greenF()),
                                      float(this._cidColor.blueF()))),
                     s = addSourceVerbose(string[] {mp});

                setUIName(nodeGroup(s), this._cidName);
            }
            
            connect(_createImageDialog, QDialog.accepted, makeImage);
            connect(_cidColorButton, QPushButton.clicked, chooseColorSlot);
        }

        QIcon icon;
        let ptype = fmtspec.split(",").front();

        _cidColorButton.setVisible(true);
        _cidColorLabel.setVisible(true);
        _cidColorButton.setEnabled(false);
        _cidColorLabel.setEnabled(false);

        case (ptype)
        {
            "srgbcolorchart" -> 
            {
                _cidName = "SRGBMacbethColorChart";
                icon = auxIcon("colorchart_48x48.png", true); 
                _cidColorButton.setStyleSheet("QPushButton { background-color: rgb(128,128,128); }");
                _cidColorButton.setVisible(false);
                _cidColorLabel.setVisible(false);
                _cidColor = QColor(0,0,0,255);
            }

            "acescolorchart" -> 
            {
                _cidName = "ACESMacbethColorChart";
                icon = auxIcon("colorchart_48x48.png", true); 
                _cidColorButton.setStyleSheet("QPushButton { background-color: rgb(128,128,128); }");
                _cidColorButton.setVisible(false);
                _cidColorLabel.setVisible(false);
                _cidColor = QColor(0,0,0,255);
            }

            "smptebars" -> 
            {
                _cidName = "SMTPEColorBars";
                icon = auxIcon("ntscbars_48x48.png", true); 
                _cidColorButton.setStyleSheet("QPushButton { background-color: rgb(128,128,128); }");
                _cidColorButton.setVisible(false);
                _cidColorLabel.setVisible(false);
                _cidColor = QColor(0,0,0,255);
            }

            "blank" -> 
            {
                _cidName = "Blank";
                icon = auxIcon("video_48x48.png", true); 
                _cidColorButton.setStyleSheet("QPushButton { background-color: rgb(128,128,128); }");
                _cidColorButton.setVisible(false);
                _cidColorLabel.setVisible(false);
                _cidColor = QColor(0,0,0,255);
                _cidWidth.setVisible(false);
                _cidHeight.setVisible(false);
            }

            "black" -> 
            {
                _cidName = "Black";
                icon = auxIcon("video_48x48.png", true); 
                _cidColorButton.setStyleSheet("QPushButton { background-color: rgb(0,0,0); }");
                _cidColor = QColor(0,0,0,255);
            }

            "solid" -> 
            {
                _cidName = "SolidColor";
                icon = auxIcon("video_48x48.png", true); 
                _cidColorButton.setStyleSheet("QPushButton { background-color: rgb(128,128,128); }");
                _cidColorButton.setEnabled(true);
                _cidColorLabel.setEnabled(true);
                _cidColor = QColor(128,128,128,255);
            }
        }

        _cidPic.setPixmap(icon.pixmap(QSize(48,48), QIcon.Normal, QIcon.Off));
        _cidGroupBox.setTitle(_cidName);
        _cidFMTSpec = fmtspec;
        _createImageDialog.show();
    }

    method: addThingSlot (void; bool checked, string thingstring)
    {
        if (regex.match(".+\\.movieproc$", thingstring))
        {
            addMovieProc(thingstring);
        }
        else if (thingstring == "")
        {
            addNodeByTypeName();
        }
        else
        {
            addNodeOfType(thingstring);
        }
    }

    method: newFolderSlot (void; bool checked, int which)
    {
        let paths = _viewTreeView.selectedNodePaths(),
            folder = newNode("RVFolderGroup", "Folder");

        string[] nodes;
        for_each (path; paths) nodes.push_back(path.front());

        if (!paths.empty())
        {
            let first = paths.front();

            if (which != 1 && !nodes.empty())
            {
                if (!setInputs(folder, nodes)) 
                {
                    if (folder neq nil) 
                    {
                        deleteNode(folder);
                        return;
                    }
                }
            }
                
            _disableUpdates = true;

            if (which == 2) 
            {
                for_each (path; paths)
                {
                    if (path.size() > 1 && nodeExists(path[1])) 
                    {
                        removeInput(path[1], path[0]);
                    }
                }
            }

            if (nodeExists(first[1])) 
            {
                addInput(first[1], folder);

                setSortKeyInParent(folder,
                                   first[1],
                                   sortKeyInParent(first[0], first[1]));
            }

            _disableUpdates = false;
        }

        _disableUpdates = true;
        renameByType(folder, if which == 1 then string[]() else nodes);
        _disableUpdates = false;

        if (!paths.empty()) setViewNode(folder);
    }

    method: deleteViewableSlot (void; bool checked)
    {
        let items = selectedItems();

        for_each (item; items) 
        {
            let node       = itemNode(item),
                parent     = itemParentNode(item),
                outs       = nodeConnections(node)._1,
                parentType = if nodeExists(parent) then nodeType(parent) else "";

            int nfolders = 0;
            for_each (o; outs) if (nodeType(o) == "RVFolderGroup") nfolders++;

            if (parentType == "RVFolderGroup" && nfolders > 1)
            {
                removeInput(parent, node);
            }
            else
            {
                _disableUpdates = true;

                try 
                {
                    //
                    //  Another weird situation with orphaned
                    //  items. Just avoid it by preventing updates.
                    //

                    deleteNode(node);
                }
                catch (object obj) 
                {
                    print ("Error: %s, failed to delete '%s'\n" % (string(obj), node));
                }

                _disableUpdates = false;
            }
        }

        _lazyUpdateTimer.start(0);
    }

    method: editViewInfoSlot (void; bool checked)
    {
        let indices = _viewTreeView.selectionModel().selectedIndexes();
        if (indices.empty()) return;
        let index = indices.front();
        _viewTreeView.edit(index);
    }

    method: reorderSelected (void; bool up, bool checked)
    {
        let indices = _inputsView.selectionModel().selectedIndexes();

        if (indices.empty()) return;

        let inputs = nodeInputs(viewNode());
        int minRow = math.min(indices.front().row(), indices.back().row());
        int maxRow = math.max(indices.front().row(), indices.back().row());

        if ((up && minRow == 0) || (!up && maxRow == inputs.size() - 1)) return;

        int numRows = _inputsModel.rowCount(QModelIndex());
        int[] selectionSizes;
        int selectionSize = 0;
        for (int i = 0; i < numRows; i++)
        {
            let index = _inputsModel.index(i, 0, QModelIndex());
            bool included = includes(indices, index);
            if (included) selectionSize++;
            else if (selectionSize > 0 || selectionSizes.size() > 0)
            {
                selectionSizes.push_back(selectionSize);
                selectionSize = 0;
            }
        }
        if (selectionSize > 0) selectionSizes.push_back(selectionSize);

        int[] includedList;
        string[] newNodes;
        newNodes.resize(numRows);
        int sizeIndex = 0;
        for (int i = 0; i < numRows; i++)
        {
            let index = _inputsModel.index(i, 0, QModelIndex());
            bool included = includes(indices, index);
            int newIndex = index.row();
            int includedInc = if up then -1 else 1;
            int excludedInc = -1 * includedInc * selectionSizes[sizeIndex];
            if (included)
            {
                newIndex = newIndex + includedInc;
                includedList.push_back(newIndex);
            }
            else if (!included && (newIndex >= minRow + includedInc) && (newIndex <= maxRow + includedInc))
            {
                newIndex = newIndex + excludedInc;
                if (sizeIndex < selectionSizes.size() - 1) sizeIndex++;
            }
            newNodes[newIndex] = nodeFromIndex(index, _inputsModel);
        }

        try
        {
            setInputs(viewNode(), newNodes);
            selectInputsRange(includedList);
        }
        catch (exception exc)
        {
            print("FAILED: %s\n" % exc);
        }

        redraw(); // don't think this is necessary
    }

    method: sortInputs (void; bool up, bool checked)
    {
        if (_inputOrderLock || viewNode() eq nil) return;

        //print ("sort %s\n" % up);
        let num    = _inputsModel.rowCount(QModelIndex()),
            node   = viewNode(),
            inputs = nodeInputs(node);

        //print ("inputs = %s\n" % inputs);
        string[] sorted;

        for_index (i; inputs)
        {
            let item   = _inputsModel.item(i, 0),
                source = inputs[i],
                media  = uiName(source);
            //print ("%s\n" % media);
            
            bool found = false;
            string[] tmp;
            
            for_each (s; sorted) {
                int order = compare(media, uiName(s));
                //print ("%s %s = %s\n" % (media, uiName(s), order));
                if (found || (up && order > 0) || (!up && order < 0)) {
                    // while s comes before item, add s to the list
                    tmp.push_back(s);
                } else {
                    // insert item before this source
                    tmp.push_back(source);
                    tmp.push_back(s);
                    found = true;
                    //print ("found %s\n" % s);
                }
            }
            if (!found) {
                // stick on the end
                //print ("append\n");
                sorted.push_back(source);
            } else {
                sorted = tmp;
            }
        }

        if (!setInputs(node, sorted))
        {
            updateInputs(node);
        }

        if (nodeType(node) == "RVFolderGroup")
        {
            for (int i = 0; i < sorted.size(); i++)
            {
                let n = sorted[i];
                setSortKeyInParent(n, node, i);
            }
            updateTree();
        }
    }

    method: rebuildInputsFromList (void;)
    {
        if (_inputOrderLock || viewNode() eq nil) return;

        let num    = _inputsModel.rowCount(QModelIndex()),
            inputs = nodeInputs(viewNode()),
            vnode  = viewNode(),
            vnodes = viewNodes();

        string[] nodes;

        _disableUpdates = true;

        for (int row = 0; row < num; row++)
        {
            let item  = _inputsModel.item(row, 0);

            if (item neq nil)
            {
                let node = itemNode(item);

                try
                {
                    if (itemIsSubComponent(item))
                    {
                        let hash     = itemSubComponentHash(item),
                            nodeItem = itemOfNode(_viewModel, node),
                            (cnode, folder) = componentAndFolderNodeFromHash(hash, node);

                        if (cnode eq nil)
                        {
                            let 
                                fullName = itemSubComponentValue(item),
                                filename = itemSubComponentMedia(item),
                                subType  = itemSubComponentType(item),
                                pval     = subComponentPropValue(item),
                                snode    = newSubComponentNode(hash, subType, filename,
                                                               fullName, pval, node, folder);

                            nodes.push_back(snode);
                        }
                        else
                        {
                            nodes.push_back(cnode);
                        }
                    }
                    else
                    {
                        nodes.push_back(node);
                    }
                }
                catch (...) {;}
            }
        }

        setViewNode(vnode);
        _disableUpdates = false;
        if (!setInputs(vnode, nodes)) updateInputs(vnode);
    }

    method: inputRowsRemovedSlot (void; QModelIndex parent, int start, int end)
    {
        if (_inputOrderLock || viewNode() eq nil) return;
        _lazySetInputsTimer.start(100);
    }

    method: printRows (void;)
    {
        let num = _inputsModel.rowCount(QModelIndex()),
            inputs = nodeInputs(viewNode());

        print("-\n");
        for (int row = 0; row < num; row++)
        {
            let item  = _inputsModel.item(row, 0);
            print("row %d -> %s\n" % (row, if item eq nil then "nil" else item.text()));
        }
    }

    method: showRows (void; Event event)
    {
        printRows();
    }

    method: inputRowsInsertedSlot (void; QModelIndex parent, int start, int end)
    {
        if (_inputOrderLock || viewNode() eq nil) return;
        _lazySetInputsTimer.start(100);
    }

    method: inputsDeleteSlot (void; bool checked) 
    { 
        if (_inputOrderLock || viewNode() eq nil) return;

        let indices = _inputsView.selectionModel().selectedIndexes(),
            inputs  = nodeInputs(viewNode()),
            vnodes  = viewNodes();

        string[] newNodes;
        
        for_index (i; inputs)
        {
            let index = _inputsModel.index(i, 0, QModelIndex());

            if (!includes(indices, index)) 
            {
                newNodes.push_back(nodeFromIndex(index, _inputsModel));
            }
        }

        try
        {
            setInputs(viewNode(), newNodes);
        }
        catch (exception exc)
        {
            print("FAILED: %s\n" % exc);
        }

        redraw(); // don't think this is necessary
    }

    method: saveTabState (void;)
    {
        let prop = "%s.sm_state.tab" % viewNode();
        set(prop, _tabWidget.currentIndex());
    }

    method: restoreTabState (void;)
    {
        let vnode = viewNode();

        if (vnode neq nil)
        {
            let prop = "%s.sm_state.tab" % vnode;
            
            if (propertyExists(prop))
            {
                let state = getIntProperty(prop).front();
                _tabWidget.setCurrentIndex(state);
            }
            else if (nodeType(vnode) == "RVSourceGroup")
            {
                _tabWidget.setCurrentIndex(1);
            }
        }
    }

    method: tabChangeSlot (void; int index)
    {
        saveTabState();
    }

    method: navButtonClicked (void; string which, bool checked)
    {
        _disableUpdates = true;

        try
        {
            if (which == "next" && nextViewNode() neq nil)     setViewNode(nextViewNode());
            if (which == "prev" && previousViewNode() neq nil) setViewNode(previousViewNode());
        }
        catch (...)
        {
            ; 
        }

        _disableUpdates = false;
        updateInputs(viewNode());
    }

    method: mainWinVisTimeout (void; )
    {
        //
        //  Don't adjust mode activity whcn main window
        //  is minimized.
        //
        if (mainWindowWidget().minimized()) return;

        if (!_dockWidget.visible() &&  _active) toggle();
        if ( _dockWidget.visible() && !_active) toggle();
    }

    method: visibilityChanged (void; bool vis)
    {
        //
        //  We want to avoid shutting down the mode when the window
        //  is minimized, but the min status is not correct unless
        //  we ask a little later ;-)
        //
        _mainWinVisTimer.start(0);
    }

    method: configSlot (void; bool checked, string onstart, bool show)
    {
        use SettingsValue;
        writeSetting("SessionManager", "showOnStartup", String(onstart));
        writeSetting("Tools", "show_session_manager", Bool(show));
    }

    method: SessionManagerMode (SessionManagerMode; string name)
    {
        _darkUI         = true;
        _inputOrderLock = false;
        _editors        = QTreeWidgetItem[]();
        _quitting       = false;
        _disableUpdates = false;
        _progressiveLoadingInProgress = (loadTotal() != 0);

        init(name,
             [ ("new-node", updateTreeEvent, "New user node"),
               ("source-modified", updateTreeEvent, "New source media"),
               ("source-group-complete", updateTreeEvent, "Source group complete"),
               ("before-progressive-loading", beforeProgressiveLoading, "before loading"),
               ("after-progressive-loading", afterProgressiveLoading, "after loading"),
               ("after-node-delete", updateTreeEvent, "Node deleted"),
               ("after-clear-session", updateTreeEvent, "Session Cleared"),
               ("after-graph-view-change", afterGraphViewChange, "Update session UI"),
               ("before-graph-view-change", beforeGraphViewChange, "Update session UI"),
               ("graph-node-inputs-changed", nodeInputsChanged, "Update session UI"),
               ("graph-state-change", propertyChanged,  "Maybe update session UI"),
               ("key-down--@", showRows, "show'em"),
               ("before-session-deletion", enterQuittingState, "Store quitting before session goes away"),
               ("view-edit-mode-activated", viewEditModeActivated, "Per-view edit mode activated, load UI"),
               ],
             nil,
             nil);

        let m = mainWindowWidget();

        _dockWidget         = QDockWidget("Session Manager", m, Qt.Widget);
        _baseWidget         = loadUIFile(auxFilePath("session_manager.ui"), m);
        _treeViewBase       = _baseWidget.findChild("treeView");
        _addButton          = _baseWidget.findChild("addButton");
        _folderButton       = _baseWidget.findChild("folderButton");
        _deleteButton       = _baseWidget.findChild("deleteButton");
        _configButton       = _baseWidget.findChild("configButton");
        _editViewInfoButton = _baseWidget.findChild("renameButton");
        _homeButton         = _baseWidget.findChild("selectCurrentButton");
        _inputsViewBase     = _baseWidget.findChild("inputsListView");
        _tabWidget          = _baseWidget.findChild("tabWidget");
        _orderUpButton      = _baseWidget.findChild("orderUpButton");
        _orderDownButton    = _baseWidget.findChild("orderDownButton");
        _sortAscButton      = _baseWidget.findChild("sortAscButton");
        _sortDescButton     = _baseWidget.findChild("sortDescButton");
        _inputsDeleteButton = _baseWidget.findChild("inputsDeleteButton");
        _uiTreeWidget       = _baseWidget.findChild("uiTreeWidget");
        _splitter           = _baseWidget.findChild("splitter");
        _viewLabel          = _baseWidget.findChild("viewLabel");
        _prevViewButton     = _baseWidget.findChild("prevViewButton");
        _nextViewButton     = _baseWidget.findChild("nextViewButton");

        _lazySetInputsTimer = QTimer(_dockWidget);
        _lazyUpdateTimer    = QTimer(_dockWidget);
        _mainWinVisTimer    = QTimer(_dockWidget);

        _lazySetInputsTimer.setSingleShot(true);
        _lazyUpdateTimer.setSingleShot(true);
        _mainWinVisTimer.setSingleShot(true);

        let vbox = QVBoxLayout(_treeViewBase);
        vbox.setContentsMargins(0, 0, 0, 0);
        _viewTreeView = NodeTreeView(_treeViewBase);
        vbox.addWidget(_viewTreeView);

        let ivbox = QVBoxLayout(_inputsViewBase);
        ivbox.setContentsMargins(0, 0, 0, 0);
        _inputsView = InputsView(_viewTreeView, _inputsViewBase, updateTree);
        ivbox.addWidget(_inputsView);
        _inputsView.setObjectName("inputsViewList");

        if (_css neq nil) _baseWidget.setStyleSheet(_css);
        _dockWidget.setWidget(_baseWidget);
        //_dockWidget.setTitleBarWidget(QWidget(m,0));
        _dockWidget.setTitleBarWidget(_baseWidget.findChild("navPanel"));
        _dockWidget.setObjectName(name);
        _eventFilter = EventFilter(mainWindowWidget());
        _dockWidget.installEventFilter(_eventFilter);

        //_viewModel   = QStandardItemModel(m);
        _viewModel   = NodeModel(m);
        _inputsModel = QStandardItemModel(m);

        _viewTreeView._viewModel = _viewModel;

        _viewModel.setHorizontalHeaderLabels(string[] {"Name", "*", "*"});
        _viewTreeView.header().setMinimumSectionSize(-1);

        _viewTreeView.setModel(_viewModel);
        _viewTreeView.setDragEnabled(true);
        _viewTreeView.setAcceptDrops(true);
        _viewTreeView.setShowDropIndicator(true);
        _viewTreeView.setHeaderHidden(false);
        _viewTreeView.setSelectionMode(QAbstractItemView.ExtendedSelection);
        _viewTreeView.setEditTriggers(QAbstractItemView.EditKeyPressed);
        _viewTreeView.setContextMenuPolicy(Qt.CustomContextMenu);
        _viewTreeView.setDragDropMode(QAbstractItemView.DragDrop);
        _viewTreeView.setDefaultDropAction(Qt.MoveAction);
        _viewTreeView.setExpandsOnDoubleClick(false);

        _inputsView.setModel(_inputsModel);
        _inputsView.setDragEnabled(true);
        _inputsView.setAcceptDrops(true);
        _inputsView.setSelectionMode(QAbstractItemView.ExtendedSelection);
        _inputsView.setSelectionBehavior(QAbstractItemView.SelectRows);
        _inputsView.setDefaultDropAction(Qt.MoveAction);
        _inputsView.setShowDropIndicator(true);
        _inputsView.setDragDropMode(QAbstractItemView.DragDrop);
        _inputsView.setEditTriggers(QAbstractItemView.NoEditTriggers);
        //_inputsView.setDragDropOverwriteMode(true);


        m.addDockWidget(Qt.LeftDockWidgetArea, _dockWidget);

        let addAction          = QAction(auxIcon("add_48x48.png", true), "Create View", _addButton),
            folderAction       = QAction(auxIcon("foldr_48x48.png", true), "Create Folder", _folderButton),
            deleteAction       = QAction(auxIcon("trash_48x48.png", true), "Delete View", _deleteButton),
            configAction       = QAction(auxIcon("confg_48x48.png", true), "Configure", _configButton),
            editInfoAction     = QAction(auxIcon("sinfo_48x48.png", true), "Edit View Info", _editViewInfoButton),
            orderUpAction      = QAction(auxIcon("up_48x48.png", true), "Move Input Higher in List", _orderUpButton),
            orderDownAction    = QAction(auxIcon("down_48x48.png", true), "Movie Input Lower in List", _orderDownButton),
            sortAscAction      = QAction(auxIcon("sortup_48x48.png", true), "A - Z", _sortAscButton),
            sortDescAction     = QAction(auxIcon("sortdown_48x48.png", true), "Z - A", _sortDescButton),
            inputsDeleteAction = QAction(auxIcon("trash_48x48.png", true), "Delete Input", _inputsDeleteButton),
            prevViewAction     = QAction(auxIcon("back_48x48.png", true), "Previous View", _prevViewButton),
            nextViewAction     = QAction(auxIcon("forwd_48x48.png", true), "Next View", _nextViewButton),
            homeAction         = QAction(auxIcon("home_48x48.png", true), "Select Current View", _homeButton);

        //
        //  Cache all the icons ahead of time (this was seriously
        //  slowing things down before). Put the icons in *reverse*
        //  order of likelyhood they'll appear. i.e. first in list is
        //  least likely to be needed, last is most likely.
        //

        _typeIcons = (string,QIcon)[]();

        for_each (t; [
                      ("RVSourceGroup", "videofile_48x48.png"),
                      ("RVImageSource", "videofile_48x48.png"),
                      ("RVSwitchGroup", "shuffle_48x48.png"),
                      ("RVRetimeGroup", "tempo_48x48.png"),
                      ("RVLayoutGroup", "lgicn_48x48.png"),
                      ("RVStackGroup", "photoalbum_48x48.png"),
                      ("RVSequenceGroup", "playlist_48x48.png"),
                      ("RVFolderGroup", "foldr_48x48.png"),
                      ("RVFileSource", "videofile_48x48.png")] )
        {
            _typeIcons.push_back( (t._0, auxIcon(t._1, true)) );
        }

        _viewIcon = auxIcon("view.png", true);
        _videoIcon = auxIcon("video_48x48.png", true);
        _channelIcon = auxIcon("channel.png", true);
        _layerIcon = auxIcon("layer.png", true);
        _unknownTypeIcon = auxIcon("new_48x48.png", true);

        _addButton.setDefaultAction(addAction);
        _deleteButton.setDefaultAction(deleteAction);
        _editViewInfoButton.setDefaultAction(editInfoAction);
        _addButton.setPopupMode(QToolButton.InstantPopup);

        _configButton.setDefaultAction(configAction);
        _configButton.setPopupMode(QToolButton.InstantPopup);

        _colorDialog = QColorDialog(m);
        _colorDialog.setOption(QColorDialog.ShowAlphaChannel, false);

        _orderUpButton.setDefaultAction(orderUpAction);
        _orderDownButton.setDefaultAction(orderDownAction);
        _sortAscButton.setDefaultAction(sortAscAction);
        _sortDescButton.setDefaultAction(sortDescAction);
        _inputsDeleteButton.setDefaultAction(inputsDeleteAction);

        _prevViewButton.setDefaultAction(prevViewAction);
        _nextViewButton.setDefaultAction(nextViewAction);
        _homeButton.setDefaultAction(homeAction);

        let addMenu          = QMenu("New Viewable", _addButton),
            addSequence      = addMenu.addAction(auxIcon("playlist_48x48.png", true), "Sequence"),
            addStack         = addMenu.addAction(auxIcon("photoalbum_48x48.png", true), "Stack"),
            addSwitch        = addMenu.addAction(auxIcon("shuffle_48x48.png", true), "Switch"),
            addFolder        = addMenu.addAction(auxIcon("foldr_48x48.png", true), "Folder"),
            addLayout        = addMenu.addAction(auxIcon("lgicn_48x48.png", true), "Layout"),
            addRetime        = addMenu.addAction(auxIcon("tempo_48x48.png", true), "Retime");

        QAction addColorize  = nil;
        QAction addOCIO      = nil;
        QAction addDynamic   = nil;
        QAction addUserNode  = nil;

	//
	//  For now, remove the rv/rvsdi/rvx dependency here.  Hide Dynamic node from everyone.
	//
        if (true || shortAppName() == "rvsdi" || shortAppName() == "rvx")
        {
            addColorize      = addMenu.addAction(auxIcon("new_48x48.png", true), "Color");
            addOCIO          = addMenu.addAction(auxIcon("new_48x48.png", true), "OCIO");
            if (system.getenv("RV_ENABLE_DYNAMIC_NODE", nil) neq nil)
            {
                addDynamic   = addMenu.addAction(auxIcon("new_48x48.png", true), "Dynamic");
            }
            addUserNode      = addMenu.addAction(auxIcon("new_48x48.png", true), "New Node by Type...");
        }

        let _                = addMenu.addSeparator(),
            addSRGBCChart    = addMenu.addAction(auxIcon("colorchart_48x48.png", true), "SRGB Color Chart..."),
            addACESCChart   = addMenu.addAction(auxIcon("colorchart_48x48.png", true), "ACES Color Chart..."),
            addCBars         = addMenu.addAction(auxIcon("ntscbars_48x48.png", true), "Color Bars..."),
            addBlack         = addMenu.addAction(auxIcon("video_48x48.png", true), "Black..."),
            addColor         = addMenu.addAction(auxIcon("video_48x48.png", true), "Color..."),
            addBlank         = addMenu.addAction(auxIcon("video_48x48.png", true), "Blank...");

        let menuActions      = [(addStack, "RVStackGroup"),
                                (addFolder, "RVFolderGroup"),
                                (addLayout, "RVLayoutGroup"),
                                (addSequence, "RVSequenceGroup"),
                                (addRetime, "RVRetimeGroup"),
                                (addSwitch, "RVSwitchGroup"),
                                (addCBars, "smptebars,%s.movieproc"),
                                (addSRGBCChart, "srgbcolorchart,%s.movieproc"),
                                (addACESCChart, "acescolorchart,%s.movieproc"),
                                (addBlack, "black,%s.movieproc"),
                                (addColor, "solid,%s.movieproc"),
                                (addBlank, "blank,%s.movieproc") ];

        if (true || shortAppName() == "rvsdi" || shortAppName() == "rvx")
        {
            if (system.getenv("RV_ENABLE_DYNAMIC_NODE", nil) neq nil)
            {
                menuActions  = [(addStack, "RVStackGroup"),
                                (addFolder, "RVFolderGroup"),
                                (addLayout, "RVLayoutGroup"),
                                (addSequence, "RVSequenceGroup"),
                                (addRetime, "RVRetimeGroup"),
                                (addSwitch, "RVSwitchGroup"),
                                (addColorize, "RVColor"),
                                (addOCIO, "RVOCIO"),
                                (addDynamic, "Dynamic"),
                                (addUserNode, ""),
                                (addSRGBCChart, "srgbcolorchart,%s.movieproc"),
                                (addACESCChart, "acescolorchart,%s.movieproc"),
                                (addCBars, "smptebars,%s.movieproc"),
                                (addBlack, "black,%s.movieproc"),
                                (addColor, "solid,%s.movieproc"),
                                (addBlank, "blank,%s.movieproc") ];
            }
            else
            {
                menuActions  = [(addStack, "RVStackGroup"),
                                (addFolder, "RVFolderGroup"),
                                (addLayout, "RVLayoutGroup"),
                                (addSequence, "RVSequenceGroup"),
                                (addRetime, "RVRetimeGroup"),
                                (addSwitch, "RVSwitchGroup"),
                                (addColorize, "RVColor"),
                                (addOCIO, "RVOCIO"),
                                (addUserNode, ""),
                                (addSRGBCChart, "srgbcolorchart,%s.movieproc"),
                                (addACESCChart, "acescolorchart,%s.movieproc"),
                                (addCBars, "smptebars,%s.movieproc"),
                                (addBlack, "black,%s.movieproc"),
                                (addColor, "solid,%s.movieproc"),
                                (addBlank, "blank,%s.movieproc") ];
            }
        }

        _addButton.setMenu(addMenu);
        _addButton.setArrowType(Qt.NoArrow);
        _createMenu = addMenu;

        let folderMenu       = QMenu("New Folder", _folderButton),
            newFolderAction  = folderMenu.addAction("Empty Folder"),
            newFolder2Action = folderMenu.addAction("From Selection"),
            newFolder3Action = folderMenu.addAction("From Copy of Selection");

        _folderButton.setDefaultAction(folderAction);
        _folderButton.setMenu(folderMenu);
        _folderButton.setArrowType(Qt.NoArrow);
        _folderButton.setPopupMode(QToolButton.InstantPopup);
        _folderMenu = folderMenu;

        let configMenu     = QMenu("Config", _configButton),
            configAlwaysOn = configMenu.addAction("Always Show at Start Up"),
            configNeverOn  = configMenu.addAction("Never Show at Start Up"),
            configLastOn   = configMenu.addAction("Restore Last State at Start Up"),
            configGroup    = QActionGroup(_configButton);

        for_each (a; [configAlwaysOn, configNeverOn, configLastOn])
        {
            a.setCheckable(true);
            configGroup.addAction(a);
        }

        _configButton.setMenu(configMenu);

        try
        {
            use SettingsValue;

            let String configState = readSetting("SessionManager", "showOnStartup", String("no"));
            
            case (configState)
            {
                "yes"   -> { configAlwaysOn.setChecked(true); }
                "no"    -> { configNeverOn.setChecked(true); }
                "last"  -> { configLastOn.setChecked(true); }
                _       -> { configNeverOn.setChecked(true); }
            }
        }
        catch (...)
        {
            ;
        }

        for_each (a; menuActions) 
        {
            let (action, protocol) = a;
            connect(action, QAction.triggered, addThingSlot(,protocol));
        }

        _viewContextMenuActions = QAction[] {
            deleteAction, editInfoAction, homeAction };

        connect(homeAction, QAction.triggered, selectCurrentViewSlot);
        connect(deleteAction, QAction.triggered, deleteViewableSlot);
        connect(editInfoAction, QAction.triggered, editViewInfoSlot);
        connect(orderUpAction, QAction.triggered, reorderSelected(true,));
        connect(orderDownAction, QAction.triggered, reorderSelected(false,));
        connect(sortAscAction, QAction.triggered, sortInputs(true,));
        connect(sortDescAction, QAction.triggered, sortInputs(false,));
        connect(inputsDeleteAction, QAction.triggered, inputsDeleteSlot);
        connect(prevViewAction, QAction.triggered, navButtonClicked ("prev", ));
        connect(nextViewAction, QAction.triggered, navButtonClicked ("next", ));
        connect(_tabWidget, QTabWidget.currentChanged, tabChangeSlot);
        connect(_inputsModel, QAbstractItemModel.rowsRemoved, inputRowsRemovedSlot);
        connect(_inputsModel, QAbstractItemModel.rowsInserted, inputRowsInsertedSlot);
        connect(_lazySetInputsTimer, QTimer.timeout, rebuildInputsFromList);
        connect(_lazyUpdateTimer, QTimer.timeout, updateTree);
        connect(_mainWinVisTimer, QTimer.timeout, mainWinVisTimeout);
        connect(_colorDialog, QColorDialog.currentColorChanged, newColorSlot);
        connect(_splitter, QSplitter.splitterMoved, splitterMoved);
        connect(configAlwaysOn, QAction.triggered, configSlot(,"yes",true));
        connect(configNeverOn, QAction.triggered, configSlot(,"no",false));
        connect(configLastOn, QAction.triggered, configSlot(,"last",true));
        connect(_viewModel, QStandardItemModel.itemChanged, viewItemChanged);
        connect(_viewTreeView, QTreeView.expanded, setItemExpandedState(,1));
        connect(_viewTreeView, QTreeView.collapsed, setItemExpandedState(,0));
        connect(_viewTreeView, QTreeView.customContextMenuRequested, viewContextMenuSlot);
        connect(_viewTreeView, QAbstractItemView.doubleClicked, viewByIndex(,_viewModel));
        connect(_viewTreeView, QAbstractItemView.pressed, itemPressed(,_viewModel));
        connect(_inputsView, QListView.doubleClicked, viewByIndex(,_inputsModel));
        connect(newFolderAction, QAction.triggered, newFolderSlot(,1));
        connect(newFolder2Action, QAction.triggered, newFolderSlot(,2));
        connect(newFolder3Action, QAction.triggered, newFolderSlot(,3));

        //
        //  Create the props on the display node we'll use
        //

        updateTree();

        //print(document_symbol("qt.QItemSelectionModel"));

        _dockWidget.show();
        m.show();

        let sprop = "#Session.sm_window.splitter";

        if (propertyExists(sprop))
        {
            let fpos = getFloatProperty(sprop).front();
            //_splitter.
        }

        State state = data();
        state.sessionManager = this;

        connect(_dockWidget, QDockWidget.visibilityChanged, visibilityChanged);

        updateNavUI();
    }
}

\: createMode (Mode;)
{
    return SessionManagerMode("session_manager");
}

\: theMode (SessionManagerMode; )
{
    SessionManagerMode m = rvui.minorModeFromName("session_manager");

    return m;
}

}
