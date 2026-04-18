# Ekos Image-Centric Workspace Initiative — Sub-Issue Draft

This draft is intentionally dependency-ordered and grouped by milestone.
It matches the current Ekos architecture, where the embedded workspace is Setup-tab-local today and broader shared-workspace behavior must land incrementally.

## Milestone 1 — Baseline and Rollout Preparation

### I1 — Freeze the current embedded-workspace architecture and acceptance baseline
Title: Document the current Setup-tab-local image flow and the initial acceptance baseline.
Why: The initiative needs a stable description of what exists today before the workspace becomes more central.
Scope: Record the current `Manager` -> `CapturePreviewWidget` -> `SummaryFITSView` path, the detached `FITSViewer` fallback, and the existing UI regression targets that can serve as baseline checks.
Acceptance Criteria: The initiative docs clearly separate current architecture, current regression coverage, and missing fgstars-v1 acceptance coverage.
Dependencies: None.
Verification: Review the current code paths and confirm the baseline command set still runs against the named targets.

### I2 — Clarify rollout settings and migration behavior
Title: Define the rollout contract around `useSummaryPreview`, detached FITS Viewer behavior, and any new workspace toggle.
Why: v1 should reuse existing knobs where possible instead of inventing an all-new migration model.
Scope: Document how the current summary preview option interacts with detached viewer behavior and specify whether a new workspace toggle is needed.
Acceptance Criteria: The rollout plan is explicit about legacy behavior, preferred behavior, and how users can fall back during migration.
Dependencies: I1.
Verification: Review current settings and confirm the documented migration behavior matches live options and code paths.

## Milestone 2 — Workspace Promotion

### I3 — Promote `SummaryFITSView` to the visually primary workspace surface
Title: Make the embedded workspace the obvious first path for routine image review.
Why: The current layout still reads as controls-first with a secondary preview.
Scope: Rebalance the Setup-tab layout so the embedded workspace is visually primary while keeping existing controls and status visible.
Acceptance Criteria: The workspace clearly dominates the working area for capture-first workflows without removing existing controls.
Dependencies: I1, I2.
Verification: Run the capture-focused UI regressions and review screenshots from the promoted layout.

### I4 — Clarify workspace-host ownership
Title: Cleanly separate responsibilities between `Manager`, `CapturePreviewWidget`, and `SummaryFITSView`.
Why: The current hosting path works, but it needs clearer ownership before more modules publish into the workspace.
Scope: Define which object owns layout, which object owns image loading, and which object owns overlays and contextual controls.
Acceptance Criteria: The workspace host path is clear enough to extend without duplicating image-loading logic.
Dependencies: I3.
Verification: Review the code after the change and confirm the host responsibilities are explicit and limited.

## Milestone 3 — Canonical Capture Surface

### I5 — Make the existing summary preview path the canonical default capture surface
Title: Standardize routine capture review on the embedded workspace.
Why: Capture already has the strongest embedded path, so it should become the canonical default capture surface before broader module work begins.
Scope: Keep using the existing summary preview infrastructure, but make it the preferred default for preview and routine captured-frame review.
Acceptance Criteria: Capture paths use the embedded workspace by default for normal review flows, while keeping the detached advanced viewer available on request.
Dependencies: I3, I4.
Verification: Run capture regressions and confirm images arrive in the embedded workspace without requiring the detached viewer.

### I6 — Preserve capture history and explicit detached-viewer fallback
Title: Keep the current history and advanced-viewer escape hatches intact.
Why: v1 should reduce detached-viewer dependence without breaking users who still need explicit advanced tools.
Scope: Preserve history browsing, deletion/trash behaviors, and explicit user actions that open detached `FITSViewer`.
Acceptance Criteria: Capture history remains usable and the detached viewer still opens only on explicit request.
Dependencies: I5.
Verification: Exercise history navigation and explicit advanced-viewer actions in targeted UI tests.

## Milestone 4 — Shared Workspace Bridge and Continuity

### I7 — Introduce a shared workspace bridge
Title: Add a narrow publication boundary for workspace-bound images and context.
Why: Focus, Align, and Guide currently own module-local views, so the first shared step should be a bridge rather than full state unification.
Scope: Introduce a bridge object or equivalent interface that lets image-aware modules publish image/view updates into one shared workspace sink.
Acceptance Criteria: Multiple modules can publish into one sink without immediately replacing their internal state ownership.
Dependencies: I4, I5.
Verification: Add focused tests around bridge publication ordering and sink updates.

### I8 — Preserve viewport continuity after the bridge lands
Title: Keep zoom and pan state stable where practical after shared publication exists.
Why: Viewport continuity is a user-visible win, but it should follow the bridge instead of blocking it.
Scope: Preserve zoom and pan state for the canonical capture path first, then extend continuity to bridged module flows where practical.
Acceptance Criteria: The first continuity pass avoids unnecessary viewport resets in the supported v1 paths.
Dependencies: I7.
Verification: Add targeted UI tests that switch between supported flows and confirm viewport continuity expectations.

## Milestone 5 — Initial Module Integration

### I9 — Bridge Focus, Align, and Guide image outputs into the shared workspace sink
Title: Publish initial non-capture image context into the shared workspace.
Why: The initiative should show cross-module value in v1, but only through a narrow, realistic bridge-first step.
Scope: Bridge Focus, Align, and Guide image outputs into the shared workspace sink while leaving deeper module rewrites out of scope for v1.
Acceptance Criteria: Each supported module can publish initial image context into the shared workspace without losing its existing internal tools.
Dependencies: I7.
Verification: Run module-specific flows and confirm at least one relevant image/context update appears in the shared workspace sink.

### I10 — Add initial overlay and context adapters
Title: Surface practical module context in the shared workspace without a full overlay rewrite.
Why: Current module visuals are coupled to module-local views, so v1 should target initial adapters instead of a universal overlay system.
Scope: Add initial adapters for focus ROI/HFR context, align solve/FOV context, and guide visual context where practical.
Acceptance Criteria: The shared workspace can show useful module context for the supported v1 paths, even if some advanced module visuals remain local.
Dependencies: I8, I9.
Verification: Review targeted artifacts and UI tests to confirm the initial adapters are understandable and non-disruptive.

## Milestone 6 — Validation and Rollout

### I11 — Add workspace-specific UI acceptance coverage and eval gating
Title: Turn the initiative into a real acceptance-tested rollout.
Why: Existing named UI targets are useful baseline regressions, but they do not yet prove the new workspace behavior.
Scope: Add workspace-specific UI acceptance coverage for canonical capture routing, shared-workspace bridge behavior, viewport continuity, and explicit detached-viewer fallback. Keep the runs non-headless.
Acceptance Criteria: The initiative has explicit workspace-specific UI acceptance coverage in addition to baseline regressions.
Dependencies: I5, I6, I7, I8, I9, I10.
Verification: Execute the new tests plus the current baseline regression targets and inspect workspace artifacts when visual changes are involved.

### I12 — Decide default-on rollout and cleanup
Title: Make the final rollout decision after acceptance coverage and UX validation.
Why: Default-on should depend on proven behavior, not just code landing.
Scope: Final cleanup, docs, rollout notes, and the decision to keep the feature guarded or switch it on by default.
Acceptance Criteria: The initiative ends with a documented rollout decision backed by workspace-specific UI acceptance coverage and stable regression results.
Dependencies: I11.
Verification: Review the final acceptance results, rollout notes, and user-facing defaults before flipping the final switch.
