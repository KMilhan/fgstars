# Star Studio UX Direction

## Status

Draft product direction for the `fgstars v1` image-centric workspace.

This document describes the product-level UX shift from KStars/Ekos as a
planetarium-first astronomy application toward Star Studio as an
image-capture-first astrophotography workspace.

The implementation order is tracked separately in
[`Star Studio Implementation Blueprint`](star-studio-implementation-blueprint.md).

## Product Decision

Star Studio should make the captured image the primary workspace.

The sky map remains valuable, but it should no longer be the product's first
visual promise. In Star Studio, the planetarium surface becomes a supporting
target-finding tool used to choose what to shoot, not the main place where the
session is operated.

The first-screen signal should be:

- The user is in a capture studio.
- The current or next image is the center of work.
- Target search helps decide what to shoot.
- Alignment, solving, guiding, and focusing are contextual assistants around
  capture, not separate destinations the user must manually assemble.

## Problem Statement

Current KStars/Ekos exposes internal modules directly as the user's workflow.
That creates avoidable friction in the most common astrophotography journey:
choose a target, slew to it, center it, focus, guide, and capture frames.

The main problems are:

- The planetarium view is visually and conceptually dominant even when the user
  is trying to run an imaging session.
- Mount, Capture, Align, Focus, and Guide are presented as separate operating
  stations instead of one continuous capture flow.
- `Align` is overloaded language. It can mean polar alignment, plate solving,
  mount sync, field correction, or model building.
- Advanced solver controls consume primary screen space before the user has a
  reason to tune them.
- Detached FITS windows make image review feel secondary to the real workflow.

## Design Principles

### Task First

Design around what the user is trying to do:

- Find a target.
- Center the target.
- Capture a preview.
- Start or monitor a sequence.
- Inspect the latest image.

Avoid making users think in implementation modules such as Mount, Align,
Capture, or FITS Viewer unless they explicitly enter advanced tools.

### Image First

The largest and most stable surface should be the image workspace. Target
selection, solving, focusing, guiding, and capture history should attach to that
workspace as panels, overlays, or actions.

### Progressive Disclosure

Default controls should match the normal imaging path. Expert settings remain
available, but they should move into popovers, secondary panels, or advanced
dialogs.

### Preserve Expert Escape Hatches

Star Studio should not remove Ekos power features. It should change their
default prominence. The detached FITS Viewer, detailed solver configuration,
and module-specific panels remain available for advanced workflows.

## First Screen Layout

The default Star Studio workspace should open as a capture surface, not as a
full planetarium.

```text
+------------------------------------------------------------------+
| Header: target search | current target | devices | session state |
+----------------------+-------------------------------------------+
|                      |                                           |
| Target / Tools       |        Image Workspace                    |
| navigation           |        latest frame, preview, overlays    |
|                      |                                           |
|                      |                                           |
+----------------------+---------------------------+---------------+
| Recent frames / capture history                 | Capture panel  |
+-------------------------------------------------+---------------+
```

Recommended hierarchy:

- Center: `SummaryFITSView` or the shared image workspace.
- Header bar: target search, current target, connection/session status.
- Right panel: exposure, filter, count, storage, sequence start/stop.
- Bottom strip: recent captured frames and previews.
- Secondary navigation: `Capture`, `Targets`, `Focus`, `Guide`, `Advanced`.

The first screen should answer "what am I shooting and what did the camera just
see?" before it answers "where is this object on the whole sky map?"

## Information Architecture

Star Studio should rename user-facing destinations around capture tasks.

| Current surface | Star Studio surface | Primary user intent |
| --- | --- | --- |
| KStars Planetarium | Target Finder | Choose what to shoot |
| Ekos Capture | Capture Studio | Configure and run image capture |
| Align | Center Target | Put the selected object in frame |
| Focus | Focus Assistant | Improve star sharpness |
| Guide | Guiding Status | Keep tracking stable |
| FITS Viewer | Image Workspace | Inspect the latest image |
| Scheduler | Capture Plan | Queue the night's imaging work |

These names do not require immediate internal class renames. They define the
product language and navigation model.

## Target Finder

The former planetarium role should become `Target Finder`.

`Target Finder` is a supporting workflow for answering:

- What can I shoot tonight?
- Is this target visible from my location?
- When is it high enough?
- How will it fit my camera and telescope?
- Can I slew and center it now?

The default Target Finder surface should prioritize:

- Search results for named objects.
- Visibility window for tonight.
- Altitude and azimuth over time.
- Framing preview with the current optical train.
- Actions: `Go To`, `Center Target`, `Add to Capture Plan`.

A full interactive sky map can remain available, but it should behave like a
detail view or advanced browsing mode. It should not dominate the main capture
workspace.

