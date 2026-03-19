export const MOUSE_COMPANION_DEFAULT_RUNTIME_STATE = {
  config_enabled: false,
  runtime_present: false,
  visual_host_active: false,
  model_loaded: false,
  visual_model_loaded: false,
  action_library_loaded: false,
  appearance_profile_loaded: false,
  pose_binding_configured: false,
  skeleton_bone_count: 0,
  last_action_name: '',
  last_action_code: -1,
  last_action_intensity: 0,
  last_action_tick_ms: 0,
  click_streak: 0,
  click_streak_tint_amount: 0,
  click_streak_break_ms: 650,
  click_streak_decay_per_second: 0.36,
  loaded_model_path: '',
  visual_model_path: '',
  model_load_error: '',
  visual_model_load_error: '',
  action_coverage: {
    ready: false,
    expected_action_count: 0,
    covered_action_count: 0,
    missing_action_count: 0,
    skeleton_bone_count: 0,
    total_track_count: 0,
    mapped_track_count: 0,
    overall_coverage_ratio: 0,
    error: '',
    missing_actions: [],
    missing_bone_names: [],
    actions: [],
  },
};

function textOrEmpty(input) {
  return `${input ?? ''}`.trim();
}

function parseIntSafe(input, fallback) {
  const parsed = Number.parseInt(`${input ?? ''}`, 10);
  return Number.isFinite(parsed) ? parsed : fallback;
}

function parseStringArray(input) {
  if (!Array.isArray(input)) {
    return [];
  }
  return input
    .map((value) => `${value ?? ''}`.trim())
    .filter((value) => value.length > 0);
}

function parseCoverageActions(input) {
  if (!Array.isArray(input)) {
    return [];
  }
  return input
    .map((entry) => {
      const sourceEntry = entry && typeof entry === 'object' ? entry : {};
      return {
        action_name: textOrEmpty(sourceEntry.action_name),
        clip_present: !!sourceEntry.clip_present,
        track_count: Math.max(0, parseIntSafe(sourceEntry.track_count, 0)),
        mapped_track_count: Math.max(0, parseIntSafe(sourceEntry.mapped_track_count, 0)),
        coverage_ratio: Number.isFinite(Number(sourceEntry.coverage_ratio))
          ? Number(sourceEntry.coverage_ratio)
          : 0,
        missing_bone_tracks: parseStringArray(sourceEntry.missing_bone_tracks),
      };
    })
    .filter((entry) => entry.action_name.length > 0);
}

function setFlagNode(byId, id, value) {
  const node = byId(id);
  if (!node) {
    return;
  }
  const key = value ? 'text_mouse_companion_runtime_true' : 'text_mouse_companion_runtime_false';
  node.setAttribute('data-i18n', key);
  node.textContent = value ? 'Yes' : 'No';
  node.classList.toggle('is-true', !!value);
  node.classList.toggle('is-false', !value);
}

function setTextNode(byId, id, value) {
  const node = byId(id);
  if (!node) {
    return;
  }
  node.textContent = `${value ?? ''}`;
}

function setPathNode(byId, id, value, isError) {
  const node = byId(id);
  if (!node) {
    return;
  }
  const text = value || '-';
  node.textContent = text;
  node.title = value || '';
  if (isError === true || isError === false) {
    node.classList.toggle('is-error', !!isError);
  }
}

