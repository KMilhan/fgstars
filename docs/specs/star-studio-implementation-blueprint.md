# Star Studio Implementation Blueprint

## Status

Draft implementation blueprint.

This document turns the Star Studio UX direction into an implementation order.
It assumes the product direction in
[`Star Studio UX Direction`](ekos-gnome-hig-ux-redesign.md) and the technical
workspace plan in
[`fgstars v1 Spec: Ekos Image-Centric Workspace`](fgstars-v1-ekos-image-centric-workspace.md).

## Guiding Rule

Do not rename or rearrange the UI faster than the simulator and journey tests
can verify the new behavior.

The safe order is:

1. Make the simulator-backed test foundation trustworthy.
2. Stabilize the image workspace as the primary working surface.
3. Add Star Studio user-facing workflows on top of that foundation.
4. Move legacy KStars/Ekos affordances behind advanced or fallback paths only
   after the new workflow is covered.

## Current Assessment

KStars to FGStars has already improved the testable surface:

- INDI simulator profiles can exercise Telescope, CCD, Focuser, Guide, Weather,
  Rotator, and Light Panel paths.
- `TestEkosWorkspaceCapture` covers embedded workspace host behavior, resize
  priority, tab persistence, detached viewer fallback, capture session state,
  and guide workspace context.
- `TestEkosWorkspaceAlign` covers initial align annotations in the shared
  workspace.
- `TestEkosCaptureWorkflow` contains blackbox target-selection and repeated
  capture journeys.

The remaining gap is product-level verification:

- `TestEkosCaptureWorkflow` is still marked unstable, so it is not yet a
  reliable gate for the Star Studio journey.
- Simulator star availability must become a real assertion instead of a helper
  name that always succeeds.
- The existing workspace tests prove important slices, but they do not yet prove
  the whole `Find Target -> Center Target -> Capture Preview -> Inspect Frame`
  journey.
- Target Finder and Center Target need their own acceptance tests before they
  become the product's default path.

## Blueprint Phases

| Phase | Goal | Implementation focus | Validation gate |
| --- | --- | --- | --- |
| 0 | Make simulator results trustworthy | Real CCD simulator star availability, blackbox capture flake reduction | No fake-green simulator setup |
| 1 | Lock the image workspace foundation | Persistent `SummaryFITSView`, visual priority, default embedded preview | `TestEkosWorkspaceCapture` |
| 2 | Make capture-first routing canonical | Preview, capture, history, and detached fallback behavior | Capture frames route to embedded workspace by default |
| 3 | Introduce Target Finder | Product-facing target search and framing context over the existing KStars object search | Target search stays inside capture context |
| 4 | Introduce Center Target | One user action for slew, solve, sync/correct, and verification | Simulator-backed Center Target journey |
| 5 | Add the Star Studio shell | Capture-first first screen, task labels, advanced escape hatches | Screenshot and journey acceptance |
| 6 | Mature failure paths | No stars, solve failure, weather alert, guide unavailable, target not visible | Recovery UI is covered by tests |
| 7 | Decide default-on rollout | Feature flag cleanup, default migration, legacy path preservation | Full workspace eval gate |

## Phase 0: Trustworthy Simulator Foundation

### Goal

Before the product shell changes, make simulator-based acceptance reliable
enough to reject broken behavior.

### Implementation Tasks

- Replace placeholder star availability checks with a real CCD simulator
  assertion.
- Keep the check narrow: capture or load a simulator-backed frame, verify that
  the image can support the star-dependent assertions that the test will make,
  and return a useful skip/failure reason.
- Reduce flakes in `TestEkosCaptureWorkflow` around modal dialogs, active
  windows, INDI startup, and timing-sensitive capture completion checks.
- Keep the existing simulator profile helper as the common setup path instead
  of creating per-test profiles by hand.

### Acceptance Criteria

- A test that requires detectable stars cannot pass when the simulator has no
  usable stars.
- The blackbox target/capture journey has a clear path to move from
  `unstable;ui` toward stable gating.
- Skips are explicit and environmental; product regressions fail.

## Phase 1: Image Workspace Foundation

### Goal

Make the embedded workspace the stable center of the application before new
Star Studio affordances depend on it.

### Implementation Tasks

- Keep `Manager -> CapturePreviewWidget -> SummaryFITSView` as the initial
  architectural seam.
- Preserve the embedded workspace above image-heavy module switches.
- Ensure the workspace stays visually larger than contextual side panels at
  supported window sizes.
- Keep `useSummaryPreview=true` and `useFITSViewer=false` as the intended
  non-hardware-limited default path.

### Acceptance Criteria

- `TestEkosWorkspaceCapture` remains green.
- The embedded workspace stays visible across Capture, Focus, Align, and Guide.
- Detached FITS Viewer does not open during routine capture review.

## Phase 2: Capture-First Routing

### Goal

Make preview, capture, latest-frame review, and history navigation use the
embedded image workspace by default.

### Implementation Tasks