## Primary Workflow

### Choose and Center a Target

1. The user searches for a target in the header bar.
2. Results show target identity, visibility, altitude window, and framing
   suitability.
3. The user chooses `Go To` or `Center Target`.
4. Star Studio slews, captures a solve frame, plate-solves, syncs or corrects
   the mount, and verifies framing.
5. The image workspace shows the verification frame and target overlay.

The user-facing concept is `Center Target`. `Plate Solve`, `Sync`, and `Slew`
are implementation details unless the operation fails or the user opens
advanced details.

### Capture a Preview

1. The user selects exposure and filter in the capture panel.
2. The user clicks `Capture Preview`.
3. The preview appears in the central image workspace.
4. Recent frames update in the bottom history strip.
5. Image overlays and quick tools remain attached to the current frame.

### Start a Sequence

1. The user confirms target, framing, exposure, filter, and frame count.
2. The user clicks `Start Sequence`.
3. Star Studio keeps the image workspace focused on the latest captured frame.
4. Capture progress, guiding quality, and focus status appear as compact
   contextual status, not as competing primary panes.

## Core Actions

Use direct task labels:

- `Find Target`
- `Go To`
- `Center Target`
- `Capture Preview`
- `Start Sequence`
- `Open Advanced Viewer`

Avoid making primary controls depend on expert terms:

- `Align`
- `Solver Action`
- `Sync`
- `Slew to Target`
- `Plate Solve Capture Options`

Those terms can still appear in advanced details, logs, and diagnostic panels.

## Center Target Interaction

`Center Target` should be a first-class action visible from the image workspace
and from Target Finder.

Recommended placement:

- Primary button near the image workspace toolbar or capture panel header.
- Secondary entry in each target search result.
- Optional image overlay when a target is selected but not yet centered.

Expected behavior:

1. Capture a solve frame with the current default solving exposure.
2. Run astrometry.
3. Calculate framing error.
4. Sync or correct the mount.
5. Re-capture to verify.
6. Show success, residual error, or a compact failure explanation.

Detailed solver settings should live behind an advanced popover next to the
action. Failure states may expose relevant advanced controls inline.

## Visual Priority

The image workspace should receive the most visual weight:

- It should be larger than any target list, log, or settings panel.
- It should stay visible while switching between capture-related modes.
- It should own overlays for target frame, solve result, focus ROI, guide
  annotations, and image inspection tools.
- It should avoid permanent chrome that reduces image area.

The planetarium or sky map should be visually subordinate:

- It can appear as a Target Finder detail pane.
- It can appear as a popover or secondary view from search results.
- It can support framing decisions, but should not replace the current image as
  the main workspace during capture.

## Relationship to fgstars v1

This UX direction depends on the existing `fgstars v1` image-centric workspace
plan:

- Promote the embedded `SummaryFITSView` into the visually primary workspace.
- Make the summary preview path the default capture surface.
- Keep the detached FITS Viewer as an explicit advanced action.
- Let Capture, Focus, Align, and Guide publish contextual information into the
  same image workspace over time.

The implementation should be incremental. Star Studio product language and
navigation can land before every internal module is fully renamed or unified.

## Implementation Sequence

The full phased implementation order is maintained in
[`Star Studio Implementation Blueprint`](star-studio-implementation-blueprint.md).
At a product level, the sequence is:

1. Promote the embedded image workspace so it is visually primary.
2. Make routine preview and capture review open in the embedded workspace by
   default.
3. Add `Find Target` / `Target Finder` as the product-facing replacement for
   the planetarium-first entry point.
4. Add `Center Target` as the primary user-facing wrapper around the common
   solve-and-correct flow.
5. Move advanced solver controls behind a popover or advanced panel.
6. Add capture history and latest-frame continuity around the image workspace.
7. Add shared workspace contributions from Focus, Align, and Guide.
8. Keep the full sky map and detached FITS Viewer available as explicit
   advanced tools.

## Acceptance Criteria

Star Studio is moving in the right direction when:

- Opening the app presents a capture image workspace as the dominant surface.
- A user can search for a target without leaving the capture context.
- A user can choose a target and run `Center Target` without knowing which Ekos
  module owns plate solving.
- Preview and routine captured frames appear in the embedded workspace by
  default.
- The detached FITS Viewer is available but no longer feels like the normal
  capture-review path.
- Advanced solver settings are reachable but do not occupy the default capture
  workspace.

## Non-Goals

This direction does not require:

- Rewriting KStars/Ekos in GTK or QML.
- Removing the full planetarium engine.
- Removing expert Ekos module controls.
- Renaming all internal classes in the first implementation pass.
- Replacing FITSView or FITSViewer infrastructure.
