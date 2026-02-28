function normalizeInteger(value) {
  const parsed = Number(value);
  if (!Number.isFinite(parsed)) {
    return 0;
  }
  return Math.max(0, Math.round(parsed));
}

function normalizeBoolean(value) {
  return !!value;
}

function normalizeText(value) {
  return `${value || ''}`.trim();
}

export function normalizeWasmDiagnostics(input) {
  const value = input || {};
  return {
    last_call_duration_us: normalizeInteger(value.last_call_duration_us),
    last_output_bytes: normalizeInteger(value.last_output_bytes),
    last_command_count: normalizeInteger(value.last_command_count),
    last_call_exceeded_budget: normalizeBoolean(value.last_call_exceeded_budget),
    last_call_rejected_by_budget: normalizeBoolean(value.last_call_rejected_by_budget),
    last_output_truncated_by_budget: normalizeBoolean(value.last_output_truncated_by_budget),
    last_command_truncated_by_budget: normalizeBoolean(value.last_command_truncated_by_budget),
    last_budget_reason: normalizeText(value.last_budget_reason),
    last_parse_error: normalizeText(value.last_parse_error),
    last_load_failure_stage: normalizeText(value.last_load_failure_stage),
    last_load_failure_code: normalizeText(value.last_load_failure_code),
  };
}

export function hasWasmDiagnosticWarning(diagnostics) {
  const value = diagnostics || {};
  if (value.last_call_exceeded_budget) return true;
  if (value.last_call_rejected_by_budget) return true;
  if (value.last_output_truncated_by_budget) return true;
  if (value.last_command_truncated_by_budget) return true;
  if (normalizeText(value.last_load_failure_stage)) return true;
  if (normalizeText(value.last_load_failure_code)) return true;
  const parseError = normalizeText(value.last_parse_error).toLowerCase();
  return parseError.length > 0 && parseError !== 'none';
}

export function buildWasmCallMetricsText(diagnostics, translate) {
  const value = diagnostics || {};
  const t = typeof translate === 'function' ? translate : (_key, fallback) => fallback;
  return `${t('label_wasm_metric_duration', 'Duration')}=${value.last_call_duration_us}us, `
    + `${t('label_wasm_metric_output_bytes', 'Output')}=${value.last_output_bytes}B, `
    + `${t('label_wasm_metric_command_count', 'Commands')}=${value.last_command_count}`;
}

export function buildWasmBudgetFlagsText(diagnostics, translate) {
  const value = diagnostics || {};
  const t = typeof translate === 'function' ? translate : (_key, fallback) => fallback;
  const yes = t('wasm_text_yes', 'Yes');
  const no = t('wasm_text_no', 'No');
  const boolText = (flag) => (flag ? yes : no);
  return `${t('label_wasm_budget_flag_exceeded', 'Exceeded')}=${boolText(value.last_call_exceeded_budget)}, `
    + `${t('label_wasm_budget_flag_rejected', 'Rejected')}=${boolText(value.last_call_rejected_by_budget)}, `
    + `${t('label_wasm_budget_flag_output_truncated', 'OutputTrunc')}=${boolText(value.last_output_truncated_by_budget)}, `
    + `${t('label_wasm_budget_flag_command_truncated', 'CommandTrunc')}=${boolText(value.last_command_truncated_by_budget)}`;
}

export function buildWasmLifetimeInvokeText(snapshot, translate) {
  const value = snapshot || {};
  const t = typeof translate === 'function' ? translate : (_key, fallback) => fallback;
  return `${t('label_wasm_metric_total', 'Total')}=${value.lifetime_invoke_calls || 0}, `
    + `${t('label_wasm_metric_success', 'Success')}=${value.lifetime_invoke_success_calls || 0}, `
    + `${t('label_wasm_metric_failed', 'Failed')}=${value.lifetime_invoke_failed_calls || 0}, `
    + `${t('label_wasm_metric_exceeded_budget', 'ExceededBudget')}=${value.lifetime_invoke_exceeded_budget_calls || 0}, `
    + `${t('label_wasm_metric_rejected_budget', 'RejectedBudget')}=${value.lifetime_invoke_rejected_by_budget_calls || 0}`;
}

export function buildWasmLifetimeRenderText(snapshot, translate) {
  const value = snapshot || {};
  const t = typeof translate === 'function' ? translate : (_key, fallback) => fallback;
  return `${t('label_wasm_metric_total', 'Total')}=${value.lifetime_render_dispatches || 0}, `
    + `${t('label_wasm_metric_rendered', 'Rendered')}=${value.lifetime_rendered_by_wasm_dispatches || 0}, `
    + `${t('label_wasm_metric_text_commands', 'TextCmd')}=${value.lifetime_executed_text_commands || 0}, `
    + `${t('label_wasm_metric_image_commands', 'ImgCmd')}=${value.lifetime_executed_image_commands || 0}, `
    + `${t('label_wasm_metric_throttled', 'Throttled')}=${value.lifetime_throttled_render_commands || 0} `
    + `(cap=${value.lifetime_throttled_by_capacity_render_commands || 0}, int=${value.lifetime_throttled_by_interval_render_commands || 0}), `
    + `${t('label_wasm_metric_dropped', 'Dropped')}=${value.lifetime_dropped_render_commands || 0}`;
}
