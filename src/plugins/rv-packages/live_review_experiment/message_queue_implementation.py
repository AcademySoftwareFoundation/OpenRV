from rv import qtutils
from rv.commands import (
    defineModeMenu,
    NeutralMenuState,
    DisabledMenuState,
    theTime,
    sendInternalEvent,
)
from rv.rvtypes import MinorMode

from PySide2 import QtCore, QtWidgets

import pika

import os
import platform
import ssl


class MessageQueueImplementation(MinorMode, QtCore.QObject):
    def __init__(self):
        MinorMode.__init__(self)
        QtCore.QObject.__init__(self, qtutils.sessionWindow())

        self.__pika_credentials = os.environ.get("RV_AMQP_CREDENTIALS", "")
        self.__pika_current_exchange = os.environ.get("RV_AMQP_DEFAULT_EXCHANGE", "")
        self.__pika_connection = None
        self.__pika_channel = None
        self.__pika_consumer_tag = None
        self.__pika_current_queue = None
        self.__pika_timerEvent = None

        self.__uuid = theTime()

        self.init(
            self.menu_name.replace(" ", ""),
            self.global_bindings,
            self.local_bindings,
            self.menu,
        )

    def timerEvent(self, event):
        self.process_next_message()

    #
    # MinorMode properties
    #

    @property
    def menu_name(self):
        return "ASWF Live Review"

    @property
    def global_bindings(self):
        return [
            (
                "sync-review-change",
                self.send_payload_to_queue,
                "Turn the playack settings into a sync review payload",
            )
        ]

    @property
    def local_bindings(self):
        return None

    @property
    def menu(self):
        if not self.amqp_connected:
            return [
                (
                    self.menu_name,
                    [
                        (
                            "Join Review Server",
                            self.join_review_server,
                            None,
                            lambda: NeutralMenuState,
                        )
                    ],
                )
            ]
        elif not self.in_session:
            return [
                (
                    self.menu_name,
                    [
                        (
                            "Create Review",
                            self.create_review,
                            None,
                            lambda: NeutralMenuState,
                        ),
                        (
                            "Join Review",
                            self.join_review,
                            None,
                            lambda: NeutralMenuState,
                        ),
                        (
                            "Leave Review Server",
                            self.leave_review_server,
                            None,
                            lambda: NeutralMenuState,
                        ),
                    ],
                )
            ]
        else:
            return [
                (
                    self.menu_name,
                    [
                        (
                            f"Leave {self.mq_exchange} ({self.mq_queue})",
                            self.leave_review,
                            None,
                            lambda: NeutralMenuState,
                        )
                    ],
                )
            ]

    #
    # Bindings callbacks
    #

    def send_payload_to_queue(self, event):
        if self.in_session:
            self.send_message(event.contents())

    #
    # Menu callbacks
    #

    def join_review_server(self, event=None):
        sw = qtutils.sessionWindow()
        creds, ok = QtWidgets.QInputDialog.getText(
            sw,
            "Joining Review Server",
            "Review Server Credentials",
            text=self.mq_credentials,
        )

        if ok:
            self.mq_credentials = creds
            self.connect_mq()

    def create_review(self, event=None):
        sw = qtutils.sessionWindow()
        id, ok = QtWidgets.QInputDialog.getText(
            sw,
            "Creating a Review Session",
            "Review Session ID:",
            text=self.mq_exchange,
        )

        if ok:
            self.mq_exchange = id
            self.create_channel()
            self.declare_exchange()

    def join_review(self, event=None):
        sw = qtutils.sessionWindow()
        id, ok = QtWidgets.QInputDialog.getText(
            sw,
            "Joining a Review Session",
            "Review Session ID:",
            text=self.mq_exchange,
        )

        if ok:
            self.mq_exchange = id
            self.create_channel()
            self.declare_queue()

    def leave_review(self, event=None):
        self.close_channel()

        self.mq_queue = None
        self.mq_channel = None

    def leave_review_server(self, event=None):
        self.disconnect_mq()

    #
    # Review Session properties
    #

    @property
    def review_uuid(self):
        return f"{os.getlogin()}@{platform.uname().node}/{self.__uuid}"

    @property
    def amqp_connected(self):
        return self.mq_connection != None

    @property
    def in_session(self):
        return bool(self.mq_exchange) and bool(self.mq_queue)

    @property
    def mq_credentials(self):
        return self.__pika_credentials

    @mq_credentials.setter
    def mq_credentials(self, value):
        self.__pika_credentials = value

    @property
    def mq_connection(self):
        return self.__pika_connection

    @mq_connection.setter
    def mq_connection(self, value):
        self.__pika_connection = value
        defineModeMenu(self.menu_name.replace(" ", ""), self.menu, True)

    @property
    def mq_channel(self):
        return self.__pika_channel

    @mq_channel.setter
    def mq_channel(self, value):
        self.__pika_channel = value
        defineModeMenu(self.menu_name.replace(" ", ""), self.menu, True)

    @property
    def mq_exchange(self):
        return self.__pika_current_exchange or ""

    @mq_exchange.setter
    def mq_exchange(self, value):
        self.__pika_current_exchange = value
        defineModeMenu(self.menu_name.replace(" ", ""), self.menu, True)

    @property
    def mq_queue(self):
        return self.__pika_current_queue or ""

    @mq_queue.setter
    def mq_queue(self, value):
        self.__pika_current_queue = value
        defineModeMenu(self.menu_name.replace(" ", ""), self.menu, True)
        sendInternalEvent("sync-review-queue-name-change", value or "")

    @property
    def mq_consumer_tag(self):
        return self.__pika_consumer_tag or ""

    @mq_consumer_tag.setter
    def mq_consumer_tag(self, value):
        self.__pika_consumer_tag = value

    @property
    def mq_timerEvent(self):
        return self.__pika_timerEvent

    @mq_timerEvent.setter
    def mq_timerEvent(self, value):
        self.__pika_timerEvent = value

    #
    # Connection management
    #

    def connect_mq(self):
        print(f"Connecting to {self.mq_credentials}")

        ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
        ssl_context.set_ciphers("ECDHE+AESGCM:!ECDSA")

        url_parameters = pika.URLParameters(self.mq_credentials)

        url_parameters.ssl_options = pika.SSLOptions(context=ssl_context)

        self.mq_connection = pika.BlockingConnection(
            parameters=url_parameters,
        )

    def create_channel(self):
        print("Creating a new channel")
        self.mq_channel = self.mq_connection.channel()

        print("Specifying the QoS for the channel")
        self.mq_channel.basic_qos(prefetch_count=1)

    def disconnect_mq(self):
        self.close_channel()

        print("Disconnecting")
        self.mq_connection.close()
        self.mq_connection = None

        self.mq_queue = None

    #
    # Channel management
    #
    def close_channel(self):
        self.stop_consuming_queue()

        if self.mq_channel:
            print("Closing the channel")

            self.mq_channel.close()
            self.mq_channel = None

            self.mq_queue = None

    #
    # Queue management
    #

    def declare_queue(
        self,
    ):
        queue_name = f"{self.review_uuid}/{self.mq_exchange}"

        print(f"Declaring queue {queue_name}")

        self.mq_channel.queue_declare(
            queue=f"{self.review_uuid}/{self.mq_exchange}",
            durable=False,
            auto_delete=False,
            exclusive=False,
            arguments={"x-expires": 60000},
        )

        print(f"Binding queue {queue_name} to exchange {self.mq_exchange}")

        self.mq_channel.queue_bind(
            queue=queue_name,
            exchange=self.mq_exchange,
        )

        self.mq_queue = queue_name

        self.start_consuming_queue()

    #
    # Exchange management
    #

    def declare_exchange(self):
        print(f"Declaring exchange {self.mq_exchange}")

        self.mq_channel.exchange_declare(
            exchange=self.mq_exchange,
            exchange_type="fanout",
            durable=False,
            auto_delete=True,
            internal=False,
        )

        self.declare_queue()

    #
    # Message management
    #

    def send_message(self, message):
        print(f"Sending message to {self.mq_exchange} (from {self.mq_queue})")

        self.mq_channel.basic_publish(
            exchange=self.mq_exchange,
            routing_key="",
            body=message.encode("utf-8"),
            properties=pika.BasicProperties(
                app_id=self.mq_queue,
                content_type="application/json",
                delivery_mode=1,
            ),
        )

    def start_consuming_queue(self):
        self.stop_consuming_queue()
        print(f"Starting consuming from loop")
        self.mq_timerEvent = self.startTimer(500)

    def stop_consuming_queue(self):
        if self.mq_timerEvent:
            print(f"Stopping consuming loop")
            self.killTimer(self.mq_timerEvent)
            self.mq_timerEvent = None

    def process_next_message(self):
        method_frame, header_frame, body = self.mq_channel.basic_get(
            queue=self.mq_queue
        )
        if None in (method_frame, header_frame, body):
            return

        if header_frame.app_id == self.mq_queue:
            return

        print(f"Received message from {header_frame.app_id}")

        self.mq_channel.basic_ack(delivery_tag=method_frame.delivery_tag)

        sendInternalEvent("sync-review-change-received", body.decode("utf-8"))
        self.process_next_message()


_mode = None


def createMode():
    global _mode
    _mode = MessageQueueImplementation()
    return _mode


def getMode():
    return _mode
