# Copyright (c) 2021 Autodesk.
#
# CONFIDENTIAL AND PROPRIETARY
#
# This work is provided "AS IS" and subject to the ShotGrid Pipeline Toolkit
# Source Code License included in this distribution package. See LICENSE.
# By accessing, using, copying or modifying this work you indicate your
# agreement to the ShotGrid Pipeline Toolkit Source Code License. All rights
# not expressly granted therein are reserved by ShotGrid Software Inc.

try:
    from PySide2 import QtCore, QtGui, QtWidgets
except ImportError:
    try:
        from PySide6 import QtCore, QtGui, QtWidgets
    except ImportError:
        pass

from rv import rvtypes, qtutils
from rv import commands as rvc
from rv import extra_commands as rve

from multiple_source_media_rep_logger import package_logger
import multiple_source_media_rep_utils as utils


class MediaRepresentationItem(QtWidgets.QWidgetAction):
    """
    Custom Qt Widget for the items in the media rep dropdown menu.
    Each item is composed of three information: rep name, extension, and resolution.
    """

    _check_mark = None
    _label = None

    def __init__(self, parent):
        super(MediaRepresentationItem, self).__init__(parent)
        self._build_ui()

    def _build_ui(self):
        # Let's use a parent widget that will handle the mouse events
        # and take care of the background color.
        widget = QtWidgets.QWidget()
        widget.setStyleSheet(
            "QWidget"
            "{"
            "margin: 0px;"
            "padding: 0px;"
            "margin-top: 5px;"
            "}"
            "QWidget::hover"
            "{"
            "background-color: rgb(62, 62, 132);"
            "}"
        )

        # Using a layout to position the labels.
        layout = QtWidgets.QHBoxLayout(widget)
        layout.setContentsMargins(2, 6, 2, 2)
        layout.setAlignment(QtCore.Qt.AlignLeft)

        # Using a label to mimic the QAction's tick on the left side
        # of the label.
        # Size policy to retain the widget's size when it is hidden.
        # No background so that it uses the parent's one.
        # Does not listen for mouse events.
        check_mark = QtWidgets.QLabel(widget)
        check_mark.setText("\u2713")
        policy = check_mark.sizePolicy()
        policy.setRetainSizeWhenHidden(True)
        check_mark.setSizePolicy(policy)
        check_mark.setStyleSheet(
            "background: none;"
            "margin: 0px;"
            "padding: 0px;"
            "color: DarkGray;"
            "font-size: 12px;"
        )
        check_mark.setAttribute(QtCore.Qt.WA_TransparentForMouseEvents)
        layout.addWidget(check_mark)
        self._check_mark = check_mark

        # The label that will show the media information.
        # No background so that it uses the parent's one.
        # Does not listen for mouse events.
        label = QtWidgets.QLabel(widget)
        label.setStyleSheet("background: none;" "margin: 0px;" "padding: 0px;")
        label.setAttribute(QtCore.Qt.WA_TransparentForMouseEvents)
        layout.addWidget(label)
        self._label = label

        widget.setLayout(layout)
        self.setDefaultWidget(widget)

    def set_values(self, rep, resolution, extension, switch_nodes):
        rep_txt = "<font color='DarkGray'>%s</font>" % rep
        res_txt = "<font color='DarkGray' size='0.6em'>%s</font>" % resolution
        ext_txt = "<font color='DarkGray' size='0.6em'>%s</font>" % extension
        self._label.setText(
            "%s<br>%s&nbsp;&nbsp;&nbsp;%s" % (rep_txt, res_txt, ext_txt)
        )

        self.setData({"rep": rep, "switch_nodes": switch_nodes})

    def mousePressEvent(self, _):
        self._action.trigger()
        self.parentWidget().hide()

    def setChecked(self, value):
        super(MediaRepresentationItem, self).setChecked(value)
        self._check_mark.setVisible(value)


