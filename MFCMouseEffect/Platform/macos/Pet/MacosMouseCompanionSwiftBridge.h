#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void* mfx_macos_mouse_companion_create_v1(int sizePx);
void mfx_macos_mouse_companion_release_v1(void* handle);
void mfx_macos_mouse_companion_show_v1(void* handle);
void mfx_macos_mouse_companion_hide_v1(void* handle);
int mfx_macos_mouse_companion_load_model_v1(void* handle, const char* modelPathUtf8);
int mfx_macos_mouse_companion_apply_appearance_v1(
    void* handle,
    const char* skinVariantIdUtf8,
    const char* const* enabledAccessoryIdsUtf8,
    int accessoryCount,
    const char* const* textureOverridePathsUtf8,
    int textureOverrideCount);
int mfx_macos_mouse_companion_configure_pose_binding_v1(
    void* handle,
    const char* const* boneNamesUtf8,
    int boneCount);
void mfx_macos_mouse_companion_configure_follow_profile_v1(
    void* handle,
    int offsetX,
    int offsetY,
    int pressLiftPx,
    int edgeClampModeCode);
void mfx_macos_mouse_companion_update_state_v1(
    void* handle,
    int cursorX,
    int cursorY,
    int actionCode,
    float actionIntensity,
    int skeletonBoneCount);
void mfx_macos_mouse_companion_apply_pose_v1(
    void* handle,
    const int* boneIndices,
    const float* positionsXYZ,
    const float* rotationsXYZW,
    const float* scalesXYZ,
    int poseCount);

#ifdef __cplusplus
}
#endif
