"""Gate 5 — ThumbnailWidget / FilmstripWidget / SourcePreviewWidget on the port.

COVERAGE.md drops the hover and scrub behavior from the golden inventory because it
is pointer-position dependent, which makes these the tests that actually pin it. They
build the real widgets and feed them real images, so the frame arithmetic in
showFrameAtX is checked against pixels rather than described.
"""
from __future__ import annotations

import unittest

import _rv_stubs

SKIP = _rv_stubs.requiresPySide6()

if not SKIP:
    from PySide6 import QtCore, QtGui, QtWidgets
    from PySide6.QtCore import Qt

_app = None


def setUpModule():
    if SKIP:
        raise unittest.SkipTest(SKIP)
    global _app
    _app = QtWidgets.QApplication.instance() or QtWidgets.QApplication([])


def _stripFile(tmpdir, frames, height=45, frameWidth=240, colors=None):
    """A filmstrip PNG: `frames` frames side by side, each a distinct flat color."""
    image = QtGui.QImage(frameWidth * frames, height, QtGui.QImage.Format_RGB32)
    for f in range(frames):
        color = (colors or [])[f] if colors else QtGui.QColor(f * 40 % 256, 0, 0)
        for x in range(f * frameWidth, (f + 1) * frameWidth):
            for y in range(height):
                image.setPixelColor(x, y, color)
    path = str(tmpdir / ("strip_%d.png" % frames))
    image.save(path, "PNG")
    return path


class WidgetTest(unittest.TestCase):
    def setUp(self):
        self.sm, self.graph = _rv_stubs.importPort("session_manager")
        import tempfile
        import pathlib

        self._tmp = tempfile.TemporaryDirectory()
        self.tmpdir = pathlib.Path(self._tmp.name)

    def tearDown(self):
        self._tmp.cleanup()


class TestThumbnailWidget(WidgetTest):
    def test_init_scales_contents(self):
        w = self.sm.ThumbnailWidget(None)
        self.assertTrue(w.hasScaledContents())

    def test_set_fallback_installs_the_pixmap(self):
        w = self.sm.ThumbnailWidget(None)
        pixmap = QtGui.QPixmap(10, 10)
        pixmap.fill(QtGui.QColor("red"))
        w.setFallback(pixmap)
        self.assertFalse(w.pixmap().isNull())

    def test_load_replaces_the_pixmap(self):
        path = _stripFile(self.tmpdir, 1)
        w = self.sm.ThumbnailWidget(None)
        w.load(path)
        self.assertFalse(w.pixmap().isNull())

    def test_load_of_a_missing_file_keeps_the_fallback(self):
        """I9: the fallback must survive a thumbnail that has not been generated."""
        w = self.sm.ThumbnailWidget(None)
        fallback = QtGui.QPixmap(8, 8)
        fallback.fill(QtGui.QColor("blue"))
        w.setFallback(fallback)

        w.load(str(self.tmpdir / "does_not_exist.png"))

        self.assertEqual(w.pixmap().size(), fallback.size())


class TestFilmstripWidget(WidgetTest):
    def test_init_state(self):
        w = self.sm.FilmstripWidget(None)
        self.assertFalse(w.isLoaded())
        self.assertTrue(w.hasScaledContents())
        self.assertTrue(w.hasMouseTracking())
        self.assertEqual(w._frameWidth, self.sm.FILMSTRIP_FRAME_WIDTH)

    def test_load_sets_loaded(self):
        w = self.sm.FilmstripWidget(None)
        w.load(_stripFile(self.tmpdir, 3))
        self.assertTrue(w.isLoaded())

    def test_load_of_a_missing_file_leaves_it_unloaded(self):
        w = self.sm.FilmstripWidget(None)
        w.load(str(self.tmpdir / "nope.png"))
        self.assertFalse(w.isLoaded())

    def test_show_frame_before_load_is_a_noop(self):
        w = self.sm.FilmstripWidget(None)
        w.resize(240, 45)
        w.showFrameAtX(10)
        self.assertTrue(w.pixmap().isNull(),
                        "scrubbing an unloaded filmstrip must not set a pixmap")

    def test_frame_width_is_one_frame_wide(self):
        w = self.sm.FilmstripWidget(None)
        w.load(_stripFile(self.tmpdir, 4))
        w.resize(self.sm.FILMSTRIP_FRAME_WIDTH, 45)
        w.showFrameAtX(0)
        self.assertEqual(w.pixmap().width(), self.sm.FILMSTRIP_FRAME_WIDTH)

    def test_left_edge_selects_the_first_frame(self):
        colors = [QtGui.QColor("red"), QtGui.QColor("green"),
                  QtGui.QColor("blue"), QtGui.QColor("white")]
        w = self.sm.FilmstripWidget(None)
        w.load(_stripFile(self.tmpdir, 4, colors=colors))
        w.resize(400, 45)

        w.showFrameAtX(0)
        got = w.pixmap().toImage().pixelColor(5, 5)
        self.assertEqual(got.red(), 255)
        self.assertEqual(got.green(), 0)

    def test_beyond_the_right_edge_clamps_to_the_last_frame(self):
        colors = [QtGui.QColor("red"), QtGui.QColor("green"),
                  QtGui.QColor("blue"), QtGui.QColor("white")]
        w = self.sm.FilmstripWidget(None)
        w.load(_stripFile(self.tmpdir, 4, colors=colors))
        w.resize(400, 45)

        w.showFrameAtX(100000)
        got = w.pixmap().toImage().pixelColor(5, 5)
        self.assertEqual((got.red(), got.green(), got.blue()), (255, 255, 255),
                         "past the end must clamp to the final frame, not wrap")

    def test_negative_x_clamps_to_the_first_frame(self):
        colors = [QtGui.QColor("red"), QtGui.QColor("green")]
        w = self.sm.FilmstripWidget(None)
        w.load(_stripFile(self.tmpdir, 2, colors=colors))
        w.resize(400, 45)

        w.showFrameAtX(-500)
        got = w.pixmap().toImage().pixelColor(5, 5)
        self.assertEqual((got.red(), got.green()), (255, 0))

    def test_mouse_move_scrubs_to_the_event_position(self):
        colors = [QtGui.QColor("red"), QtGui.QColor("white")]
        w = self.sm.FilmstripWidget(None)
        w.load(_stripFile(self.tmpdir, 2, colors=colors))
        w.resize(400, 45)

        event = QtGui.QMouseEvent(
            QtCore.QEvent.MouseMove,
            QtCore.QPointF(399, 10),
            QtCore.QPointF(399, 10),
            Qt.NoButton, Qt.NoButton, Qt.NoModifier,
        )
        w.mouseMoveEvent(event)

        got = w.pixmap().toImage().pixelColor(5, 5)
        self.assertEqual((got.red(), got.green(), got.blue()), (255, 255, 255))


