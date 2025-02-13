#! /usr/bin/python
#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
from __future__ import print_function

import os
import socket
import sys
import time
import six


class RvCommunicator:
    """
    Wrap up connection and communciation with a running RV.  The
    target RV process must have networking turned on and be
    listening on some well-known port (you can make both these
    happen from the command line: "-network -networkPort 45129".
    By default, RV will listen on port 45124.

    NOTE: if the target RV is not on the same machine as this
    client, the user will need to specifically allow the connection.
    Connections from the local machine are assumed to be safe.
    """

    def __init__(self, name="rvCommunicator-1", noPP=True):
        """
        "name" should be unique among all clients of the network
        protocol.
        noPP will disable ping-pong "heartbeat" messages.
        """
        self.defaultPort = 45124
        self.port = self.defaultPort
        self.connected = False
        self.sock = 0
        self.name = name
        self.handlers = {}
        self.eventQueue = []
        self.noPingPong = noPP

    def __del__(self):
        if self.connected:
            self.disconnect()

    def connect(self, host, port=-1):
        """
        Connect to the specified host/port, exchange greetings with
        RV, turn off heartbeat if so desired.
        """
        if self.connected:
            self.disconnect()

        if port != -1:
            self.port = port

        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        except socket.error as msg:
            print("ERROR: can't create socket: %s\n" % msg[1], file=sys.stderr)
            return

        try:
            self.sock.connect((host, self.port))
        except socket.error as msg:
            print("ERROR: can't connect: %s\n" % msg[1], file=sys.stderr)
            return

        try:
            greeting = "%s rvController" % self.name
            msg = "NEWGREETING %d %s" % (len(greeting), greeting)
            self.sock.sendall(six.ensure_binary(msg))
            if self.noPingPong:
                self.sock.sendall(six.ensure_binary("PINGPONGCONTROL 1 0"))
        except socket.error as msg:
            print("ERROR: can't send greeting: %s\n" % msg[1], file=sys.stderr)
            return

        self.sock.setblocking(0)
        self.connected = True

        self.processEvents()

    def disconnect(self, send_msg=True):
        """
        Disconnect from remote RV.
        """
        try:
            if send_msg:
                self._sendMessage("DISCONNECT")
            self.sock.shutdown(socket.SHUT_RDWR)
            self.sock.close()
        except:
            pass
        self.connected = False

    def _sendMessage(self, message):
        """
        For internal use.  Send and arbitrary message.
        """
        msg = "MESSAGE %d %s" % (len(message), message)
        self.sock.sendall(six.ensure_binary(msg))

    def sendEvent(self, eventName, eventContents="Default"):
        """
        Send a remote event.  eventName must be one of the events
        listed in the RV Reference Manual.
        """
        message = "EVENT %s * %s" % (eventName, eventContents)
        self._sendMessage(message)

    def sendEventAndReturn(self, eventName, eventContents="Default"):
        """
        Send a remote event, then wait for a return value (string).
        eventName must be one of the events
        listed in the RV Reference Manual.
        """
        message = "RETURNEVENT %s * %s" % (eventName, eventContents)
        self._sendMessage(message)
        return self._processEvents(True)

    def remoteEval(self, code):
        """
        Special case of sendEvent, remote-eval is the most common remote event.
        """
        self.sendEvent("remote-eval", code)

    def remoteEvalAndReturn(self, code):
        """
        Special case of sendEventAndReturn, remote-eval is the most common remote event.
        """

        return self.sendEventAndReturn("remote-eval", code)

    def remotePyEval(self, code):
        """
        Evaluate python expression returning result.
        """
        self.sendEvent("remote-pyeval", code)

    def remotePyExec(self, code):
        """
        Execute python command (no return value).
        """
        self.sendEvent("remote-pyexec", code)

    def messageAvailable(self):
        """
        Return true iff there is an incomming message waiting.
        """
        available = False
        try:
            data = self.sock.recv(1, socket.MSG_PEEK)
            if len(data) != 0:
                available = True
            else:
                print("ERROR: remote host closed connection\n", file=sys.stderr)
                self.sock.close()
                self.connected = False

        except socket.error as msg:
            sanitized_msg = msg
            if hasattr(msg, "errno"):
                sanitized_msg = (msg.errno, msg.strerror)
            if (
                sanitized_msg[1] != "Resource temporarily unavailable"
                and sanitized_msg[1]
                != "A non-blocking socket operation could not be completed immediately"
                and sanitized_msg[0] != 10035
            ):
                print("ERROR: peek for messages failed: %s\n" % msg, file=sys.stderr)

        return available

    def _receiveMessageField(self):
        field = six.ensure_binary("")
        while True:
            c = self.sock.recv(1)
            if c == six.ensure_binary(" "):
                break
            field += c

        return field

    def _receiveSingleMessage(self):

        messType = 0
        messContents = 0

        try:
            messType = self._receiveMessageField()
            messSize = int(self._receiveMessageField())

            self.sock.setblocking(1)
            messContents = self.sock.recv(messSize)
            self.sock.setblocking(0)

        except socket.error as msg:
            print("ERROR: can't process message: %s\n" % msg[1], file=sys.stderr)
            self.sock.setblocking(0)

        return (messType, messContents)

    def bindToEvent(self, eventName, eventHandler):
        """
        Bind to a remote event.  That is provide a python function
        (that takes a single string argument), which will be called
        whenever the remote event occurs.  The event contents will
        be provided to the eventHandler as a string.

        It's probably better if the eventHandler does not itself
        send events, but just sets state for later action.
        """
        eventHandlerString = str(eventHandler).split()[1]
        remoteHandlerName = "remoteHandler%s_%s" % (
            eventHandlerString,
            "_".join(eventName.split("-")),
        )
        remoteCode = """
        require commands;

        function: %s (void; Event event)
        {
            string contact = nil;
            for_each (c; commands.remoteConnections())
            {
                if (regex("%s@").match(c)) contact = c;
            }

            if (contact neq nil)
            {
                commands.remoteSendEvent ("%s", "*",
                        event.contents(), string[] {contact});
            }
            event.reject();
        }
        commands.bind("default", "global", "%s", %s, "python event handler");
        true;

        """ % (
            remoteHandlerName,
            self.name,
            eventName,
            eventName,
            remoteHandlerName,
        )

        if self.remoteEvalAndReturn(remoteCode) == "true":
            self.handlers[eventName] = eventHandler

    def _processSingleMessage(self, contents):
        parts = contents.split()
        messType = parts[0]

        if messType == six.ensure_binary("RETURN"):
            contents = six.ensure_binary("")
            if len(parts) > 1:
                contents = six.ensure_binary(" ").join(parts[1:])
            return (six.ensure_binary("RETURN"), contents)

        elif messType == six.ensure_binary("EVENT"):
            eventName = parts[1]
            contents = six.ensure_binary("")
            if len(parts) > 3:
                contents = six.ensure_binary(" ").join(parts[3:])
            return (eventName, contents)

    def processEvents(self):
        self._processEvents()

    def _processEvents(self, processReturnOnly=False):

        while 1:
            noMessage = True
            while noMessage:
                if not self.connected:
                    return six.ensure_binary("")
                noMessage = not self.messageAvailable()
                if noMessage and processReturnOnly:
                    time.sleep(0.01)
                else:
                    break

            if noMessage:
                break

            (messType, messContents) = self._receiveSingleMessage()

            if messType == six.ensure_binary("MESSAGE"):
                (event, contents) = self._processSingleMessage(messContents)

                if event == six.ensure_binary("RETURN"):
                    if processReturnOnly:
                        return contents
                    else:
                        print(
                            "ERROR: out of order return: %s\n" % contents,
                            file=sys.stderr,
                        )
                        return six.ensure_binary("")
                elif (
                    len(self.eventQueue) == 0
                    or (event, contents) != self.eventQueue[-1:]
                ):
                    self.eventQueue.append((event, contents))

            elif messType == six.ensure_binary("PING"):
                self.sock.sendall(six.ensure_binary("PONG 1 p"))

            elif (
                messType == six.ensure_binary("GREETING")
                or messType == six.ensure_binary("NEWGREETING")
                or messType == six.ensure_binary("PONG")
            ):
                #   ignore
                pass
            else:
                print("ERROR: unknown message type: %s\n" % messType, file=sys.stderr)

        for event, contents in self.eventQueue:
            if event in self.handlers:
                self.handlers[event](contents)

        self.eventQueue = []

        return six.ensure_binary("")