class MultipleSourceMediaRepMode(rvtypes.MinorMode):
    """
    This plugin is provided as an example of how to manage multiple source
    media representations in RV.

    You can replace it wholesale by doing this:

      rvc.bind("Multiple Source Media Rep", "global", "source-media-unavailable", myFunc, "")

    (E.g. from your ~/.rvrc.py file or a facility init if you
    don't want to edit this directly).

    Or by creating your own custom rv package based on this example.

    ORDERING: this mode uses a sort key of "multiple_source_media_rep" with an ordering
    value of 1. If you create a mode which piggybacks off of the
    results of this one for source-media-unavailable you should also use
    "multiple_source_media_rep" as the sort key but with an ordering number *after*
    1. If you want yours *before* this mode do the same, but use 0 or a
    negative ordering number.
    """

    # Caching current source to avoid useless refresh
    _current_sources = []
    _current_media_rep_name = None

    # Media representation widgets
    _bottomToolBar = None
    _media_representation_btn = None
    _media_resolution_lbl = None
    _media_extension_lbl = None

    # Media representation widgets' actions
    _media_representation_action = None

    # This is used to postponed received events until progressive loading
    # gets completed.
    _in_progressive_loading = False

    # This is used to force an update on the next event
    _force_update_media_info = False

    # This method is bound to the source-media-unavailable event.
    # It is called any time a source media representation is unavailable for RV
    # to load so that a fallback source media representation replacement can be
    # selected.
    #
    # It is provided as an example so that you can adapt it to implement your
    # own multiple media representation logic.
    #
    # It currently implements the following standard ShotGrid logic in priority
    # order:
    #   1. Frames       - sg_path_to_frames
    #   2. Movie        - sg_path_to_movie
    #   3. Streaming    - transcoded streamable media from the ShotGrid server
    #
    # In other words, it will prioritize the 'Frames' source media
    # representation if available, which represents an image sequence typically
    # at higher resolution. If this representation is not available, then the
    # 'Movie' media representation will be selected, and so on.
    def sourceMediaUnavailable(self, event):

        package_logger.debug("%s in sourceMediaUnavailable" % event.name())

        #  event.reject() is done to allow other functions bound to
        #  this event to get a chance to modify the state as well. If
        #  its not rejected, the event will be eaten and no other call
        #  backs will occur.
        event.reject()

        if self._readingSession:
            # Wait until the session reading is done before taking any action
            return

        # The arguments of the source-media-unavailable event are:
        # nodename;;filename;;mediarepname
        args = event.contents().split(";;")
        expectedNumberOfArgs = 3
        if len(args) != expectedNumberOfArgs:
            package_logger.error(
                "source-media-unavailable has {} parameters, expected={}".format(
                    len(args), expectedNumberOfArgs
                )
            )
            return

        sourceNodeName = args[0]
        mediaRepPath = args[1]
        mediaRepName = args[2]

        # Determine the media rep name to fall back to since this one is not available
        fallbackMediaRepName = self._getFallbackMediaRepName(
            sourceNodeName, mediaRepName
        )

        package_logger.debug(
            "sourceMediaUnavailable()-sourceNodeName={}, mediaRepName={}, fallbackMediaRepName={}".format(
                sourceNodeName, mediaRepName, fallbackMediaRepName
            )
        )

        # Select the fallback media rep
        if fallbackMediaRepName is not None:
            fallbackNoticeText = "{} ({}) is not available, falling back to {}".format(
                mediaRepName, mediaRepPath, fallbackMediaRepName
            )
            rve.displayFeedback(fallbackNoticeText, 3.0)
            package_logger.info(fallbackNoticeText)
            rvc.setActiveSourceMediaRep(sourceNodeName, fallbackMediaRepName)

    # Figure out and return the fallback source media rep name if any
    def _getFallbackMediaRepName(self, sourceNodeName, unavailableMediaRepName):
        # Find the source media representations available
        mediaRepNames = rvc.sourceMediaReps(sourceNodeName)

        # If there is only one, then there is no other source media
        # representation to fall back on
        if len(mediaRepNames) <= 1:
            return None

        # Sort the available source media representations and mark the position
        # of the current representation in the list
        unavailableMediaRepPos = -1
        mediaRepNamesSorted = []
        for mediaRepName in self.sourceMediaRepsInPriorityOrder:
            if mediaRepName in mediaRepNames:
                if unavailableMediaRepName == mediaRepName:
                    unavailableMediaRepPos = len(mediaRepNamesSorted)
                mediaRepNamesSorted.append(mediaRepName)

        # Find the position of the current source media representation in the list
        fallbackMediaRepPos = unavailableMediaRepPos + 1
        if fallbackMediaRepPos < len(mediaRepNamesSorted):
            return mediaRepNamesSorted[fallbackMediaRepPos]

        # Otherwise this means we have reached the end of the possible fallback options
        return None

    def beforeSessionRead(self, event):
        package_logger.debug(event.name())

        event.reject()
        self._readingSession = True

    def afterSessionRead(self, event):
        package_logger.debug(event.name())

        event.reject()
        self._readingSession = False

        # Refresh all the active source media representations
        rvc.setActiveSourceMediaRep("all", self.sourceMediaRepsInPriorityOrder[0])

    def _on_media_rep_changed(self, action):
        """
        This method is called any time a new media representation
        get selected from the dropdown menu.
        It calls the 'setActiveSourceMediaRep' RV commands to set the active input to
        the newly selected media representation specified.
        """
        for switch_node in action.data()["switch_nodes"]:
            if action.data()["rep"] in rvc.sourceMediaReps(switch_node):
                rvc.setActiveSourceMediaRep(switch_node, action.data()["rep"])

    def _on_media_rep_about_to_show(self):
        """
        This method is called right before the menu get shown.
        It updates the menu items if needed and add a check mark to
        the action item referring to the current active media rep.
        """
        # FIXME: Sources properties seem to not be all available even if all
        #        sources have been added via the addSourceMediaRep.
        #        If we switch to a new media rep, then open the dropdown menu,
        #        RV might still be unable to retrieve the props (repName, media infos)
        #        of this new media rep, even if it is loaded in the player.
        #        debugging with multiple prints solve the issue by itself, so
        #        there seems to be a race condition. Also, fetching the same property
        #        a second time might also solve the issue.
        #        Note: We do not have issue when switching the media rep one by one
        #        from the first to the last added.
        self._update_media_rep_menu()

    def _build_media_representation_widgets(self):
        """
        Adds a dropdown menu along side the media quality info on the left of the playback mode selector.
        The new dropdown menu can be use to swap the media representation of the current media with
        one of the other available source media representations.
        """
        # This needs to be kept so that the actions variable get not garbage collected
        # after the function call.
        self._bottomToolBar = qtutils.sessionBottomToolBar()

        playback_style_action = None
        for action in self._bottomToolBar.actions():
            if action.toolTip() == QtCore.QObject().tr("Select playback style"):
                playback_style_action = action

        # Dropdown menu for the media reps.
        self._media_representation_btn = QtWidgets.QToolButton(self._bottomToolBar)
        self._media_representation_btn.setToolTip("Select media playback type")
        self._media_representation_btn.setProperty("tbstyle", "solo_menu")
        self._media_representation_btn.setStyleSheet("color: gray")
        self._media_representation_btn.setPopupMode(QtWidgets.QToolButton.InstantPopup)

        menu = QtWidgets.QMenu(self._media_representation_btn)
        menu.triggered.connect(self._on_media_rep_changed)
        menu.aboutToShow.connect(self._on_media_rep_about_to_show)
        self._media_representation_btn.setMenu(menu)

        self._media_representation_action = self._bottomToolBar.insertWidget(
            playback_style_action, self._media_representation_btn
        )
        self._media_representation_action.setVisible(False)

        # Media resolution.
        self._media_resolution_lbl = QtWidgets.QLabel("", self._bottomToolBar)
        self._media_resolution_lbl.setStyleSheet(
            "color: gray; background-color: transparent; margin-right: 5px"
        )
        self._bottomToolBar.insertWidget(
            playback_style_action, self._media_resolution_lbl
        )

        # Media extension.
        self._media_extension_lbl = QtWidgets.QLabel("", self._bottomToolBar)
        self._media_extension_lbl.setStyleSheet(
            "color: gray; background-color: transparent"
        )
        self._bottomToolBar.insertWidget(
            playback_style_action, self._media_extension_lbl
        )

    def _populate_media_rep_menu(self, menu, switch_nodes):
        """
        Populates the media rep drop down menu with the available media representations
        and their associated media infos.
        """

        active_rep = utils.get_media_rep(switch_nodes)

        # Calculate the union of all the media reps along with their associated source nodes
        reps_and_source_nodes = {}
        for switch_node in switch_nodes:
            reps_and_nodes = rvc.sourceMediaRepsAndNodes(switch_node)
            for rep, source_node in reps_and_nodes:
                if rep in reps_and_source_nodes.keys():
                    reps_and_source_nodes[rep].append(source_node)
                else:
                    reps_and_source_nodes[rep] = [source_node]

        # Fetch the resolution and extension for all the media reps.
        # The menu items are composed of the media rep name, the extension and
        # the resolution.
        for rep in reps_and_source_nodes.keys():
            source_media_infos = utils.get_common_source_media_infos(
                reps_and_source_nodes[rep]
            )

            action = MediaRepresentationItem(menu)
            action.set_values(
                rep,
                source_media_infos.resolution,
                source_media_infos.extension,
                switch_nodes,
            )
            menu.addAction(action)
            action.setChecked(rep == active_rep)

    def _update_media_rep_menu(self):
        """
        Updates the dropdown menu based on the available media reps of the
        current source.
        """

        menu = self._media_representation_btn.menu()
        menu.clear()

        switch_nodes = utils.get_switch_nodes_at_current_frame()
        if not switch_nodes:
            return

        # Menu title.
        title = QtWidgets.QAction("Media Playback", menu)
        title.setEnabled(False)
        menu.addAction(title)
        menu.addSeparator()

        # Populate the menu with the updated information.
        if len(switch_nodes) == 1:
            # There is only one source at the current frame
            # Add that source's media representations to the menu
            self._populate_media_rep_menu(menu, switch_nodes)
        else:
            # There are multiple sources at the current frame
            # Add the union of all the sources's media representations to the menu
            self._populate_media_rep_menu(menu, switch_nodes)

            menu.addSeparator()

            # Then add each source's media representations to the menu
            for switch_node in switch_nodes:
                src_menu = menu.addMenu(rve.uiName(switch_node))
                self._populate_media_rep_menu(src_menu, [switch_node])
                menu.addSeparator()

    def _show_media_representation(self, show):
        """
        Shows or hides the media rep drop down menu
        """
        if self._media_representation_action.isVisible() != show:
            self._media_representation_action.setVisible(show)
            self._bottomToolBar.repaint()

    def _update_media_representation(self):
        """
        Update the label of the media rep drop down menu and sets
        its visibility.
        """

        # Ensure that there is a RVSwitchGroup in evaluation path.
        # Note: Current source might be part of a RVSwitchGroup, but
        # depending on the view node, the RVSwitchGroup may not be in
        # the evaluation path.
        # RVSwitchGroup node is NOT present if view node is the source
        # node itself.
        rep = utils.get_media_rep_at_current_frame()
        if rep:
            # The name must have a maximum of 20 characters.
            label = rep if len(rep) <= 20 else f"{rep[0:17]}..."
            self._media_representation_btn.setText(label)

        self._current_media_rep_name = rep
        self._show_media_representation(rep != "")

    def _update_resolution_and_extension(self):
        """
        Updates the resolution and extension labels based on the media info of
        the current sources.
        Note: when there are multiple sources at the current frame then the
        resolution and extension is only shown when they are common to all
        """
        common_source_media_infos = utils.get_common_source_media_infos(
            self._current_sources
        )

        self._media_resolution_lbl.setText(common_source_media_infos.resolution)
        self._media_extension_lbl.setText(common_source_media_infos.extension)

    def _update_media_info(self, event):
        """
        Updates the media representation components if necessary.
        This method is called any time a new source is added, replaced, swapped,
        or cleared, and when changing frame.
        """
        package_logger.debug("%s in _update_media_info" % event.name())

        event.reject()

        # Do not update the media info when in progressive loading.
        # This will be called a second time on after-progressive-loading.
        if self._in_progressive_loading:
            return

        # Important: Skip update if same sources as current.
        sources = rvc.sourcesAtFrame(rvc.frame())
        if sources == self._current_sources and not self._force_update_media_info:
            return

        # Update current sources and UI.
        self._current_sources = sources
        self._force_update_media_info = False

        self._update_media_representation()
        self._update_resolution_and_extension()

    def _before_progressive_loading(self, event):
        """
        This method is bound to the before-progressive-loading event.
        It turns off updates until progressive loading is completed.
        """
        package_logger.debug(event.name())

        event.reject()
        self._in_progressive_loading = True

    def _after_progressive_loading(self, event):
        """
        This method is bound to the after_progressive_loading event.
        It re-enables the updates and force an UI update.
        """
        event.reject()
        self._in_progressive_loading = False
        self._update_media_info(event)

    def _before_source_delete(self, event):
        """
        This method is bound to the before-source-delete event.
        """
        event.reject()
        source_deleted = event.contents()

        # With the aim of avoiding useless UI refreshes, the current source is
        # cached. However, we need to reset the cached source if it gets deleted,
        # otherwise the UI might not reflect the state of a new source if it
        # happens to have the same name
        # Note that the source about to be deleted could be either a source or a
        # source group.
        if self._current_sources is None:
            return
        if (source_deleted in self._current_sources) or (
            source_deleted + "_source" in self._current_sources
        ):
            self._current_sources = None

    def _on_force_update_media_info(self, event):
        """
        This method is bound to the following events:
         - source-media-set
         - source-modified
         - after-graph-view-change
        It sets the _force_update_media_info flag to True to force an update and then
        calls the _update_media_info method.
        Events order:
          - source-group-complete (media not ready)
          - source-media-set, or source-modified (media ready)
        """
        event.reject()
        self._force_update_media_info = True
        self._update_media_info(event)

    def _on_postpone_force_update_media_info(self, event):
        """
        This method is bound to the following events:
         - media-relocated
         - source-media-rep-activated
        It sets the _force_update_media_info flag to True so that the UI
        gets updated at the next event (i.e. source-group-complete), even
        if the node source does not change.
        Events order:
          - media-relocated, source-media-rep-activated (media not ready)
          - source-group-complete (media ready)
        """
        package_logger.debug(
            "%s in _on_postpone_force_update_media_info" % event.name()
        )

        event.reject()
        self._force_update_media_info = True

    def __init__(self):
        self._readingSession = False

        rvtypes.MinorMode.__init__(self)

        self.init(
            "Multiple Source Media Rep",
            None,
            [
                ("after-session-read", self.afterSessionRead, ""),
                ("before-session-read", self.beforeSessionRead, ""),
                (
                    "source-media-unavailable",
                    self.sourceMediaUnavailable,
                    "Fallback mechanism when a specific source media representation is unavailable",
                ),
                ("before-progressive-loading", self._before_progressive_loading, ""),
                ("after-progressive-loading", self._after_progressive_loading, ""),
                ("before-source-delete", self._before_source_delete, ""),
                ("source-group-complete", self._update_media_info, ""),
                ("frame-changed", self._update_media_info, ""),
                ("session-clear-everything", self._update_media_info, ""),
                ("source-media-set", self._on_force_update_media_info, ""),
                ("source-modified", self._on_force_update_media_info, ""),
                ("media-relocated", self._on_postpone_force_update_media_info, ""),
                (
                    "source-media-rep-activated",
                    self._on_force_update_media_info,
                    "",
                ),
                ("after-graph-view-change", self._on_force_update_media_info, ""),
                (
                    "multiple-source-media-rep-update-ui",
                    self._on_force_update_media_info,
                    "",
                ),
            ],
            None,
            "multiple_source_media_rep",
            1,
        )

        # Multiple source media representation in order of priority
        self.sourceMediaRepsInPriorityOrder = ["Frames", "Movie", "Streaming"]

        self._build_media_representation_widgets()


def createMode():
    return MultipleSourceMediaRepMode()
