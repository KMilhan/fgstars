#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd -- "${SCRIPT_DIR}/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"${REPO_ROOT}/build-tests"}

WORKSPACE_REGEX='^(TestEkosCapture|TestEkosCaptureWorkflow|TestStarStudioTargetFinderJourney|TestStarStudioCenterTargetJourney|TestStarStudioShellJourney|TestStarStudioFailureStates|TestStarStudioTargetVisibilityFailure|TestEkosSchedulerOps|TestEkosScheduler|TestEkosFocus|TestEkosWizard|TestEkosWorkspaceCapture|TestEkosWorkspaceAlign)$'

exec "${SCRIPT_DIR}/eval_tests.sh" -R "${WORKSPACE_REGEX}"
