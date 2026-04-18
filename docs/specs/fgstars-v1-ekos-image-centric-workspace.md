# fgstars v1 Spec: Ekos Image-Centric Workspace

## Status

Draft v1

## Objective

Define `fgstars v1` as the first major milestone that shifts Ekos from a tab-and-form-first experience toward an image-centric workflow centered on one shared image workspace.

The user goal is to let astrophotography users work against one central image surface while capture, focus, alignment, and guiding tools contribute contextual overlays, status, and actions instead of forcing routine work through detached image windows.

## Background

Ekos already contains most of the technical building blocks needed for an image-centric UI:

- `SummaryFITSView` is embedded into the Ekos setup flow.
- `FITSView` already supports zoom, pan, selection, overlays, grids, star detection, and image updates.
- `FITSViewer` already provides advanced FITS-specific actions and remains useful as a detached advanced viewer.

Capture already has an embedded image path today. When `useSummaryPreview` is enabled, `CapturePreviewWidget` loads preview and captured frames into the embedded `SummaryFITSView`.

The current limitation is architectural, not foundational. The embedded workspace is `Setup-tab-local`, not yet an Ekos-wide persistent shell surface, and Focus, Align, and Guide still own module-local image views and optional detached viewer flows. That prevents Ekos from behaving like one continuous visual workspace.

## Product Goal

`fgstars v1` establishes the following default interaction model:

- Ekos presents a visually primary embedded image workspace for normal capture-first operation.
- The existing summary preview path becomes the canonical default capture surface.
- Focus, Align, and Guide can publish initial context into the shared workspace where practical.
- Module-specific information is shown as overlays, contextual panels, or workspace-side controls.
- The standalone FITS viewer remains available for advanced or detached workflows, but it is no longer the default path for normal Ekos capture review.

## Non-Goals

The following are explicitly out of scope for `fgstars v1`:

- Rewriting Ekos in GTK, QML, or another UI framework.
- Replacing `FITSView` with a new rendering engine.
- Fully redesigning Scheduler, Analyze, or Observatory around the image workspace.
- Porting the new UX to KStars Lite.
- Fully unifying internal image ownership for every module in one pass.
- Reproducing the full Siril processing toolchain inside Ekos.

## Users and Primary Workflows

### Primary users

- Existing KStars/Ekos astrophotography users.
- Advanced astrophotography users who expect rapid visual iteration against recent frames.
- Users who currently rely on FITS Viewer only because the embedded preview is not sufficient.

### Primary workflows

1. Start Ekos and immediately see a large embedded image workspace in the main working area.
2. Capture a preview or frame and inspect it without switching to an external viewer.
3. Switch between Capture and other image-aware modules while retaining a coherent visual context.
4. Use zoom, pan, selection, overlay toggles, and frame history from one workspace-first path.
5. Open an advanced detached FITS Viewer only when explicitly requested.

## Existing Architecture

### Top-level Ekos shell

- `Ekos::Manager` is a `QDialog` with a vertical root layout and a top-level vertical splitter.
- The main content host is `toolsWidget`, a `QTabWidget`.
- The embedded image workspace currently lives inside the Setup tab, not as a top-level Ekos-wide shell surface.
- The setup flow contains `deviceSplitter`, a horizontal splitter with:
  - left: `CapturePreviewWidget`
  - right: mount/focus/guide status widgets

Relevant files:

- `kstars/ekos/manager.ui`
- `kstars/ekos/manager.cpp`
- `kstars/ekos/manager.h`

### Embedded capture workspace

- `Ekos::Manager` owns workspace placement in the Setup-tab shell and exposes the shared embedded preview entry point.
- `CapturePreviewWidget` owns embedded workspace creation, lifetime, and the capture overlay host inside `previewWidget`.
- `SummaryFITSView` is a specialized `FITSView` with overlay hosting and a floating toolbar.
- `CapturePreviewWidget::updateJobProgress()` already routes image data into the embedded view when `useSummaryPreview` is enabled.

Relevant files:

- `kstars/fitsviewer/summaryfitsview.h`
- `kstars/fitsviewer/summaryfitsview.cpp`
- `kstars/ekos/capture/capturepreviewwidget.h`
- `kstars/ekos/capture/capturepreviewwidget.cpp`
- `kstars/kstars.kcfg`