- Route routine capture frames into the workspace.
- Preserve explicit `Open Advanced Viewer` behavior from workspace history.
- Keep capture process overlays and session state tied to the current workspace
  frame.
- Keep the detached FITS Viewer as a fallback, not as the default review path.

### Acceptance Criteria

- Captured frames appear in `SummaryFITSView`.
- History navigation updates the workspace frame.
- Opening the detached FITS Viewer requires an explicit action.

## Phase 3: Target Finder

### Goal

Reframe the planetarium role as a supporting target-selection workflow.

### Implementation Tasks

- Add a product-facing `Find Target` or `Target Finder` entry point without
  removing the full sky map.
- Start by wrapping the existing KStars object search and selected-object state.
- Show capture-relevant target context first: visibility, altitude window,
  current framing suitability, and available actions.
- Keep the full interactive sky map available as a detail or advanced view.

### Acceptance Criteria

- The user can search for a target without leaving the capture context.
- Target search exposes `Go To`, `Center Target`, and `Add to Capture Plan`
  actions.
- The selected target becomes visible to the capture workspace/session state.

## Phase 4: Center Target

### Goal

Turn the routine alignment flow into one user-facing action.

### Implementation Tasks

- Add a `Center Target` action that orchestrates the existing low-level steps:
  capture solve frame, solve, sync or correct, slew if needed, re-capture, and
  verify.
- Keep `Align`, `Plate Solve`, `Sync`, and solver configuration visible in
  advanced details and diagnostics, not as the default task labels.
- Publish solve result, residual error, target frame, and verification image to
  the shared workspace.
- Add a dedicated `StarStudioCenterTargetJourney` test slice once the first
  orchestration path lands.

### Acceptance Criteria

- A user can run `Center Target` without manually switching to the Align module.
- The workspace shows the verification frame and relevant overlay state.
- Failure produces a compact actionable state, not only a log line.

## Phase 5: Star Studio Shell

### Goal

Make the first screen communicate that Star Studio is a capture application.

### Implementation Tasks

- Make the image workspace the dominant first-screen surface.
- Move task labels toward `Find Target`, `Center Target`, `Capture Preview`,
  `Start Sequence`, and `Open Advanced Viewer`.
- Keep advanced Ekos module controls reachable without making them the primary
  path.
- Add screenshot-based or artifact-based review for supported window sizes.

### Acceptance Criteria

- Opening the app presents the image workspace as the dominant surface.
- Target selection and capture controls are available without exposing all
  module internals.
- Text and controls fit at supported desktop window sizes.

## Phase 6: Failure Path Maturity

### Goal

Make the simulator suite cover the places where real users lose trust.

### Implementation Tasks

- Cover no detectable stars.
- Cover solve failure.
- Cover weather alert blocking capture or scheduling.
- Cover guide unavailable or guide context missing.
- Cover target not visible or below constraints.

### Acceptance Criteria

- Failure states are visible in the Star Studio workflow.
- The user sees what can be retried, changed, or opened in advanced details.
- The tests distinguish product failure from unavailable local test resources.

## Phase 7: Default-On Rollout

### Goal

Decide when the Star Studio path can become the normal path.

### Implementation Tasks

- Keep feature flags or guarded defaults until simulator-backed journeys are
  stable.
- Preserve the full sky map and detached FITS Viewer as advanced escape hatches.
- Keep `scripts/eval_fgstars_workspace.sh` aligned with the current acceptance
  gate.
- Promote Target Finder and Center Target tests into the initiative gate before
  making them default expectations.

### Acceptance Criteria

- Stable regression targets remain green.
- Workspace-specific acceptance targets remain green.
- Star Studio target/capture journeys are stable enough to fail product
  regressions rather than test harness noise.

## First Implementation Slice

The first PR-sized slice should be verification-first:

1. Make CCD simulator star availability a real check.
2. Reduce the most visible `TestEkosCaptureWorkflow` blackbox journey flakes.
3. Add a skeleton acceptance target for Star Studio Target Finder or Center
   Target journey.
4. Keep product UI changes minimal until the new test can observe them.

This slice intentionally improves the foundation before renaming or moving large
parts of the UI.

## Architecture Anchors

Use the existing seams first:

- `Ekos::Manager` owns the shell placement and shared workspace access.
- `CapturePreviewWidget` owns embedded workspace hosting in the current setup.
- `SummaryFITSView` remains the initial image workspace surface.
- `WorkspaceSession` or an equivalent narrow bridge records the currently
  published frame, overlays, and viewport state.
- Capture, Focus, Align, and Guide should publish context into the shared
  workspace before any broad module rewrite.

Avoid introducing a parallel UI or image-rendering stack in the first pass.

## Definition of Done

Star Studio is ready to become the normal path when:

- Simulator-backed journeys can verify target selection, centering, preview
  capture, and image inspection.
- Routine captures stay in the embedded workspace.
- Center Target works as a single user action with useful success and failure
  states.
- The full sky map and detached FITS Viewer remain available as explicit
  advanced tools.
- The eval wrapper documents and runs the current acceptance gate.