class TestSourcePreviewWidget(WidgetTest):
    def test_init_shows_thumbnail_and_hides_filmstrip(self):
        w = self.sm.SourcePreviewWidget(None)
        self.assertFalse(w._thumbnail.isHidden())
        self.assertTrue(w._filmstrip.isHidden())

    def test_hover_attribute_is_set(self):
        w = self.sm.SourcePreviewWidget(None)
        self.assertTrue(w.testAttribute(Qt.WA_Hover))

    def test_hover_enter_without_a_strip_keeps_the_thumbnail(self):
        w = self.sm.SourcePreviewWidget(None)
        w.event(QtCore.QEvent(QtCore.QEvent.HoverEnter))
        self.assertFalse(w._thumbnail.isHidden())
        self.assertTrue(w._filmstrip.isHidden())

    def test_hover_enter_with_a_strip_swaps_to_the_filmstrip(self):
        w = self.sm.SourcePreviewWidget(None)
        w.show()
        w.loadStrip(_stripFile(self.tmpdir, 3))

        w.event(QtCore.QEvent(QtCore.QEvent.HoverEnter))

        self.assertFalse(w._filmstrip.isHidden())
        self.assertTrue(w._thumbnail.isHidden())

    def test_hover_leave_swaps_back(self):
        w = self.sm.SourcePreviewWidget(None)
        w.show()
        w.loadStrip(_stripFile(self.tmpdir, 3))
        w.event(QtCore.QEvent(QtCore.QEvent.HoverEnter))

        w.event(QtCore.QEvent(QtCore.QEvent.HoverLeave))

        self.assertTrue(w._filmstrip.isHidden())
        self.assertFalse(w._thumbnail.isHidden())

    def test_hover_events_are_handled_by_the_override_not_the_base_class(self):
        """QWidget.event() also returns True for hover, so returning True proves
        nothing on its own. What distinguishes the override is the side effect: it
        swaps the two child widgets. Checked here by confirming an unrelated event
        type leaves them alone while a hover does not.
        """
        w = self.sm.SourcePreviewWidget(None)
        w.show()
        w.loadStrip(_stripFile(self.tmpdir, 3))

        w.event(QtCore.QEvent(QtCore.QEvent.None_))
        self.assertTrue(w._filmstrip.isHidden(), "a non-hover event must not swap")

        w.event(QtCore.QEvent(QtCore.QEvent.HoverEnter))
        self.assertFalse(w._filmstrip.isHidden(), "HoverEnter must reach the override")

    def test_set_fallback_reaches_the_thumbnail(self):
        w = self.sm.SourcePreviewWidget(None)
        pixmap = QtGui.QPixmap(6, 6)
        pixmap.fill(QtGui.QColor("red"))
        w.setFallback(pixmap)
        self.assertFalse(w._thumbnail.pixmap().isNull())

    def test_load_thumbnail_reaches_the_thumbnail(self):
        w = self.sm.SourcePreviewWidget(None)
        w.loadThumbnail(_stripFile(self.tmpdir, 1))
        self.assertFalse(w._thumbnail.pixmap().isNull())


if __name__ == "__main__":
    unittest.main()