### Detached advanced image viewer

- `KStars::createFITSViewer()` creates or returns a `FITSViewer`.
- `CameraProcess::updateFITSViewer()` routes new frames to the detached viewer when enabled.
- Focus, Align, and Guide also keep module-local image views and can update detached FITSViewer tabs through their own paths.

Relevant files:

- `kstars/fitsviewer/fitsviewer.h`
- `kstars/fitsviewer/fitsview.h`
- `kstars/kstars.cpp`
- `kstars/ekos/capture/cameraprocess.cpp`
- `kstars/ekos/focus/focus.cpp`
- `kstars/ekos/align/align.cpp`
- `kstars/ekos/guide/guide.cpp`

## v1 Scope

### In scope

- Promote the existing embedded `SummaryFITSView` into the visually primary Ekos workspace.
- Make the existing summary preview path the canonical default capture surface.
- Unify capture history, current frame display, and process overlays inside that workspace.
- Introduce a narrow shared `workspace bridge` so image-aware modules can publish into one shared sink before full state unification exists.
- Preserve detached `FITSViewer` support as an explicit advanced action.
- Gate rollout behind existing and new options where needed for safe migration.

### Out of scope for v1 implementation

- Full workspace-driven redesign of non-image-heavy modules.
- Eliminating every separate preview path in one pass.
- Replacing module-local image state with one fully centralized object in the first milestone set.
- Heavy refactors to INDI, DBus, or FITS data core structures unrelated to workspace ownership.

## Functional Requirements

### FR1: Visually primary embedded workspace

Ekos shall present a visually primary embedded image workspace for capture-first workflows.

Acceptance criteria:

- A large image workspace is visible in the Ekos setup/camera workflow by default.
- The workspace is no longer treated as a secondary preview-only surface.
- The rollout can begin from the existing Setup-tab host before an Ekos-wide persistent shell lands.

### FR2: Canonical default capture surface

The existing summary preview path shall become the canonical default capture surface for preview and routine captured-frame review.

Acceptance criteria:

- Preview and routine captured frames appear in the embedded workspace without requiring a detached FITS Viewer.
- Existing capture history browsing continues to function.
- Detached FITS Viewer opening remains explicit and opt-in rather than implicit or default.

### FR3: Shared workspace bridge

Ekos shall introduce a shared workspace bridge for workspace-bound images and context.

Acceptance criteria:

- Capture, Focus, Align, and Guide can publish image or view updates into one shared workspace sink without requiring immediate internal state unification.
- Workspace-side overlays and contextual controls can consume bridge updates from multiple modules.
- The bridge does not require destabilizing INDI or DBus flows.

### FR4: Viewport continuity

Ekos shall preserve viewport continuity where practical once the shared workspace bridge is in place.

Acceptance criteria:

- Module switches do not unnecessarily discard zoom or pan state after the relevant bridge path lands.
- The first v1 delivery may preserve continuity for capture-first flows before broader module parity exists.
- Continuity work can follow bridge adoption instead of blocking initial workspace promotion.

### FR5: Initial module workspace integration

Image-aware modules shall contribute initial context to the shared workspace whenever feasible.

Acceptance criteria:

- Capture process status and history controls remain available as overlays or side controls.
- Focus can contribute ROI, HFR, focus-star, or related visual context through the workspace bridge where practical.
- Align can contribute solve, FOV, grid, or related image annotations where practical.
- Guide can contribute initial guiding visual context where feasible in v1.

### FR6: Advanced viewer fallback

The standalone FITS Viewer shall remain available as an advanced or detached workflow.

Acceptance criteria:

- Users can still explicitly open the detached FITS Viewer.
- Advanced FITS-specific tooling remains accessible.
- Existing detached-viewer workflows are not broken by the new default.

### FR7: Safe rollout controls

The redesign shall support incremental rollout.

Acceptance criteria:

- The new behavior can be gated through an option, feature flag, or migration toggle.
- Existing behavior can remain available during the transition.
- The rollout path does not require a flag day rewrite.

