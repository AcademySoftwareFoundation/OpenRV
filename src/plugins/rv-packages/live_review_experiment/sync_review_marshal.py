from rv import commands
from rv import rvtypes
from rv.rvtypes import MinorMode

import os
import json

import otio_writer
import otio_reader


class SyncReviewMarshal(MinorMode):
    updating_graph = False
    updating_playbacksettings = False
    queue_name = ""

    def __init__(self):
        super(SyncReviewMarshal, self).__init__()

        self.init(
            "sync_review_marshal",
            [
                (
                    "graph-node-inputs-changed",
                    self.send_graph_change,
                    "Turn the visible graph into a sync review payload",
                ),
                (
                    "play-start",
                    self.send_playback_start,
                    "Turn the playack settings into a sync review payload",
                ),
                (
                    "play-stop",
                    self.send_playback_stop,
                    "Turn the playack settings into a sync review payload",
                ),
                (
                    "sync-review-change-received",
                    self.receive_change_event,
                    "",
                ),
                (
                    "sync-review-queue-name-change",
                    self.set_queue_name,
                    "",
                )
            ],
            None,
        )

    @staticmethod
    def get_current_otio_time():
        """
        Return an OTIO RationalTime corresponding to the current time
        """
        # TODO: There's no __dict__ method on RationalTime, so do it manually for now
        return {
            "OTIO_SCHEMA": "RationalTime.1",
            "rate": commands.fps(),
            "value": commands.frame(),
        }

    @staticmethod
    def extract_frame_payload(payload):
        """
        Return an OTIO RationalTime corresponding to the current time
        """
        # TODO: There's no __dict__ method on RationalTime, so do it manually for now

        return {
            "OTIO_SCHEMA": "RationalTime.1",
            "rate": commands.fps(),
            "value": commands.frame(),
        }

    @staticmethod
    def send_sync_review_event(event_name, payload):
        """
        Sends an event with a json payload
        """
        if os.environ.get("DEBUG_SYNC_REVIEW"):
            print(f"SEND MESSAGE to session {SyncReviewMarshal.queue_name}: {payload}")

        commands.sendInternalEvent(
            event_name,
            json.dumps(
                {"schema": "SYNC_REVIEW_1.0", "session": SyncReviewMarshal.queue_name, "payload": payload},
                separators=(",", ":"),
                indent=1,
                sort_keys=True,
            ),
        )

    @staticmethod
    def marshal_node(node):
        """
        Converts a node graph into a sync review session message
        """
        SyncReviewMarshal.send_sync_review_event(
            "sync-review-change",
            {
                "command_schema": "OTIO_SESSION_1.0",
                "command": {
                    "event": "SET",
                    "payload": {
                        "otio": json.loads(otio_writer.write_otio_string(node)),
                    },
                },
            },
        )

    @staticmethod
    def marshal_playback_settings(**kwargs):
        """
        Takes the kwargs turns it into sync review playback message
        """
        SyncReviewMarshal.send_sync_review_event(
            "sync-review-change",
            {
                "command_schema": "PLAYBACK_SETTINGS_1.0",
                "command": {
                    "event": "SET",
                    "payload": kwargs,
                },
            },
        )

    @staticmethod
    def extract_payload(message):
        """
        Extracts a message payload for the supplied command_schema
        """
        message_json = json.loads(message)
        if message_json.get("schema") != "SYNC_REVIEW_1.0":
            print(f"Unhandled message schema: {message_json.get('schema')}")
            return "", ""

        payload = message_json.get("payload")
        if payload is None:
            print(f"Message has no payload: {message}")
            return "", ""

        command_schema = payload.get("command_schema")
        if command_schema is None:
            print(f"Message has no command_schema: {message}")
            return "", ""

        command = payload.get("command")
        if command is None:
            print(f"message has no command: {message}")
            return "", ""

        return command_schema, command

    @staticmethod
    def send_graph_change(event):
        """
        Takes the currently viewed node and turns it into sync review message
        """
        event.reject()
        if SyncReviewMarshal.updating_graph:
            return

        SyncReviewMarshal.marshal_node(commands.viewNode())

    @staticmethod
    def send_playback_start(event):
        """
        Sends a synced review message that playback started
        """
        event.reject()

        if SyncReviewMarshal.updating_playbacksettings:
            return

        SyncReviewMarshal.marshal_playback_settings(
            playing=True, current_time=SyncReviewMarshal.get_current_otio_time()
        )

    @staticmethod
    def send_playback_stop(event):
        """
        Sends a synced review message that playback stopped
        """
        event.reject()

        if SyncReviewMarshal.updating_playbacksettings:
            return

        SyncReviewMarshal.marshal_playback_settings(
            playing=False, current_time=SyncReviewMarshal.get_current_otio_time()
        )

    @staticmethod
    def receive_change_event(event):
        """
        Receives a change message and updates the graph
        """
        event.reject()
        command_schema, command = SyncReviewMarshal.extract_payload(event.contents())
        if not command_schema or not command:
            return

        if command_schema.startswith("OTIO_SESSION_1"):
            print("Processing OTIO_SESSION_1 Change")
            return SyncReviewMarshal.receive_graph_change(command)
        elif command_schema.startswith("PLAYBACK_SETTINGS_1"):
            print("Processing PLAYBACK_SETTINGS_1 Change")
            return SyncReviewMarshal.receive_playback_change(command)

    @staticmethod
    def set_queue_name(event):
        """
        Receives the current name of the message queue
        """
        event.reject()
        SyncReviewMarshal.queue_name = event.contents() 

    @staticmethod
    def receive_graph_change(command):
        """
        Receives a graph change message and updates the graph
        """
        if command.get("event") != "SET":
            return

        payload = command.get("payload")
        if payload is None:
            return

        new_otio = payload.get("otio")
        new_otio = json.dumps(new_otio, indent=1, sort_keys=True)

        # Not really optimal, but required to compare both otio strings
        old_otio = otio_writer.write_otio_string(commands.viewNode())
        old_otio = json.loads(old_otio)
        old_otio = json.dumps(old_otio, indent=1, sort_keys=True)

        if new_otio == old_otio:
            return

        print(f"Updating Graph using OTIO\n{new_otio}")

        SyncReviewMarshal.updating_graph = True
        commands.clearSession()

        if new_otio != "{}":
            root_node = otio_reader.read_otio_string(new_otio)
            commands.setViewNode(root_node)
        SyncReviewMarshal.updating_graph = False

    @staticmethod
    def receive_playback_change(command):
        """
        Recieves a playback settings messages and updates the playback settings
        """

        if command.get("event") != "SET":
            return

        payload = command.get("payload")
        if payload is None:
            return

        SyncReviewMarshal.updating_playbacksettings = True
        playing = payload.get("playing")
        if playing is not None:
            if playing:
                commands.play()
            else:
                commands.stop()
                current_time = payload.get("current_time")
                if current_time:
                    frame = current_time.get("value")
                    if frame:
                        commands.setFrame(frame)
        SyncReviewMarshal.updating_playbacksettings = False


_mode = None


def createMode():
    global _mode
    _mode = SyncReviewMarshal()
    return _mode


def getMode():
    return _mode or createMode()
