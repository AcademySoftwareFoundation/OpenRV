import hashlib
import logging
import math
import os
import shutil
import subprocess
import tempfile
import textwrap
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from typing import Any

try:
    from PySide2 import QtCore
except ImportError:
    from PySide6 import QtCore

from rv import commands, rvtypes

logger = logging.getLogger(__name__)

the_mode = None

FRAME_WIDTH = 240
MAX_FILMSTRIP_FRAMES = 25
MAX_WORKERS = 4
UI_REFRESH_DELAY_MS = 100


class _SignalBridge(QtCore.QObject):
    """Bridges background threads to the main thread via Qt signals."""

    finished = QtCore.Signal(str, str, str)  # cache_key, path_key, output_path ("" if failed)


class LocalThumbnailGen(rvtypes.MinorMode):
    """
    Generates filmstrip and thumbnail previews locally via rvio for any media
    source that does not have their own custom thumbnail or filmstrip.

    Sync cache lookup events return cached paths and trigger generation if needed.
    Generation runs in background threads. When threads complete, a coalescing
    timer triggers a UI refresh so the session manager picks up the new paths.
    """

    def __init__(self) -> None:
        rvtypes.MinorMode.__init__(self)
        self._cache: dict[str, dict[str, Path | None]] = {}
        self._cache_dir = Path(tempfile.gettempdir()) / f"rv_thumbnails_{os.getpid()}"
        self._cache_dir.mkdir(parents=True, exist_ok=True)
        self._in_flight: set[str] = set()
        self._pool = ThreadPoolExecutor(max_workers=MAX_WORKERS)

        self._bridge = _SignalBridge()
        self._bridge.finished.connect(self._on_generation_done, QtCore.Qt.QueuedConnection)

        self._refresh_timer = QtCore.QTimer()
        self._refresh_timer.setSingleShot(True)
        self._refresh_timer.setInterval(UI_REFRESH_DELAY_MS)
        self._refresh_timer.timeout.connect(self._trigger_ui_refresh)

        # The last parameter is the priority of the plugin. Having it at 10 means it will be run last
        # letting custom plugins of higher priority run first and consume the event before the local plugin runs.
        self.init("LocalThumbnailGen", self.global_bindings(), None, None, None, 10)

    def global_bindings(self) -> list[tuple[str, Any, str]]:
        return [
            (
                "session-manager-get-filmstrip-path",
                self._on_get_filmstrip_path,
                "Return cached local filmstrip path, start generation if needed",
            ),
            (
                "session-manager-get-thumbnail-path",
                self._on_get_thumbnail_path,
                "Return cached local thumbnail path, start generation if needed",
            ),
            (
                "before-session-deletion",
                self._on_session_deletion,
                "Delete all cached local filmstrips and thumbnails on RV close",
            ),
        ]

    def _get_cached_path(self, event: Any, path_key: str) -> None:
        event.reject()
        source_node = event.contents()
        media_path = self._get_media_path(source_node)
        if not media_path:
            event.setReturnContent("")
            return

        cache_key = self._cache_key(media_path)
        cached = self._cache.get(cache_key, {})
        path = cached.get(path_key)

        if path:
            event.setReturnContent(str(path))
            return

        flight_key = f"{cache_key}_{path_key}"
        if flight_key not in self._in_flight:
            self._start_generation(source_node, cache_key, media_path, path_key)

        event.setReturnContent("")

    def _on_get_filmstrip_path(self, event: Any) -> None:
        self._get_cached_path(event, "filmstrip_path")

    def _on_get_thumbnail_path(self, event: Any) -> None:
        self._get_cached_path(event, "thumbnail_path")

    def _start_generation(self, source_node: str, cache_key: str, media_path: str, path_key: str) -> None:
        rvio_bin = self._get_rvio_bin()
        if not rvio_bin:
            return

        source_info = self._get_source_info(source_node)
        if not source_info:
            return

        self._in_flight.add(f"{cache_key}_{path_key}")

        start_frame, end_frame, width, height = source_info

        if path_key == "thumbnail_path":
            self._pool.submit(
                self._generate_thumbnail,
                cache_key,
                rvio_bin,
                media_path,
                (start_frame + end_frame) // 2,
            )
        else:
            self._pool.submit(
                self._generate_filmstrip,
                cache_key,
                rvio_bin,
                media_path,
                start_frame,
                end_frame,
                width,
                height,
            )

    def _cache_key(self, media_path: str) -> str:
        return hashlib.sha256(media_path.encode()).hexdigest()[:16]

    def _get_rvio_bin(self) -> str | None:
        rvio = os.getenv("RV_APP_RVIO")
        if not rvio:
            logger.warning("RV_APP_RVIO not set")
            return None
        return rvio

    def _get_media_path(self, source_node: str) -> str | None:
        try:
            return commands.getStringProperty(f"{source_node}.media.movie")[0]
        except Exception as e:
            logger.warning(f"Could not get media path: {e}")
            return None

    def _get_source_info(self, source_node: str) -> tuple[int, int, int, int] | None:
        try:
            info = commands.sourceMediaInfo(source_node)
            return (
                info.get("startFrame", 1),
                info.get("endFrame", info.get("startFrame", 1)),
                info.get("width", 1920),
                info.get("height", 1080),
            )
        except Exception as e:
            logger.warning(f"Could not get source info: {e}")
            return None

    def _pick_frames(self, start_frame: int, end_frame: int) -> list[int]:
        n_frames = end_frame - start_frame + 1
        if n_frames < MAX_FILMSTRIP_FRAMES:
            return list(range(start_frame, end_frame + 1))
        incr = (n_frames - 1) / (MAX_FILMSTRIP_FRAMES - 1.0)
        return [int(math.floor(start_frame + i * incr)) for i in range(MAX_FILMSTRIP_FRAMES)]

    def _write_filmstrip_session(
        self, session_path: Path, media_path: str, frames: list[int], width: int, height: int
    ) -> tuple[int, int]:
        n = len(frames)
        output_width = FRAME_WIDTH * n
        aspect_ratio = float(height) / (n * width) if (n * width) > 0 else 1.0
        output_height = int(math.floor(output_width * aspect_ratio))

        source_group_names = [f'"sourceGroup{frame:06d}"' for frame in frames]
        lhs = " ".join(source_group_names)
        rhs = " ".join('"defaultLayout"' for _ in frames)
        top_nodes = f'"defaultLayout" {lhs}'

        with open(session_path, "w") as f:
            f.write(
                textwrap.dedent(f"""\
                GTOa (3)

                rv : RVSession (2)
                {{
                    session
                    {{
                        string viewNode = "defaultLayout"
                        int marks = [ ]
                        int[2] range = [ [ 1 4 ] ]
                        int[2] region = [ [ 1 4 ] ]
                        float fps = 24
                        int realtime = 0
                        int inc = 1
                        int currentFrame = 1
                        int version = 1
                        int background = 0
                    }}
                }}

                connections : connection (1)
                {{
                    evaluation
                    {{
                        string lhs = [ {lhs} ]
                        string rhs = [ {rhs} ]
                    }}

                    top
                    {{
                        string nodes = [ {top_nodes} ]
                    }}
                }}

                defaultLayout : RVLayoutGroup (1)
                {{
                    ui
                    {{
                        string name = "Default Layout"
                    }}

                    layout
                    {{
                        string mode = "row"
                    }}

                    timing
                    {{
                        int retimeInputs = 1
                    }}

                    session
                    {{
                        float fps = 24
                        int marks = [ ]
                        int frame = 1
                    }}
                }}

                defaultLayout_stack : RVStack (1)
                {{
                    output
                    {{
                        float fps = 24
                        int size = [ {output_width} {output_height} ]
                        int autoSize = 0
                        string chosenAudioInput = ".all."
                    }}

                    mode
                    {{
                        int useCutInfo = 1
                        int alignStartFrames = 0
                        int strictFrameRanges = 0
                    }}

                    composite
                    {{
                        string type = "over"
                    }}
                }}

            """)
            )

            for frame in frames:
                f.write(
                    textwrap.dedent(f"""\
                    sourceGroup{frame:06d} : RVSourceGroup (1)
                    {{
                        ui
                        {{
                            string name = "{frame}"
                        }}
                    }}

                    sourceGroup{frame:06d}_source : RVFileSource (1)
                    {{
                        media
                        {{
                            string movie = "{media_path}"
                        }}

                        group
                        {{
                            float fps = 24
                            float volume = 1
                            float audioOffset = 0
                            int rangeOffset = 0
                            int noMovieAudio = 0
                            float balance = 0
                            float crossover = 0
                        }}

                        cut
                        {{
                            int in = {frame}
                            int out = {frame}
                        }}
                    }}

                """)
                )

        return output_width, output_height

    def _generate_thumbnail(self, cache_key: str, rvio_bin: str, media_path: str, mid_frame: int) -> None:
        """Runs rvio to generate a single-frame thumbnail in a worker thread."""
        output_path = self._cache_dir / f"{cache_key}_thumbnail.jpg"
        try:
            subprocess.run(
                [rvio_bin, media_path, "-t", str(mid_frame), "-o", str(output_path)],
                capture_output=True,
                timeout=120,
            )
        except Exception as e:
            logger.error(f"Thumbnail generation failed: {e}")

        self._bridge.finished.emit(
            cache_key,
            "thumbnail_path",
            str(output_path) if output_path.exists() else "",
        )

    def _generate_filmstrip(
        self,
        cache_key: str,
        rvio_bin: str,
        media_path: str,
        start_frame: int,
        end_frame: int,
        width: int,
        height: int,
    ) -> None:
        """Runs rvio to generate a filmstrip image in a worker thread."""
        output_path = self._cache_dir / f"{cache_key}_filmstrip.jpg"
        session_path = self._cache_dir / f"filmstrip_{cache_key}.rv"
        try:
            output_width, output_height = self._write_filmstrip_session(
                session_path, media_path, self._pick_frames(start_frame, end_frame), width, height
            )
            subprocess.run(
                [
                    rvio_bin,
                    str(session_path),
                    "-resize",
                    str(FRAME_WIDTH),
                    str(output_height),
                    "-outres",
                    str(output_width),
                    str(output_height),
                    "-o",
                    str(output_path),
                ],
                capture_output=True,
                timeout=120,
            )
        except Exception as e:
            logger.error(f"Filmstrip generation failed: {e}")
        finally:
            try:
                session_path.unlink(missing_ok=True)
            except Exception as e:
                logger.warning(f"Failed to delete session file {session_path}: {e}")

        self._bridge.finished.emit(
            cache_key,
            "filmstrip_path",
            str(output_path) if output_path.exists() else "",
        )

    def _on_generation_done(self, cache_key: str, path_key: str, output_path: str) -> None:
        """Called on the main thread when a single rvio job completes."""
        if output_path:
            self._cache.setdefault(cache_key, {})[path_key] = Path(output_path)

        self._in_flight.discard(f"{cache_key}_{path_key}")
        self._refresh_timer.start()

    def _trigger_ui_refresh(self) -> None:
        """Called by the coalescing timer. Tells the session manager to rebuild."""
        commands.sendInternalEvent("session-manager-preview-available", "")

    def _on_session_deletion(self, event: Any) -> None:
        event.reject()
        self._refresh_timer.stop()
        self._pool.shutdown(wait=False, cancel_futures=True)
        self._in_flight.clear()

        if self._cache_dir.exists():
            try:
                shutil.rmtree(self._cache_dir)
            except Exception as e:
                logger.warning(f"Failed to delete cache directory {self._cache_dir}: {e}")
        self._cache.clear()


def createMode() -> LocalThumbnailGen:
    global the_mode
    the_mode = LocalThumbnailGen()
    return the_mode


def theMode() -> LocalThumbnailGen | None:
    return the_mode
