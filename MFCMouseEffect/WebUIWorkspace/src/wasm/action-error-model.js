const ERROR_CODE_MESSAGES = {
  manifest_path_required: ['wasm_error_manifest_path_required', 'Plugin manifest path is required.'],
  manifest_path_not_found: ['wasm_error_manifest_path_not_found', 'Plugin manifest path does not exist.'],
  manifest_path_not_file: ['wasm_error_manifest_path_not_file', 'Plugin manifest path is not a file.'],
  manifest_load_failed: ['wasm_error_manifest_load_failed', 'Failed to load plugin manifest.'],
  source_entry_invalid: ['wasm_error_source_entry_invalid', 'Source plugin entry file is invalid.'],
  primary_root_resolve_failed: ['wasm_error_primary_root_resolve_failed', 'Failed to resolve primary plugin root.'],
  copy_failed: ['wasm_error_copy_failed', 'Failed to copy plugin files.'],
  destination_manifest_missing: ['wasm_error_destination_manifest_missing', 'Destination plugin manifest is missing.'],
  destination_entry_invalid: ['wasm_error_destination_entry_invalid', 'Destination plugin entry file is invalid.'],
  no_plugins_discovered: ['wasm_error_no_plugins_discovered', 'No plugins were discovered for export.'],
  export_root_resolve_failed: ['wasm_error_export_root_resolve_failed', 'Failed to resolve plugin export root.'],
  create_export_directory_failed: ['wasm_error_create_export_directory_failed', 'Failed to create plugin export directory.'],
  no_plugin_copied: ['wasm_error_no_plugin_copied', 'No plugin was copied during export.'],
  selected_folder_manifest_missing: ['wasm_error_selected_folder_manifest_missing', 'plugin.json is missing in selected folder.'],
  folder_picker_cancelled: ['wasm_error_folder_picker_cancelled', 'Folder selection was cancelled.'],
  folder_picker_failed: ['wasm_error_folder_picker_failed', 'Folder picker failed.'],
  native_folder_picker_not_supported: ['wasm_error_native_folder_picker_not_supported', 'Native folder picker is not supported on this platform.'],
};

export function normalizeActionErrorCode(value) {
  return `${value || ''}`.trim().toLowerCase();
}

export function resolveWasmActionErrorMessage(errorCode, translate) {
  const code = normalizeActionErrorCode(errorCode);
  if (!code) {
    return '';
  }
  const entry = ERROR_CODE_MESSAGES[code];
  if (!entry) {
    return '';
  }
  const [key, fallback] = entry;
  if (typeof translate === 'function') {
    return translate(key, fallback);
  }
  return fallback;
}
