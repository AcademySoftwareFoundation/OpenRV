# Copyright (c) 2021 Autodesk.
#
# CONFIDENTIAL AND PROPRIETARY
#
# This work is provided "AS IS" and subject to the ShotGrid Pipeline Toolkit
# Source Code License included in this distribution package. See LICENSE.
# By accessing, using, copying or modifying this work you indicate your
# agreement to the ShotGrid Pipeline Toolkit Source Code License. All rights
# not expressly granted therein are reserved by ShotGrid Software Inc.
"""RV Node Graph Viz."""

try:
    import matplotlib
    import networkx as nx

    matplotlib.use("Qt5Agg")
    import matplotlib.pyplot as plt
except:
    print("Exception occurred while importing and initializing matplotlib.")
    print(
        "'py-interp -m pip install matplotlib networkx' to install it into RV's environment."
    )
    raise

import itertools

import rv.commands as rvc
import rv.rvtypes


class RVNodeGraphViz(rv.rvtypes.MinorMode):
    def __init__(self):
        """Add Node Graph Viz to the RV toolbar."""
        rv.rvtypes.MinorMode.__init__(self)
        self.init(
            "rv_node_graph_viz",
            [
                (
                    "ngv-make-graph",
                    self.make_graph,
                    "Inspect Node Graph",
                ),
            ],
            None,
            [
                (
                    "Node Graph Viz",
                    [
                        ("Inspect Node Graph", self.make_graph, None, None),
                    ],
                )
            ],
        )

    def make_graph(self, _):
        """Create the graph and then draw it."""
        start_node = rvc.viewNode()
        self.dag = nx.DiGraph()

        def walk_graph(node, dag, visted=None):
            nodes = visted or []
            if node in nodes:
                return
            nodes.append(node)

            inputs, outputs = rvc.nodeConnections(node)
            for i, o in itertools.zip_longest(inputs, outputs):
                if o:
                    dag.add_edge(node, o)
                    walk_graph(o, dag, nodes)
                if i:
                    dag.add_edge(i, node)
                    walk_graph(i, dag, nodes)

        walk_graph(start_node, self.dag)
        self._show_graph()

    def _show_graph(self):
        """Draw and display the graph."""
        nx.draw_networkx(
            self.dag, pos=nx.spring_layout(self.dag), with_labels=True, font_size=6
        )
        plt.axis("off")
        plt.show()


g_the_mode = None


def createMode():
    global g_the_mode
    g_the_mode = RVNodeGraphViz()
    return g_the_mode


def theMode():
    global g_the_mode
    return g_the_mode