## UX Requirements

### Layout

- The image workspace must visually dominate the main Ekos working area.
- The right-hand side must function as contextual controls/status, not as the primary focus.
- Logs and background actions should remain accessible without overwhelming the workspace.

### Interaction

- Users must be able to zoom, pan, fit-to-window, and inspect frames from the shared workspace.
- Frame updates should feel continuous across the canonical capture path before full cross-module parity is required.
- Module-specific overlays should be discoverable and togglable.

### Information density

- Capture process metadata should remain available but should not permanently consume primary image space.
- Overlays should surface task-critical information first.
- Detached tool windows should be reduced to advanced flows only.

## Architecture Requirements

### AR1: Reuse existing Qt Widgets and FITS infrastructure

The solution must reuse the current `Qt Widgets`, `FITSView`, `SummaryFITSView`, and `FITSViewer` infrastructure.

### AR2: Preserve detached FITSViewer compatibility

Current `FITSViewer` creation and update paths must remain compatible until migration is complete.

### AR3: Keep module wiring incremental

Module integration must be staged so that capture-first workspace promotion lands before broader shared-workspace bridge and viewport continuity work.

### AR4: Avoid destabilizing INDI and DBus flows

UI ownership changes must not require unnecessary changes to device communication or DBus APIs.

## Dependency Graph

The implementation dependency graph is:

1. Workspace spec and acceptance baseline
2. Layout and embedded workspace promotion
3. Capture-first default routing and history behavior
4. Shared workspace bridge
5. Viewport continuity
6. Initial Focus/Align/Guide integration
7. Rollout cleanup and final defaulting decision

Expressed as a graph:

```text
Spec + acceptance baseline
    ->
Workspace host promotion
    ->
Canonical capture surface
    ->
Shared workspace bridge
    ->
Viewport continuity
    ->
Initial module integration
    ->
Rollout cleanup and defaulting decision
```

## Milestones

### fgstars v1.0a: Baseline and acceptance preparation

- Freeze the current Setup-tab-local architecture and migration seams.
- Confirm which existing tests are baseline regressions and which new tests must be added for fgstars v1 acceptance.
- Document rollout controls around `useSummaryPreview`, detached FITS Viewer behavior, and any new workspace toggle.

### fgstars v1.0b: Workspace promotion

- Make the embedded workspace visually primary.
- Clean up `CapturePreviewWidget` and `SummaryFITSView` hosting responsibilities.
- Add top-level affordances that make the embedded workspace the obvious first path.

### fgstars v1.0c: Canonical capture surface

- Route preview and routine capture review through the embedded workspace by default.
- Preserve history browsing and explicit detached `FITSViewer` fallback behavior.

### fgstars v1.0d: Shared workspace bridge

- Introduce a reusable bridge for cross-module workspace publication.
- Keep the bridge narrow so it can land before full shared state ownership exists.

### fgstars v1.0e: Viewport continuity and initial module integration

- Preserve viewport continuity where practical after the bridge lands.
- Add initial Focus, Align, and Guide workspace contributions without requiring a full module rewrite.

### fgstars v1.0f: Stabilization and rollout decision

- Tighten tests and regressions.
- Tune layout and user-facing behavior.
- Decide whether the feature flag becomes default-on or remains a production-ready guarded path.

## Planned GitHub Issue Breakdown

The `fgstars v1` milestone should be tracked as one epic plus dependency-ordered sub-issues:

### fgstars v1.0a

1. Freeze the current Setup-tab-local workspace architecture and acceptance baseline.
2. Clarify rollout settings and migration behavior around `useSummaryPreview`, detached FITS Viewer usage, and any new workspace toggle.

### fgstars v1.0b

3. Promote `SummaryFITSView` to the visually primary Ekos workspace surface.
4. Clarify workspace-host ownership between `Manager`, `CapturePreviewWidget`, and `SummaryFITSView`.

### fgstars v1.0c

5. Make the existing summary preview path the canonical default capture surface.
6. Preserve capture history browsing and the explicit detached `FITSViewer` fallback.

### fgstars v1.0d

7. Introduce a shared workspace bridge for Capture, Focus, Align, and Guide publication.
8. Preserve viewport continuity after the bridge lands.

