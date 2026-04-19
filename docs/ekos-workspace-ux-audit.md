# Ekos Workspace UX Audit

This is a living, screenshot-driven audit for `fgstars v1`.

Its purpose is not just to catch functional regressions. It records where the
image-centric Ekos workspace still feels visually secondary, cramped, or too
close to the legacy tab-and-form workflow. If a screenshot shows a structural
UX problem, that problem should be treated as a redesign candidate, not as a
small polish note.

## Review Method

- Capture real, non-headless screenshots while using the actual program or UI
  tests.
- Review the screenshots for hierarchy, legibility, responsiveness, density,
  and visual quality.
- Log large UX needs here before or alongside implementation work.
- Prefer structural changes over piecemeal tweaks when the screenshots show
  that the workspace still reads as a preview embedded inside a larger form.

## Evidence Set

Local screenshots from the current review session live under
`.artifacts/ui-shots/`.

Primary evidence files used for this audit:

- `ekos-shell-connected-frame.png`
- `ekos-shell-with-preview.png`
- `ekos-window-resize-recovery.png`
- `ekos-size-probe-import.png`
- `ekos-size-probe-import-late.png`
- `ekos-workspace-resize-recovery.png`

## Structural Causes Confirmed In Code

The screenshot findings above are not just subjective. The current shell still
has several code-level constraints that push the UI back toward a legacy
control-first composition.

### Conflicting minimum sizes block real responsiveness

- The dialog advertises a `700x700` minimum.
- At the same time, child widgets still enforce large practical minimums:
  preview shell around `300`, embedded preview around `250`, counts widget
  around `600`, and a right rail capped near `480`.
- The net effect is that the shell survives at smaller sizes but does not
  behave gracefully there.

Relevant code:

- `kstars/ekos/manager.ui`
- `kstars/ekos/manager.cpp`
- `kstars/ekos/capture/capturepreviewwidget.ui`
- `kstars/ekos/capture/capturecountswidget.ui`

### Splitter policy still encodes a legacy hierarchy

- The top-level shell is still governed by a vertical splitter and fixed
  seed ratios that preserve older pane relationships.
- The workspace resize protection work keeps the image from collapsing, but the
  current policy still treats several secondary panes as always-mounted peers.

Relevant code:

- `kstars/ekos/manager.ui`
- `kstars/ekos/manager.cpp`

### The right rail is still three dense widgets stacked into one fixed lane

- Mount, focus, and guide are still presented as a fixed vertical stack.
- That works for functional coverage, but at reduced heights it produces small,
  busy cards rather than one coherent contextual rail.

Relevant code:

- `kstars/ekos/manager.ui`
- `kstars/ekos/manager.cpp`

### The preview host still carries too much permanent chrome

- The preview region still mixes title, train controls, labels, indicators,
  and a mounted counts region around the image.
- That prevents the frame from reading as the clean primary object even when
  the image area itself is large enough.

Relevant code:

- `kstars/ekos/capture/capturepreviewwidget.ui`
- `kstars/ekos/capture/capturepreviewwidget.cpp`

## Current High-Priority UX Needs

### 1. The shared image workspace is larger, but it still does not feel like the product center

What the screenshots show:

- The image surface is visually separated from the real working controls by a
  heavy strip of large circular module icons.
- The lower half of the shell is still dominated by legacy capture forms,
  empty panels, and utility controls.
- The result still reads as "preview on top, control deck below" instead of
  "one shared workspace with contextual controls around it."

Why this matters:

- This is the core product promise of `fgstars v1`. If the workspace does not
  dominate the composition, the feature lands as a layout tweak instead of a
  workflow shift.

Large change likely required:

- Promote the workspace into its own shell region with a small header and a
  true contextual side rail.
- Demote or collapse the legacy capture form deck into drawers, tabs, or a
  progressive disclosure panel.

Relevant code:

- `kstars/ekos/manager.ui`
- `kstars/ekos/manager.cpp`
- `kstars/ekos/capture/capturepreviewwidget.cpp`

### 2. The horizontal module icon bar consumes premium vertical space and breaks the visual hierarchy

What the screenshots show:

- The row of large circular icons sits directly between the workspace and the
  rest of the UI.
- It draws attention away from the frame, acts like a hard divider, and makes
  the shell feel more like a dashboard than a focused imaging workspace.

Why this matters:

- The eye lands on the icon bar before it lands on either the frame or the
  contextual tools.
- On smaller heights, that row takes space away from the image and amplifies
  the feeling that the user is managing modules rather than working on one
  continuous image surface.

Large change likely required:

- Replace the large icon row with a lighter navigation treatment.
- Move module switching into a slimmer header, segmented control, or sidebar
  entry point.

Relevant code:

- `kstars/ekos/manager.ui`

### 3. Narrow and mid-sized windows still degrade into clipped or crowded forms

What the screenshots show:

- At smaller sizes the capture controls become dense, clipped, and hard to
  scan.
- The shared workspace survives, but the surrounding UI no longer feels
  intentional or robust.
- Dense controls, partial labels, and packed button rows reduce confidence and
  make the UI look unfinished.

Why this matters:

- A layout can be functionally alive and still fail the user experience bar.
- If smaller sizes look cramped, the workspace redesign will not feel
  production ready even when the image remains visible.

Large change likely required:

- Introduce responsive breakpoints for the Ekos shell.
- Below a width threshold, move dense capture settings into collapsible
  sections, drawers, or a secondary stacked mode rather than letting the form
  compress indefinitely.

Relevant code:

- `kstars/ekos/manager.cpp`
- `kstars/ekos/capture/capturepreviewwidget.cpp`
- `kstars/ekos/capture/capture.cpp`

### 4. The right-side rail has useful information, but it still reads as a loose collection of widgets

What the screenshots show:

- Mount, Focus, and related status blocks are not yet presented as a coherent
  contextual rail.
- Visual spacing and grouping are inconsistent, and the guide graph competes
  awkwardly with status text.

Why this matters:

- The right side should feel like one purpose-built context rail for the
  current frame and active module.
- Right now it still looks like several older widgets placed next to a newer
  workspace.

Large change likely required:

- Rebuild the right side as stacked cards or sections with consistent spacing,
  titles, status emphasis, and optional collapse behavior.
- Ensure the guide graph and status blocks align to one clear hierarchy.

Relevant code:

- `kstars/ekos/manager.ui`
- `kstars/ekos/manager/focusprogresswidget.cpp`

### 5. The empty-state and placeholder visual language looks broken instead of intentional

What the screenshots show:

- The dimmed placeholder image with a large prohibition-style symbol feels like
  a warning or unavailable state.
- It does not communicate "ready workspace" or "waiting for next frame."

Why this matters:

- Before the first frame arrives, the workspace should still feel premium and
  purposeful.
- A weak empty-state lowers the perceived quality of the whole shell.

Large change likely required:

- Replace the current placeholder treatment with a deliberate empty-state that
  explains what will appear there and what the user can do next.
- Keep the workspace chrome active even before the first image.

Relevant code:

- `kstars/fitsviewer/summaryfitsview.cpp`

### 6. The session timeline and lower-pane content become visually noisy under capture

What the screenshots show:

- The timeline state shown in `ekos-size-probe-import-late.png` is information
  rich but visually overloaded.
- In the current form it pulls attention away from the image and becomes hard
  to parse quickly.

Why this matters:

- The lower region should support the workspace, not compete with it.
- High-density timelines are useful, but they need a compact mode and a clear
  default presentation.

Large change likely required:

- Split "quick history / recent frames" from "dense timeline analysis."
- Default to a compact, readable recent-session strip and expose the richer
  timeline on demand.

Relevant code:

- `kstars/ekos/capture/capturehistory.cpp`
- `kstars/ekos/capture/capturehistorynavigation.cpp`

## Screenshot Review Bar

The workspace should not be considered visually successful until all of these
are true in real screenshots:

- At `1280x860`, the image workspace is the first thing the eye notices.
- At `980x700`, the shell still feels deliberate rather than compressed.
- At `700x920`, the contextual rail remains readable and orderly.
- At `700x700`, no primary controls are clipped and the UI uses responsive
  collapse instead of brute-force compression.
- The empty-state looks intentional before the first frame.
- The post-capture state remains calmer and clearer than the current dense
  timeline-heavy presentation.

## Suggested Next Large Changes

1. Create a true workspace shell with a workspace header, image center, and
   contextual right rail.
2. Collapse the legacy capture form deck into responsive drawers or stacked
   detail panels.
3. Replace the large module icon strip with lighter navigation.
4. Redesign the empty-state and the post-capture lower history/timeline area.
5. Add screenshot checkpoints for the target window sizes above and review them
   before default-on rollout decisions.

## Iteration Log

### 2026-04-19

- Confirmed via screenshots that the current shell is functionally stronger but
  still not aesthetically at the level of a truly image-centric workspace.
- Confirmed that window-resize survivability is improving, but responsive
  behavior is still not good enough to count as high-quality UX.
- Logged the large changes above so future iterations can justify structural UI
  work rather than only incremental splitter tuning.
