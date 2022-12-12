#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
#! /usr/bin/python
from __future__ import print_function

import socket
import sys
import time
import os

doDebug = False
if os.getenv("RV_NUKE_DEBUG") != None:
    doDebug = True


def log(str):
    if doDebug:
        print("net: %s\n" % str, file=sys.stderr)


class RvCommunicator:
    """
    Wrap up connection and communciation with a running RV.  The
    target RV process must have networking turned on and be
    listening on some well-known port (you can make both these
    happen from the command line: "-network -networkPort 45129".
    But default, RV will listen on port 45124.

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

    def connect(self, host, port=-1):
        """
        Connect to the specified host/port, exchange greetings with
        RV, turn off heartbeat if so desired.
        """
        if self.connected:
            self.close()

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
            self.sock.sendall("NEWGREETING %d %s" % (len(greeting), greeting))
            if self.noPingPong:
                self.sock.sendall("PINGPONGCONTROL 1 0")
        except socket.error as msg:
            print("ERROR: can't send greeting: %s\n" % msg[1], file=sys.stderr)
            return

        self.sock.setblocking(0)
        self.connected = True

        self.processEvents()

    def disconnect(self):
        """
        Disconnect from remote RV.
        """
        self._sendMessage("DISCONNECT")
        self.sock.shutdown(socket.SHUT_RDWR)
        self.sock.close()
        self.connected = False

    def _sendMessage(self, message):
        """
        For internal use.  Send and arbitrary message.
        """
        self.sock.sendall("MESSAGE %d %s" % (len(message), message))

    def sendEvent(self, eventName, eventContents):
        """
        Send a remote event.  eventName must be one of the events
        listed in the RV Reference Manual.
        """
        message = "EVENT %s * %s" % (eventName, eventContents)
        self._sendMessage(message)

    def sendEventAndReturn(self, eventName, eventContents):
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
                print("ERROR: remote host closed connection (1)\n", file=sys.stderr)
                self.sock.close()
                self.connected = False

        except socket.error as msg:
            if (
                msg[1] != "Resource temporarily unavailable"
                and msg[1]
                != "A non-blocking socket operation could not be completed immediately"
                and msg[0] != 10035
            ):
                print("ERROR: peek for messages failed: %s\n" % msg, file=sys.stderr)
            if msg[0] == 10054 or msg[1] == "Connection reset by peer":
                print("ERROR: remote host closed connection (2)\n", file=sys.stderr)
                self.sock.close()
                self.connected = False

        return available

    def _receiveMessageField(self):
        field = ""
        while True:
            c = self.sock.recv(1)
            if c == " ":
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

        if messType == "RETURN":
            contents = ""
            if len(parts) > 1:
                contents = " ".join(parts[1:])
            return ("RETURN", contents)

        elif messType == "EVENT":
            eventName = parts[1]
            contents = ""
            if len(parts) > 3:
                contents = " ".join(parts[3:])
            return (eventName, contents)

    def processEvents(self):
        self._processEvents()

    def _processEvents(self, processReturnOnly=False):

        while 1:
            noMessage = True
            while noMessage:
                if not self.connected:
                    return ""
                noMessage = not self.messageAvailable()
                if noMessage and processReturnOnly:
                    time.sleep(0.01)
                else:
                    break

            if noMessage:
                break

            (messType, messContents) = self._receiveSingleMessage()

            log("message: %s %s" % (messType, messContents))
            if messType == "MESSAGE":
                if messContents == "DISCONNECT":
                    try:
                        self.sock.shutdown(socket.SHUT_RDWR)
                        self.sock.close()
                    except:
                        pass
                    self.connected = False
                    return ""

                (event, contents) = self._processSingleMessage(messContents)

                if event == "RETURN":
                    if processReturnOnly:
                        return contents
                    else:
                        print(
                            "ERROR: out of order return: %s\n" % contents,
                            file=sys.stderr,
                        )
                        return ""
                elif (
                    len(self.eventQueue) == 0
                    or (event, contents) != self.eventQueue[-1:]
                ):
                    self.eventQueue.append((event, contents))

            elif messType == "PING":
                self.sock.sendall("PONG 1 p")

            elif (
                messType == "GREETING"
                or messType == "NEWGREETING"
                or messType == "PONG"
            ):
                #   ignore
                pass
            else:
                print("ERROR: unknown message type: %s\n" % messType, file=sys.stderr)

        for (event, contents) in self.eventQueue:
            if event in self.handlers:
                self.handlers[event](contents)

        self.eventQueue = []

        return ""
