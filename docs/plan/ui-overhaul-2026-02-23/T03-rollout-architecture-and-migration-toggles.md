# T03 — Phased Rollout Architecture and Migration Toggles

Status: approved  
Plan ID: `ui-overhaul-2026-02-23`  
Task ID: `T03`  
Date: 2026-02-23

## 1) Approved rollout architecture

### 1.1 Goals
- Keep legacy and new UI paths coexisting safely during staged rollout.
- Minimize gameplay/menu integration risk by reusing existing parity wrappers.
- Ensure fast rollback at runtime (when both paths are compiled) and at build time.

### 1.2 Existing architecture anchors (implemented)
- Compile-time gate and runtime selector: `src/ui_migration_toggle.h`
  - `BR_UI_COMPILE_ALLOW_LEGACY` (0/1)
  - `BR_UI_COMPILE_ALLOW_NEW` (0/1)
  - `BR_UI_COMPILE_DEFAULT_NEW` (0/1)
  - Runtime override env var: `BR_UI_PATH=legacy|new|auto` (only when both paths are compiled)
- HUD path dispatch: `src/hud.h`
  - `drawUI()` routes to `drawLegacyUI()` or `drawNewUI()` via `useNewUiMigrationPath()`.
  - Current `drawLegacyUI()` and `drawNewUI()` both call the parity renderer (`drawUiParityPath()`) to keep behavior aligned during migration.
- Menu input path dispatch: `src/input_menu.h`
  - `menuInput()` routes to `menuInputLegacy()` or `menuInputNew()` via `useNewUiMigrationPath()`.
  - Current legacy/new wrappers both call parity handler (`menuInputParityPath(GLFWwindow*)`).

### 1.3 Phased architecture model
- **Phase 0 — Parity foundation (current):**
  - Both paths compiled, behavior equivalent by parity wrappers.
  - Purpose: validate toggle plumbing and branch safety.
- **Phase 1 — HUD migration:**
  - New HUD diverges behind `drawNewUI()` while menu input remains parity.
  - Runtime A/B possible through `BR_UI_PATH`.
- **Phase 2 — Menu/input migration:**
  - New menu/input diverges behind `menuInputNew()` and new menu draw path.
  - Legacy remains available for fallback.
- **Phase 3 — New-default hardening:**
  - New path defaulted in staging/candidate builds.
  - Legacy retained for emergency fallback until release acceptance.
- **Phase 4 — Legacy retirement (post-acceptance):**
  - Optional compile profile disables legacy path after sustained stability window.

## 2) Toggle strategy with default states per build type

Build types below are the required migration profiles and are implemented via compiler defines in `build.bat` (and pass-through `build_client_windows.bat`).

| Build type | `ALLOW_LEGACY` | `ALLOW_NEW` | `DEFAULT_NEW` | Runtime `BR_UI_PATH` use | Default behavior |
|---|---:|---:|---:|---|---|
| Local Dev | 1 | 1 | 0 | Enabled | Boots legacy; can switch to new/auto without rebuild |
| QA / Test | 1 | 1 | 1 | Enabled | Boots new; can force legacy for verification |
| Staging / RC | 1 | 1 | 1 | Enabled (restricted to test env) | Boots new with rollback safety |
| Production Stable (during migration) | 1 | 1 | 0 | Disabled by ops policy (do not set env) | Boots legacy by default, emergency override possible |
| Production New-Default (after acceptance) | 1 | 1 | 1 | Disabled by ops policy (do not set env) | Boots new, legacy available for emergency fallback |
| Emergency Legacy-Only artifact | 1 | 0 | 0 | N/A | Compiled legacy only |
| Post-migration New-Only artifact | 0 | 1 | 1 | N/A | Compiled new only |

Notes:
- `DEFAULT_NEW=1` is only valid when `ALLOW_NEW=1` (already compile-guarded in code).
- At least one path must be compiled (already compile-guarded in code).
- Runtime env override is valid only when both paths are compiled.

Windows wiring:
- `build.bat` accepts profile as first arg (or env `BR_BUILD_PROFILE`) and sets:
  - `BR_UI_COMPILE_ALLOW_LEGACY`
  - `BR_UI_COMPILE_ALLOW_NEW`
  - `BR_UI_COMPILE_DEFAULT_NEW`
- `build_client_windows.bat` forwards args to `build.bat`.
- Example invocations:
  - `build.bat local-dev`
  - `build.bat qa-test`
  - `build.bat emergency-legacy-only`

## 3) Rollback procedure per phase

### Phase 0 rollback (parity foundation)
- **Trigger:** startup/input regressions after integrating toggle plumbing.
- **Action order:**
  1. Set runtime `BR_UI_PATH=legacy` (if dual-path artifact).
  2. If regression persists, rebuild as legacy-only (`ALLOW_LEGACY=1`, `ALLOW_NEW=0`).
  3. Block forward migration until parity smoke tests pass.

### Phase 1 rollback (HUD migration)
- **Trigger:** HUD rendering/perf/readability regressions.
- **Action order:**
  1. Runtime fallback to `BR_UI_PATH=legacy` in impacted environments.
  2. Keep dual-path QA artifact for bug isolation.
  3. If severe, ship emergency legacy-only artifact.

### Phase 2 rollback (menu/input migration)
- **Trigger:** navigation lock, input routing mismatch, menu flow breakage.
- **Action order:**
  1. Runtime fallback to `BR_UI_PATH=legacy`.
  2. Re-test parity path (`menuInputParityPath`) as baseline.
  3. If unresolved before release window, deploy legacy-only artifact.

### Phase 3 rollback (new-default hardening)
- **Trigger:** unacceptable defect rate in staging/candidate new-default builds.
- **Action order:**
  1. Keep dual-path artifact, flip default back (`DEFAULT_NEW=0`) for next candidate.
  2. Continue running new-path in QA via runtime override for targeted fixes.
  3. Delay promotion to production-new-default until exit criteria pass.

### Phase 4 rollback (legacy retirement)
- **Trigger:** post-retirement critical issue in new-only builds.
- **Action order:**
  1. Rehydrate emergency legacy-capable artifact profile (dual-path or legacy-only).
  2. Redeploy with legacy default while hotfixing new-only path.
  3. Re-enter retirement only after stability soak.

## 4) Operational guardrails
- Record active UI path at process start (`currentUiMigrationPathName()`) in startup logs.
- For production builds, treat `BR_UI_PATH` as operationally restricted (debug/test environments only).
- Keep one prebuilt emergency legacy-capable artifact available for rapid rollback.
- Phase exit requires: no blocker defects, smoke pass for menu/HUD/gameplay loop, and multiplayer sanity check.

## 5) Approval
- Engineering Lead: Approved
- Technical Lead: Approved
- UI Lead: Approved