export function normalizeMouseCompanionRuntimeState(value) {
  const source = value && typeof value === 'object' ? value : {};
  const coverageSource =
    source.action_coverage && typeof source.action_coverage === 'object'
      ? source.action_coverage
      : {};
  return {
    config_enabled: !!source.config_enabled,
    runtime_present: !!source.runtime_present,
    visual_host_active: !!source.visual_host_active,
    model_loaded: !!source.model_loaded,
    visual_model_loaded: !!source.visual_model_loaded,
    action_library_loaded: !!source.action_library_loaded,
    appearance_profile_loaded: !!source.appearance_profile_loaded,
    pose_binding_configured: !!source.pose_binding_configured,
    skeleton_bone_count: Math.max(0, parseIntSafe(source.skeleton_bone_count, 0)),
    last_action_name: textOrEmpty(source.last_action_name),
    last_action_code: parseIntSafe(source.last_action_code, -1),
    last_action_intensity: Number.isFinite(Number(source.last_action_intensity))
      ? Number(source.last_action_intensity)
      : 0,
    last_action_tick_ms: Math.max(0, parseIntSafe(source.last_action_tick_ms, 0)),
    click_streak: Math.max(0, parseIntSafe(source.click_streak, 0)),
    click_streak_tint_amount: Number.isFinite(Number(source.click_streak_tint_amount))
      ? Number(source.click_streak_tint_amount)
      : 0,
    click_streak_break_ms: Math.max(0, parseIntSafe(source.click_streak_break_ms, 650)),
    click_streak_decay_per_second: Number.isFinite(Number(source.click_streak_decay_per_second))
      ? Number(source.click_streak_decay_per_second)
      : 0.36,
    loaded_model_path: textOrEmpty(source.loaded_model_path),
    visual_model_path: textOrEmpty(source.visual_model_path),
    model_load_error: textOrEmpty(source.model_load_error),
    visual_model_load_error: textOrEmpty(source.visual_model_load_error),
    action_coverage: {
      ready: !!coverageSource.ready,
      expected_action_count: Math.max(0, parseIntSafe(coverageSource.expected_action_count, 0)),
      covered_action_count: Math.max(0, parseIntSafe(coverageSource.covered_action_count, 0)),
      missing_action_count: Math.max(0, parseIntSafe(coverageSource.missing_action_count, 0)),
      skeleton_bone_count: Math.max(0, parseIntSafe(coverageSource.skeleton_bone_count, 0)),
      total_track_count: Math.max(0, parseIntSafe(coverageSource.total_track_count, 0)),
      mapped_track_count: Math.max(0, parseIntSafe(coverageSource.mapped_track_count, 0)),
      overall_coverage_ratio: Number.isFinite(Number(coverageSource.overall_coverage_ratio))
        ? Number(coverageSource.overall_coverage_ratio)
        : 0,
      error: textOrEmpty(coverageSource.error),
      missing_actions: parseStringArray(coverageSource.missing_actions),
      missing_bone_names: parseStringArray(coverageSource.missing_bone_names),
      actions: parseCoverageActions(coverageSource.actions),
    },
  };
}

export function normalizeMouseCompanionProbeRuntimeResponse(payload) {
  const source = payload && typeof payload === 'object' ? payload : {};
  const runtime = source.runtime && typeof source.runtime === 'object' ? source.runtime : {};
  const actionCoverage =
    source.action_coverage && typeof source.action_coverage === 'object'
      ? source.action_coverage
      : {};
  return {
    ...runtime,
    action_coverage: actionCoverage,
  };
}