### fgstars v1.0e

9. Add initial Focus workspace integration through the shared bridge.
10. Add initial Align workspace integration through the shared bridge.
11. Add initial Guide workspace integration where feasible in v1.

### fgstars v1.0f

12. Add workspace-specific UI acceptance coverage, non-headless eval gating, rollout cleanup, and the default-on decision.

## Testing Strategy

The repository already contains useful UI and integration testing infrastructure, but the current named targets are baseline Ekos regressions, not fgstars v1 acceptance coverage.

### Build commands

```bash
cmake -S . -B build-tests -DBUILD_TESTING=ON
cmake --build build-tests --target all -j"$(nproc)"
```

### Current baseline regression targets

- `TestEkosCapture`
- `TestEkosCaptureWorkflow`
- `TestEkosSchedulerOps`
- `TestEkosScheduler`

### Supplemental target relevant to this initiative

- `TestEkosFocus`

`TestEkosFocus` is currently useful as a targeted run for focus-related regression checks, but it is not part of the current stable-label gate.

### Full stable validation

```bash
ctest --test-dir build-tests/Tests -L '^stable$' -LE '^unstable$' --output-on-failure
```

### Current repo eval wrapper

```bash
BUILD_DIR=/home/mill/oss/fgstars/build-tests /home/mill/oss/fgstars/scripts/eval_tests.sh -R "TestEkosCapture|TestEkosCaptureWorkflow|TestEkosSchedulerOps|TestEkosScheduler"
```

`scripts/eval_tests.sh` is currently a pass-rate wrapper, not a two-signal evaluator.

### v1-specific validation requirements

- Add or extend UI tests that verify the embedded workspace is the default visual target.
- Verify capture frame routing into the embedded workspace.
- Verify that the shared workspace bridge behaves correctly for initial module integrations.
- Verify that viewport continuity does not regress once continuity work lands.
- Verify that detached FITS Viewer still opens on explicit request.

## Evaluation Criteria

For implementation work under this initiative:

- Keep the current stable regression gate green.
- Add workspace-specific non-headless UI acceptance coverage before default-on rollout.
- Treat UI/E2E validation and artifact inspection as required for image-centric changes.
- Until the repo grows an independent second scorer, use targeted tests plus artifact review as the active gate instead of assuming `overall_score` and `llm_average` are independent signals.

## Rollout Plan

### Phase 1

- Ship behind an option or guarded behavior.
- Keep legacy detached FITS flow intact.

### Phase 2

- Make embedded workspace the preferred capture-review path.
- Keep detached viewer as advanced fallback.

### Phase 3

- Consider default-on rollout once new acceptance coverage and usability are stable.

## Risks

### Risk: State duplication between embedded and detached viewers

Impact: high

Mitigation:

- Centralize workspace publication before broad state unification.
- Reduce parallel UI ownership of the same image state.

### Risk: Module-specific preview logic is more coupled than expected

Impact: high

Mitigation:

- Migrate Capture first.
- Introduce the workspace bridge before attempting broad internal state merging.

### Risk: UI regressions in stable Ekos workflows

Impact: high

Mitigation:

- Keep rollout guarded.
- Add focused workspace acceptance coverage before moving defaults.

### Risk: Overlays become cluttered

Impact: medium

Mitigation:

- Define overlay ownership and toggle strategy per module.
- Prefer contextual side panels for dense settings.

## Open Questions

- Should `fgstars v1` end with default-on behavior, or with a production-ready feature flag?
- How much Guide integration is realistic in v1 versus a follow-up milestone?
- Should the Setup tab remain the near-term home of the image workspace, or should the workspace become a broader Ekos shell concept in v2?

## Deliverables

`fgstars v1` is complete when:

- The embedded image workspace is the normal visual center for routine Ekos capture review.
- Capture-first flows no longer depend on detached FITS Viewer windows by default.
- A shared workspace bridge exists or an equivalent narrow publication boundary is in place.
- Focus, Align, and Guide have at least initial shared-workspace integration.
- Stable regressions remain green and new workspace acceptance coverage exists for the v1 behaviors above.
