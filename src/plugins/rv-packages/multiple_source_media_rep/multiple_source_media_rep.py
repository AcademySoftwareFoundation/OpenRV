# Copyright (c) 2021 Autodesk.
#
# CONFIDENTIAL AND PROPRIETARY
#
# This work is provided "AS IS" and subject to the ShotGrid Pipeline Toolkit
# Source Code License included in this distribution package. See LICENSE.
# By accessing, using, copying or modifying this work you indicate your
# agreement to the ShotGrid Pipeline Toolkit Source Code License. All rights
# not expressly granted therein are reserved by ShotGrid Software Inc.

from PySide2 import QtCore, QtGui, QtWidgets

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
        layout.setContentsMargins(2, 11, 2, 2)
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

    def set_values(self, rep, resolution, extension):
        rep_txt = "<font color='DarkGray'>%s</font>" % rep
        ext_txt = "<font color='Gray'>%s</color>" % extension
        res_txt = "<font color='DarkGray' size='0.6em'>%s</font>" % resolution
        self._label.setText("%s %s<br>%s" % (rep_txt, ext_txt, res_txt))

        self.setData(rep)

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
    _current_source = None
    _current_switch_node = None
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
        rvc.setActiveSourceMediaRep(self._current_switch_node, action.data())

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

        for action in self._media_representation_btn.menu().actions():
            if action.isEnabled():
                action.setChecked(action.data() == self._current_media_rep_name)

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

    def _populate_media_rep_menu(self, reps, infos):
        """
        Populates the media rep drop down menu with the available media representations
        and their associated media infos.
        """
        # TODO Reuse old actions instead of clearing and creating all anew
        menu = self._media_representation_btn.menu()
        menu.clear()

        # Menu title.
        title = QtWidgets.QAction("Media Playback", menu)
        title.setEnabled(False)
        menu.addAction(title)

        # The menu items are composed of the media rep name, the extension and
        # the resolution.
        for rep in reps:
            resolution = ""
            extension = ""

            if rep in infos and infos[rep]:
                if "width" in infos[rep]:
                    resolution = "%s x %s" % (infos[rep]["width"], infos[rep]["height"])
                if "file" in infos[rep]:
                    extension = (
                        "URL"
                        if utils.is_url(infos[rep]["file"])
                        else infos[rep]["file"].split(".")[-1].upper()
                    )

            action = MediaRepresentationItem(menu)
            action.set_values(rep, resolution, extension)
            menu.addAction(action)

        return menu

    def _update_media_rep_menu(self):
        """
        Updates the dropdown menu based on the available media reps of the
        current source.
        """
        media_reps = rvc.sourceMediaReps(self._current_switch_node)

        # Fetch the resolution and extension for all the media reps.
        infos = {k: {} for k in media_reps}
        for s in utils.get_media_reps_sources(self._current_switch_node):
            try:
                rep = rvc.getStringProperty(s + ".media.repName")[0]
                infos[rep] = utils.get_source_media_info(s)

            except:
                # Source node is not ready. Ignore that issue since the
                # information will be gather later on if needed.
                continue

        # Populate the menu with the updated information.
        self._populate_media_rep_menu(media_reps, infos)

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
        has_media_rep = False

        # Ensure that there is a RVSwitchGroup in evaluation path.
        # Note: Current source might be part of a RVSwitchGroup, but
        # depending on the view node, the RVSwitchGroup may not be in
        # the evaluation path.
        # RVSwitchGroup node is NOT present if view node is the source
        # node itself.
        rep = ""
        if self._current_switch_node:
            media_reps = rvc.sourceMediaReps(self._current_switch_node)

            if media_reps and media_reps[0]:
                rep = rvc.sourceMediaRep(self._current_switch_node)

                # The name must have a maximum of 20 characters.
                label = rep if len(rep) <= 20 else "%s..." % rep[0:17]
                self._media_representation_btn.setText(label)

        self._current_media_rep_name = rep
        self._show_media_representation(rep != "")

    def _update_resolution_and_extension(self):
        """
        Updates the resolution and extension labels based on the
        media info of the current source.
        """
        infos = utils.get_source_media_info(self._current_source)
        if infos:
            resolution = "%s x %s" % (infos["width"], infos["height"])
            extension = (
                "URL"
                if utils.is_url(infos["file"])
                else infos["file"].split(".")[-1].upper()
            )
        else:
            resolution = ""
            extension = ""

        self._media_resolution_lbl.setText(resolution)
        self._media_extension_lbl.setText(extension)

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

        # Important: Skip update if same source as current.
        source = utils.get_source_at_current_frame()
        if source == self._current_source and not self._force_update_media_info:
            return

        # Update current source and UI.
        self._current_source = source
        self._current_switch_node = utils.get_switch_node(source)
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
        if self._current_source is None:
            return
        if (self._current_source == source_deleted) or (
            source_deleted == rvc.nodeGroup(self._current_source)
        ):
            self._current_source = None

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

    def _on_new_node(self, event):
        """
        This method is bound to the new-node event.
        It forces an UI update if a SwitchGroup just got inserted in the
        evaluation path of the current source node.
        Note: When a new node is added for media rep, a SwitchGroup gets
        inserted to connect the different source media representations.
        Note:
        """
        package_logger.debug("%s in _on_new_node" % event.name())

        event.reject()

        if self._current_source and not self._current_switch_node:
            if utils.get_switch_node(self._current_source):
                self._force_update_media_info = True
                self._update_media_info(event)

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
                    self._update_media_info,
                    "",
                ),
                ("after-graph-view-change", self._on_force_update_media_info, ""),
                ("new-node", self._on_new_node, ""),
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