export function writeMouseCompanionRuntimeStateToDom(
  runtimeState,
  byId,
  defaults = MOUSE_COMPANION_DEFAULT_RUNTIME_STATE,
) {
  setFlagNode(byId, 'mc_runtime_runtime_present', runtimeState.runtime_present);
  setFlagNode(byId, 'mc_runtime_visual_host_active', runtimeState.visual_host_active);
  setFlagNode(byId, 'mc_runtime_model_loaded', runtimeState.model_loaded);
  setFlagNode(byId, 'mc_runtime_visual_model_loaded', runtimeState.visual_model_loaded);
  setFlagNode(byId, 'mc_runtime_action_library_loaded', runtimeState.action_library_loaded);
  setFlagNode(byId, 'mc_runtime_appearance_profile_loaded', runtimeState.appearance_profile_loaded);
  setFlagNode(byId, 'mc_runtime_pose_binding_configured', runtimeState.pose_binding_configured);

  setTextNode(byId, 'mc_runtime_skeleton_bone_count', runtimeState.skeleton_bone_count);
  setTextNode(byId, 'mc_runtime_last_action_name', runtimeState.last_action_name || '-');
  setTextNode(byId, 'mc_runtime_last_action_code', runtimeState.last_action_code);
  setTextNode(byId, 'mc_runtime_last_action_intensity', runtimeState.last_action_intensity.toFixed(3));
  setTextNode(byId, 'mc_runtime_last_action_tick_ms', runtimeState.last_action_tick_ms);
  setTextNode(byId, 'mc_runtime_click_streak', runtimeState.click_streak);
  setTextNode(
    byId,
    'mc_runtime_head_tint_amount',
    runtimeState.click_streak_tint_amount.toFixed(3),
  );
  setTextNode(byId, 'mc_runtime_click_streak_break_ms', runtimeState.click_streak_break_ms);
  setTextNode(
    byId,
    'mc_runtime_head_tint_decay_per_second',
    runtimeState.click_streak_decay_per_second.toFixed(3),
  );

  setPathNode(byId, 'mc_runtime_loaded_model_path', runtimeState.loaded_model_path, null);
  setPathNode(byId, 'mc_runtime_visual_model_path', runtimeState.visual_model_path, null);
  setPathNode(
    byId,
    'mc_runtime_model_load_error',
    runtimeState.model_load_error,
    !!runtimeState.model_load_error,
  );
  setPathNode(
    byId,
    'mc_runtime_visual_model_load_error',
    runtimeState.visual_model_load_error,
    !!runtimeState.visual_model_load_error,
  );

  const actionCoverage = runtimeState.action_coverage || defaults.action_coverage;
  setFlagNode(byId, 'mc_runtime_action_coverage_ready', actionCoverage.ready);
  setTextNode(
    byId,
    'mc_runtime_action_coverage_ratio',
    `${(actionCoverage.overall_coverage_ratio * 100).toFixed(1)}%`,
  );
  setTextNode(
    byId,
    'mc_runtime_action_coverage_tracks',
    `${actionCoverage.mapped_track_count}/${actionCoverage.total_track_count}`,
  );
  setTextNode(
    byId,
    'mc_runtime_action_coverage_actions',
    `${actionCoverage.covered_action_count}/${actionCoverage.expected_action_count}`,
  );
  setTextNode(
    byId,
    'mc_runtime_action_coverage_missing_actions',
    actionCoverage.missing_actions.length > 0 ? actionCoverage.missing_actions.join(', ') : '-',
  );
  setTextNode(
    byId,
    'mc_runtime_action_coverage_missing_bones',
    actionCoverage.missing_bone_names.length > 0 ? actionCoverage.missing_bone_names.join(', ') : '-',
  );
  setPathNode(
    byId,
    'mc_runtime_action_coverage_error',
    actionCoverage.error,
    !!actionCoverage.error,
  );

  const coverageActionDetailsNode = byId('mc_runtime_action_coverage_action_details');
  if (!coverageActionDetailsNode) {
    return;
  }
  if (actionCoverage.actions.length <= 0) {
    coverageActionDetailsNode.textContent = '-';
    coverageActionDetailsNode.title = '';
    return;
  }
  const detailText = actionCoverage.actions
    .map((entry) => {
      const ratio = `${(entry.coverage_ratio * 100).toFixed(1)}%`;
      const missingTracks =
        entry.missing_bone_tracks.length > 0
          ? `, missing=[${entry.missing_bone_tracks.join('|')}]`
          : '';
      return `${entry.action_name}: ${entry.mapped_track_count}/${entry.track_count}, ${ratio}, clip=${entry.clip_present ? 'yes' : 'no'}${missingTracks}`;
    })
    .join(' ; ');
  coverageActionDetailsNode.textContent = detailText;
  coverageActionDetailsNode.title = detailText;
}
