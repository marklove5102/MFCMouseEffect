export const MOUSE_COMPANION_DEFAULT_ACTIVE_TAB = 'basic';

export const MOUSE_COMPANION_TAB_IDS = ['basic', 'follow', 'probe', 'runtime'];

export const MOUSE_COMPANION_CHECKBOX_FIELDS = [
  { key: 'enabled', id: 'mc_enabled' },
  { key: 'use_test_profile', id: 'mc_use_test_profile' },
];

export const MOUSE_COMPANION_TEXT_FIELDS = [
  { key: 'model_path', id: 'mc_model_path' },
  { key: 'action_library_path', id: 'mc_action_library_path' },
  { key: 'appearance_profile_path', id: 'mc_appearance_profile_path' },
];

export const MOUSE_COMPANION_NUMBER_FIELDS = [
  { key: 'size_px', id: 'mc_size_px' },
  { key: 'offset_x', id: 'mc_offset_x' },
  { key: 'offset_y', id: 'mc_offset_y' },
  { key: 'press_lift_px', id: 'mc_press_lift_px' },
  { key: 'smoothing_percent', id: 'mc_smoothing_percent' },
  { key: 'follow_threshold_px', id: 'mc_follow_threshold_px' },
  { key: 'release_hold_ms', id: 'mc_release_hold_ms' },
  { key: 'test_press_lift_px', id: 'mc_test_press_lift_px' },
  { key: 'test_smoothing_percent', id: 'mc_test_smoothing_percent' },
];

export const MOUSE_COMPANION_RANGE_BINDINGS = [
  { id: 'mc_size_px', schemaKey: 'size_px_range' },
  { id: 'mc_offset_x', schemaKey: 'offset_range' },
  { id: 'mc_offset_y', schemaKey: 'offset_range' },
  { id: 'mc_press_lift_px', schemaKey: 'press_lift_px_range' },
  { id: 'mc_smoothing_percent', schemaKey: 'smoothing_percent_range' },
  { id: 'mc_follow_threshold_px', schemaKey: 'follow_threshold_px_range' },
  { id: 'mc_release_hold_ms', schemaKey: 'release_hold_ms_range' },
  { id: 'mc_test_press_lift_px', schemaKey: 'test_press_lift_px_range' },
  { id: 'mc_test_smoothing_percent', schemaKey: 'test_smoothing_percent_range' },
];
