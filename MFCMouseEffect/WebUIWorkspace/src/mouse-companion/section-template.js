export function getMouseCompanionSectionMarkup() {
  return `
    <div class="mouse-companion-tabs" role="tablist" aria-label="Mouse companion settings tabs">
      <button id="mc_tab_basic" type="button" class="mouse-companion-tab" role="tab" aria-controls="mc_panel_basic" data-i18n="tab_mouse_companion_basic">Basic</button>
      <button id="mc_tab_follow" type="button" class="mouse-companion-tab" role="tab" aria-controls="mc_panel_follow" data-i18n="tab_mouse_companion_follow">Follow</button>
      <button id="mc_tab_probe" type="button" class="mouse-companion-tab" role="tab" aria-controls="mc_panel_probe" data-i18n="tab_mouse_companion_probe">Probe</button>
      <button id="mc_tab_runtime" type="button" class="mouse-companion-tab" role="tab" aria-controls="mc_panel_runtime" data-i18n="tab_mouse_companion_runtime">Runtime</button>
    </div>
    <section id="mc_panel_basic" class="mouse-companion-panel" role="tabpanel" aria-labelledby="mc_tab_basic">
      <div class="grid">
        <label for="mc_enabled" class="label-with-tip"><span data-i18n="label_mouse_companion_enabled">Enable Mouse Companion</span></label>
        <label class="startup-toggle" for="mc_enabled">
          <input id="mc_enabled" class="startup-toggle__input" type="checkbox" />
          <span class="startup-toggle__switch" aria-hidden="true"><span class="startup-toggle__thumb"></span></span>
          <span id="mc_enabled_text" class="startup-toggle__text" data-i18n="text_mouse_companion_off">Disabled</span>
        </label>

        <label for="mc_model_path" class="label-with-tip"><span data-i18n="label_mouse_companion_model_path">Model Path (USDZ/GLB)</span></label>
        <input id="mc_model_path" type="text" data-i18n-placeholder="placeholder_mouse_companion_model_path" placeholder="MFCMouseEffect/Assets/Pet3D/source/pet-main.glb" />

        <label for="mc_action_library_path" class="label-with-tip"><span data-i18n="label_mouse_companion_action_library_path">Action Library Path (JSON)</span></label>
        <input id="mc_action_library_path" type="text" data-i18n-placeholder="placeholder_mouse_companion_action_library_path" placeholder="MFCMouseEffect/Assets/Pet3D/source/pet-actions.json" />

        <label for="mc_appearance_profile_path" class="label-with-tip"><span data-i18n="label_mouse_companion_appearance_profile_path">Appearance Profile Path (JSON)</span></label>
        <input id="mc_appearance_profile_path" type="text" data-i18n-placeholder="placeholder_mouse_companion_appearance_profile_path" placeholder="MFCMouseEffect/Assets/Pet3D/source/pet-appearance.json" />

        <label for="mc_size_px" data-i18n="label_mouse_companion_size_px">Companion Size (px)</label>
        <input id="mc_size_px" type="number" />
      </div>
    </section>
    <section id="mc_panel_follow" class="mouse-companion-panel" role="tabpanel" aria-labelledby="mc_tab_follow" hidden>
      <div class="grid">
        <label for="mc_position_mode" data-i18n="label_mouse_companion_position_mode">Position Mode</label>
        <select id="mc_position_mode"></select>

        <label for="mc_edge_clamp_mode" data-i18n="label_mouse_companion_edge_clamp_mode">Edge Clamp Mode</label>
        <select id="mc_edge_clamp_mode"></select>

        <label for="mc_offset_x" data-i18n="label_mouse_companion_offset_x">Offset X</label>
        <input id="mc_offset_x" type="number" />

        <label for="mc_offset_y" data-i18n="label_mouse_companion_offset_y">Offset Y</label>
        <input id="mc_offset_y" type="number" />

        <label for="mc_press_lift_px" data-i18n="label_mouse_companion_press_lift_px">Press Lift (px)</label>
        <input id="mc_press_lift_px" type="number" />

        <label for="mc_smoothing_percent" data-i18n="label_mouse_companion_smoothing_percent">Follow Smoothing (%)</label>
        <input id="mc_smoothing_percent" type="number" />

        <label for="mc_follow_threshold_px" data-i18n="label_mouse_companion_follow_threshold_px">Follow Threshold (px)</label>
        <input id="mc_follow_threshold_px" type="number" />

        <label for="mc_release_hold_ms" data-i18n="label_mouse_companion_release_hold_ms">Release Hold (ms)</label>
        <input id="mc_release_hold_ms" type="number" />

        <label for="mc_face_pointer_enabled" class="label-with-tip"><span data-i18n="label_mouse_companion_face_pointer_enabled">Pointer Facing Follow</span></label>
        <label class="startup-toggle" for="mc_face_pointer_enabled">
          <input id="mc_face_pointer_enabled" class="startup-toggle__input" type="checkbox" />
          <span class="startup-toggle__switch" aria-hidden="true"><span class="startup-toggle__thumb"></span></span>
          <span id="mc_face_pointer_enabled_text" class="startup-toggle__text" data-i18n="text_mouse_companion_face_pointer_off">Disabled</span>
        </label>

        <label for="mc_click_streak_break_ms" data-i18n="label_mouse_companion_click_streak_break_ms">Click Streak Break (ms)</label>
        <input id="mc_click_streak_break_ms" type="number" />

        <label for="mc_head_tint_per_click" data-i18n="label_mouse_companion_head_tint_per_click">Head Tint Per Click (0~1)</label>
        <input id="mc_head_tint_per_click" type="number" step="0.01" min="0.01" max="1" />

        <label for="mc_head_tint_max" data-i18n="label_mouse_companion_head_tint_max">Head Tint Max (0~1)</label>
        <input id="mc_head_tint_max" type="number" step="0.01" min="0.01" max="1" />

        <label for="mc_head_tint_decay_per_second" data-i18n="label_mouse_companion_head_tint_decay_per_second">Head Tint Decay (/s)</label>
        <input id="mc_head_tint_decay_per_second" type="number" step="0.01" min="0.05" max="4" />

        <label for="mc_use_test_profile" class="label-with-tip"><span data-i18n="label_mouse_companion_use_test_profile">Use Test Profile</span></label>
        <label class="startup-toggle" for="mc_use_test_profile">
          <input id="mc_use_test_profile" class="startup-toggle__input" type="checkbox" />
          <span class="startup-toggle__switch" aria-hidden="true"><span class="startup-toggle__thumb"></span></span>
          <span id="mc_use_test_profile_text" class="startup-toggle__text" data-i18n="text_mouse_companion_test_off">Production</span>
        </label>

        <label for="mc_test_press_lift_px" data-i18n="label_mouse_companion_test_press_lift_px">Test Press Lift (px)</label>
        <input id="mc_test_press_lift_px" type="number" />

        <label for="mc_test_smoothing_percent" data-i18n="label_mouse_companion_test_smoothing_percent">Test Smoothing (%)</label>
        <input id="mc_test_smoothing_percent" type="number" />

        <label for="mc_test_click_streak_break_ms" data-i18n="label_mouse_companion_test_click_streak_break_ms">Test Click Break (ms)</label>
        <input id="mc_test_click_streak_break_ms" type="number" />

        <label for="mc_test_head_tint_per_click" data-i18n="label_mouse_companion_test_head_tint_per_click">Test Tint Per Click (0~1)</label>
        <input id="mc_test_head_tint_per_click" type="number" step="0.01" min="0.01" max="1" />

        <label for="mc_test_head_tint_max" data-i18n="label_mouse_companion_test_head_tint_max">Test Tint Max (0~1)</label>
        <input id="mc_test_head_tint_max" type="number" step="0.01" min="0.01" max="1" />

        <label for="mc_test_head_tint_decay_per_second" data-i18n="label_mouse_companion_test_head_tint_decay_per_second">Test Tint Decay (/s)</label>
        <input id="mc_test_head_tint_decay_per_second" type="number" step="0.01" min="0.05" max="4" />
      </div>
    </section>
    <section id="mc_panel_probe" class="mouse-companion-panel" role="tabpanel" aria-labelledby="mc_tab_probe" hidden>
      <div class="mouse-companion-probe">
        <div class="mouse-companion-probe__title" data-i18n="title_mouse_companion_probe">Action Probe</div>
        <div class="mouse-companion-probe__hint" data-i18n="desc_mouse_companion_probe">Use test-dispatch API to verify move/scroll/hover/hold/click action transitions.</div>
        <div class="mouse-companion-probe__grid">
          <label for="mc_probe_x" data-i18n="label_mouse_companion_probe_x">Probe X</label>
          <input id="mc_probe_x" type="number" />
          <label for="mc_probe_y" data-i18n="label_mouse_companion_probe_y">Probe Y</label>
          <input id="mc_probe_y" type="number" />
          <label for="mc_probe_button" data-i18n="label_mouse_companion_probe_button">Probe Button (1-3)</label>
          <input id="mc_probe_button" type="number" min="1" max="3" step="1" />
          <label for="mc_probe_delta" data-i18n="label_mouse_companion_probe_delta">Scroll Delta</label>
          <input id="mc_probe_delta" type="number" step="1" />
          <label for="mc_probe_hold_ms" data-i18n="label_mouse_companion_probe_hold_ms">Hold Duration (ms)</label>
          <input id="mc_probe_hold_ms" type="number" min="0" step="10" />
        </div>
        <div class="mouse-companion-probe__actions">
          <button id="mc_probe_status" type="button" class="btn-soft" data-i18n="btn_mouse_companion_probe_status">Status</button>
          <button id="mc_probe_move" type="button" class="btn-soft" data-i18n="btn_mouse_companion_probe_move">Move</button>
          <button id="mc_probe_scroll" type="button" class="btn-soft" data-i18n="btn_mouse_companion_probe_scroll">Scroll</button>
          <button id="mc_probe_hover_start" type="button" class="btn-soft" data-i18n="btn_mouse_companion_probe_hover_start">Hover Start</button>
          <button id="mc_probe_hover_end" type="button" class="btn-soft" data-i18n="btn_mouse_companion_probe_hover_end">Hover End</button>
          <button id="mc_probe_hold_start" type="button" class="btn-soft" data-i18n="btn_mouse_companion_probe_hold_start">Hold Start</button>
          <button id="mc_probe_hold_update" type="button" class="btn-soft" data-i18n="btn_mouse_companion_probe_hold_update">Hold Update</button>
          <button id="mc_probe_hold_end" type="button" class="btn-soft" data-i18n="btn_mouse_companion_probe_hold_end">Hold End</button>
          <button id="mc_probe_button_down" type="button" class="btn-soft" data-i18n="btn_mouse_companion_probe_button_down">Button Down</button>
          <button id="mc_probe_button_up" type="button" class="btn-soft" data-i18n="btn_mouse_companion_probe_button_up">Button Up</button>
          <button id="mc_probe_click" type="button" class="btn-soft" data-i18n="btn_mouse_companion_probe_click">Click</button>
          <button id="mc_probe_sequence" type="button" class="btn-soft" data-i18n="btn_mouse_companion_probe_sequence">Run Sequence</button>
        </div>
        <div class="mouse-companion-probe__result-line">
          <span class="mouse-companion-probe__result-label" data-i18n="label_mouse_companion_probe_result">Probe Result</span>
          <span id="mc_probe_result" class="mouse-companion-probe__result">-</span>
        </div>
      </div>
    </section>
    <section id="mc_panel_runtime" class="mouse-companion-panel" role="tabpanel" aria-labelledby="mc_tab_runtime" hidden>
      <div class="mouse-companion-runtime">
        <div class="mouse-companion-runtime__title" data-i18n="title_mouse_companion_runtime">Runtime Diagnostics</div>
        <div class="mouse-companion-runtime__grid">
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_runtime_present">Runtime Present</div>
          <div id="mc_runtime_runtime_present" class="mouse-companion-runtime__value">No</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_visual_host_active">Visual Host Active</div>
          <div id="mc_runtime_visual_host_active" class="mouse-companion-runtime__value">No</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_model_loaded">Runtime Model Loaded</div>
          <div id="mc_runtime_model_loaded" class="mouse-companion-runtime__value">No</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_visual_model_loaded">Visual Model Loaded</div>
          <div id="mc_runtime_visual_model_loaded" class="mouse-companion-runtime__value">No</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_action_library_loaded">Action Library Loaded</div>
          <div id="mc_runtime_action_library_loaded" class="mouse-companion-runtime__value">No</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_appearance_profile_loaded">Appearance Loaded</div>
          <div id="mc_runtime_appearance_profile_loaded" class="mouse-companion-runtime__value">No</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_pose_binding_configured">Pose Binding</div>
          <div id="mc_runtime_pose_binding_configured" class="mouse-companion-runtime__value">No</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_skeleton_bone_count">Skeleton Bones</div>
          <div id="mc_runtime_skeleton_bone_count" class="mouse-companion-runtime__value">0</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_last_action_name">Last Action</div>
          <div id="mc_runtime_last_action_name" class="mouse-companion-runtime__value">-</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_last_action_code">Last Action Code</div>
          <div id="mc_runtime_last_action_code" class="mouse-companion-runtime__value">-1</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_last_action_intensity">Last Action Intensity</div>
          <div id="mc_runtime_last_action_intensity" class="mouse-companion-runtime__value">0.000</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_last_action_tick_ms">Last Action Tick (ms)</div>
          <div id="mc_runtime_last_action_tick_ms" class="mouse-companion-runtime__value">0</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_runtime_click_streak">Click Streak</div>
          <div id="mc_runtime_click_streak" class="mouse-companion-runtime__value">0</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_runtime_head_tint_amount">Head Tint Amount</div>
          <div id="mc_runtime_head_tint_amount" class="mouse-companion-runtime__value">0.000</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_runtime_click_streak_break_ms">Click Break (ms)</div>
          <div id="mc_runtime_click_streak_break_ms" class="mouse-companion-runtime__value">650</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_runtime_head_tint_decay_per_second">Head Tint Decay (/s)</div>
          <div id="mc_runtime_head_tint_decay_per_second" class="mouse-companion-runtime__value">0.360</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_action_coverage_ready">Action Coverage Ready</div>
          <div id="mc_runtime_action_coverage_ready" class="mouse-companion-runtime__value">No</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_action_coverage_ratio">Action Coverage Ratio</div>
          <div id="mc_runtime_action_coverage_ratio" class="mouse-companion-runtime__value">0.0%</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_action_coverage_tracks">Mapped Tracks</div>
          <div id="mc_runtime_action_coverage_tracks" class="mouse-companion-runtime__value">0/0</div>
        </div>
        <div class="mouse-companion-runtime__item">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_action_coverage_actions">Covered Actions</div>
          <div id="mc_runtime_action_coverage_actions" class="mouse-companion-runtime__value">0/0</div>
        </div>
        <div class="mouse-companion-runtime__item mouse-companion-runtime__item--wide">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_action_coverage_action_details">Action Coverage Details</div>
          <div id="mc_runtime_action_coverage_action_details" class="mouse-companion-runtime__value mouse-companion-runtime__value--path">-</div>
        </div>
        <div class="mouse-companion-runtime__item mouse-companion-runtime__item--wide">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_loaded_model_path">Runtime Model Path</div>
          <div id="mc_runtime_loaded_model_path" class="mouse-companion-runtime__value mouse-companion-runtime__value--path">-</div>
        </div>
        <div class="mouse-companion-runtime__item mouse-companion-runtime__item--wide">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_visual_model_path">Visual Model Path</div>
          <div id="mc_runtime_visual_model_path" class="mouse-companion-runtime__value mouse-companion-runtime__value--path">-</div>
        </div>
        <div class="mouse-companion-runtime__item mouse-companion-runtime__item--wide">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_model_load_error">Runtime Model Error</div>
          <div id="mc_runtime_model_load_error" class="mouse-companion-runtime__value mouse-companion-runtime__value--path">-</div>
        </div>
        <div class="mouse-companion-runtime__item mouse-companion-runtime__item--wide">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_visual_model_load_error">Visual Model Error</div>
          <div id="mc_runtime_visual_model_load_error" class="mouse-companion-runtime__value mouse-companion-runtime__value--path">-</div>
        </div>
        <div class="mouse-companion-runtime__item mouse-companion-runtime__item--wide">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_action_coverage_missing_actions">Missing Actions</div>
          <div id="mc_runtime_action_coverage_missing_actions" class="mouse-companion-runtime__value mouse-companion-runtime__value--path">-</div>
        </div>
        <div class="mouse-companion-runtime__item mouse-companion-runtime__item--wide">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_action_coverage_missing_bones">Missing Bone Tracks</div>
          <div id="mc_runtime_action_coverage_missing_bones" class="mouse-companion-runtime__value mouse-companion-runtime__value--path">-</div>
        </div>
        <div class="mouse-companion-runtime__item mouse-companion-runtime__item--wide">
          <div class="mouse-companion-runtime__label" data-i18n="label_mouse_companion_action_coverage_error">Action Coverage Error</div>
          <div id="mc_runtime_action_coverage_error" class="mouse-companion-runtime__value mouse-companion-runtime__value--path">-</div>
        </div>
      </div>
      </div>
    </section>
  `;
}
