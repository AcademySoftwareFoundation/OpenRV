# Metal 10-bit CPU fallback — implementation notes

`syncBuffers()` in `QTMetalVideoDevice` has three present routes:

1. **GPU fast path** — zero-copy GL → IOSurface blit (default)
2. **New CPU fallback** — GPU-blit into a flip FBO, then packed `glReadPixels`
3. ~~Old CPU fallback~~ — float readback + per-pixel CPU pack loop (removed)

---

## GPU fast path (zero-copy)

```
RGBA16F render FBO
       │
       │  glBlitFramebufferEXT (GPU: float→RGB10_A2, Y-flip)
       ▼
IOSurface-backed GL_RGB10_A2 FBO  ← backed by IOSurface memory directly
       │
       │  glColorMask(alpha) + glClear → force A=3 (opaque)
       │  glFlush()  (no stall — CA reads the IOSurface asynchronously)
       ▼
CALayer compositor  (reads IOSurface, no CPU copy)
```

Nothing ever touches the CPU. The blit quantises float to 10-bit and
Y-flips in one GPU pass. `glFlush()` (not `glFinish()`) is enough because
IOSurface provides cross-context synchronisation — the compositor waits on
the GPU fence itself.

---

## Old CPU fallback (removed)

```
RGBA16F render FBO
       │
       │  glFinish()  ← GPU fully idle, main thread stalls
       │  glReadPixels(GL_RGBA, GL_FLOAT)  ← 16 MB/frame @ 1080p
       ▼
CPU float buffer  [w × h × 4 × 4 bytes]
       │
       │  nested for(y) for(x):
       │    clamp(r,g,b to [0,1])
       │    × 1023.0f + 0.5f → uint32 & 0x3FF
       │    bit-pack: (3u<<30)|(ri<<20)|(gi<<10)|bi
       │    explicit Y-flip via (h-1-y) row index
       ▼
CPU uint32 buffer  [w × h × 4 bytes]
       │
       │  presentPixelData() → memcpy into IOSurface
       ▼
CALayer compositor
```

**Cost:** ~8 MB float readback + ~8 M multiply/clamp/pack ops per frame at
1080p; ~128 MB readback + ~130 M ops at 4K. Fully serialised on the main
thread.

---

## New CPU fallback (current)

```
RGBA16F render FBO
       │
       │  glBlitFramebufferEXT (GPU: float→RGB10_A2, Y-flip)  ← same GPU op as fast path
       ▼
Persistent GL_RGB10_A2 flip FBO  (not IOSurface-backed)
       │
       │  glColorMask(alpha only) + glClear → force A=3 (opaque)
       │  glFinish()  ← stalls, but only after GPU blit is done
       │  glReadPixels(GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV)
       │    ← pixels are already packed; no CPU conversion
       ▼
CPU uint32 buffer  [w × h × 4 bytes]
       │
       │  presentPixelData() → memcpy into IOSurface
       ▼
CALayer compositor
```

**Cost:** ~8 MB readback (same as before), but **0** multiply/clamp/pack
ops — the GPU packed the pixels during the blit. The main-thread stall is
reduced to the GPU-drain time only.

---

## Side-by-side comparison

| | GPU fast path | Old CPU fallback | New CPU fallback |
| --- | --- | --- | --- |
| Float readback | None | Yes — 4× larger buffer | None |
| CPU pack loop | None | Yes — ~25 M ops @ 4K | None |
| Y-flip | GPU (blit dest coords) | CPU (row index reversal) | GPU (blit dest coords) |
| `glFinish()` | No — `glFlush()` suffices | Yes | Yes (GL-on-Metal requirement) |
| Alpha fix | `glColorMask` + `glClear` | Hardcoded `(3u<<30)` | `glColorMask` + `glClear` |
| `glReadPixels` format | N/A | `GL_RGBA, GL_FLOAT` | `GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV` |
| IOSurface backing | Direct (zero-copy) | Via `memcpy` | Via `memcpy` |
| Main-thread stall | None | GPU drain + CPU work | GPU drain only |

---

## Why `GL_BGRA` and not `GL_RGBA`

`GL_UNSIGNED_INT_2_10_10_10_REV` packs the **first listed component** into
the **least significant bits**:

- `GL_RGBA` → R→bits[9:0], G→bits[19:10], B→bits[29:20], A→bits[31:30]
  = **A2B10G10R10** (Vulkan swapchain layout)
- `GL_BGRA` → B→bits[9:0], G→bits[19:10], R→bits[29:20], A→bits[31:30]
  = **A2R10G10B10** = `ARGB2101010LE` (IOSurface layout ✓)

The Vulkan path (`QTVulkanVideoDevice`) uses `GL_RGBA` because its
swapchain format is `VK_FORMAT_A2B10G10R10_UNORM_PACK32`. The Metal path
uses `GL_BGRA` because IOSurface wants
`kCVPixelFormatType_ARGB2101010LEPacked` — R and B are swapped between the
two formats.

---

## Why `glFinish()` is still needed in the new fallback

The fast path skips it because the CA compositor reads the IOSurface via a
GPU fence and never touches pixels on the CPU. The new fallback still calls
`glReadPixels`, which reads to CPU memory. On macOS's GL-on-Metal
translation layer, `glReadPixels` does **not** automatically wait for the
Metal command buffer that encoded the preceding blit to complete.
`glFinish()` forces `[MTLCommandBuffer waitUntilCompleted]` through the
GL→Metal bridge, ensuring the blit has landed before the readback samples
the flip FBO.

---

## Environment variables

| Variable | Effect |
| --- | --- |
| `RV_METAL_FORCE_CPU_PRESENT` | Force the CPU fallback on every frame regardless of IOSurface interop availability. Useful for testing the fallback path. |

---

## When does the CPU fallback trigger?

`ensureIOSurfaceTextures()` returns `false` when:

- `CGLTexImageIOSurface2D` fails (incompatible format, driver issue)
- The IOSurface FBO is incomplete
- The GL blit into the IOSurface FBO returns a GL error

On failure, `latchInteropFailure()` disables interop and sets a
`kInteropRetryFrames` (120 frame) cooldown before the next attempt. A
one-shot log fires: `"presenting via CPU readback (WxH); zero-copy
IOSurface interop unavailable — retrying periodically."` Recovery is
logged when the fast path resumes.
