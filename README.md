# UEquirectangularCapture

Unreal Engine runtime plugin for capturing six 90-degree scene views and composing them into an equirectangular render target.

## Features

- `AEquirectangularCaptureActor` for Blueprint / Editor placement
- Six-direction `SceneCaptureComponent2D` capture
- Compute shader based equirectangular composition
- Optional output `TextureRenderTarget2D`
- Internal preview render target fallback
- Editor preview refresh with configurable interval

## Requirements

- Unreal Engine 5.7
- D3D12 / SM6 capable environment

## Installation

Copy this folder into your Unreal project:

```text
YourProject/Plugins/UEquirectangularCapture
```

Then enable the plugin in the project or add it to the `.uproject` plugins list.

## Usage

1. Place `EquirectangularCaptureActor` in the level.
2. Set `Face Resolution`, `Preview Width`, and `Preview Height`.
3. Optionally assign `Output Render Target`.
4. Enable `Auto Capture` for periodic editor/game updates, or call `CaptureNow()` manually.

If `Output Render Target` is not assigned, the plugin writes to the internal `PreviewRenderTarget`.

## Editor Capture

In editor mode, the actor avoids per-frame capture by default behavior:

- Property changes request a capture on the next editor tick.
- Finished actor movement requests a capture on the next editor tick.
- When `Auto Capture` is enabled, editor preview updates every `Editor Capture Interval` seconds.

In Play mode, `Auto Capture` captures every frame.

## Blueprint API

- `CaptureNow()`
- `RefreshResources()`
- `GetPreviewRenderTarget()`
- `GetOutputRenderTarget()`
- `SetFaceEnabled(Face, bEnabled)`
- `IsFaceEnabled(Face)`
- `SetAllFacesEnabled(bEnabled)`

## Notes

The compute shader writes to an internal UAV-capable render target first. If an external output render target is assigned, the result is copied into it afterward, so the external target does not need to be UAV-capable.
