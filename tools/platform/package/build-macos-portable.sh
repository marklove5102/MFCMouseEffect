#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "$script_dir/../../.." && pwd)"

usage() {
    cat <<'EOF'
Usage:
  tools/platform/package/build-macos-portable.sh [options]

Options:
  --build-dir <path>      Build directory containing mfx_entry_posix_host (default: build-macos)
  --output-dir <path>     Output directory for packaged folder/zip (default: Install/macos)
  --package-name <name>   Package folder base name (default: MFCMouseEffect-macos-arm64-portable)
  --skip-build            Skip rebuilding mfx_entry_posix_host
  --skip-webui-build      Skip rebuilding WebUIWorkspace assets
  --no-zip                Do not create the final zip archive
  --no-dmg                Do not create the final dmg image
  -h, --help              Show this help
EOF
}

build_dir="$repo_root/build-macos"
output_dir="$repo_root/Install/macos"
package_name="MFCMouseEffect-macos-arm64-portable"
app_name="MFCMouseEffect.app"
skip_build=0
skip_webui_build=0
create_zip=1
create_dmg=1

while [[ $# -gt 0 ]]; do
    case "$1" in
    --build-dir)
        [[ $# -ge 2 ]] || { echo "missing value for --build-dir" >&2; exit 1; }
        build_dir="$2"
        shift 2
        ;;
    --output-dir)
        [[ $# -ge 2 ]] || { echo "missing value for --output-dir" >&2; exit 1; }
        output_dir="$2"
        shift 2
        ;;
    --package-name)
        [[ $# -ge 2 ]] || { echo "missing value for --package-name" >&2; exit 1; }
        package_name="$2"
        shift 2
        ;;
    --skip-build)
        skip_build=1
        shift
        ;;
    --skip-webui-build)
        skip_webui_build=1
        shift
        ;;
    --no-zip)
        create_zip=0
        shift
        ;;
    --no-dmg)
        create_dmg=0
        shift
        ;;
    -h|--help)
        usage
        exit 0
        ;;
    *)
        echo "unknown argument: $1" >&2
        exit 1
        ;;
    esac
done

if [[ "$OSTYPE" != darwin* ]]; then
    echo "this packaging script is macOS-only" >&2
    exit 1
fi

host_bin="$build_dir/mfx_entry_posix_host"
webui_workspace="$repo_root/MFCMouseEffect/WebUIWorkspace"
webui_dir="$repo_root/MFCMouseEffect/WebUI"
assets_dir="$repo_root/MFCMouseEffect/Assets"
pet_source_dir="$assets_dir/Pet3D/source"
plugin_dist_dir="$repo_root/examples/wasm-plugin-template/dist"
plugin_target_name="demo.template.default.v2"

if [[ "$skip_build" -ne 1 ]]; then
    cmake --build "$build_dir" --target mfx_entry_posix_host -j8
fi

if [[ "$skip_webui_build" -ne 1 ]]; then
    (
        cd "$webui_workspace"
        pnpm run build:mouse-companion
        node scripts/copy-output.mjs
    )
fi

[[ -x "$host_bin" ]] || { echo "host binary missing: $host_bin" >&2; exit 1; }
[[ -f "$webui_dir/index.html" ]] || { echo "WebUI assets missing: $webui_dir/index.html" >&2; exit 1; }
[[ -f "$pet_source_dir/pet-main.usdz" ]] || { echo "pet asset missing: $pet_source_dir/pet-main.usdz" >&2; exit 1; }
[[ -f "$pet_source_dir/pet-actions.json" ]] || { echo "pet action library missing: $pet_source_dir/pet-actions.json" >&2; exit 1; }
[[ -f "$pet_source_dir/pet-appearance.json" ]] || { echo "pet appearance profile missing: $pet_source_dir/pet-appearance.json" >&2; exit 1; }
[[ -f "$pet_source_dir/pet-effects.json" ]] || { echo "pet effects profile missing: $pet_source_dir/pet-effects.json" >&2; exit 1; }
[[ -f "$plugin_dist_dir/plugin.json" ]] || { echo "sample plugin manifest missing: $plugin_dist_dir/plugin.json" >&2; exit 1; }
[[ -f "$plugin_dist_dir/effect.wasm" ]] || { echo "sample plugin wasm missing: $plugin_dist_dir/effect.wasm" >&2; exit 1; }

mkdir -p "$output_dir"
package_root="$output_dir/$package_name"
rm -rf "$package_root"
app_root="$package_root/$app_name"
app_contents="$app_root/Contents"
app_macos_dir="$app_contents/MacOS"
app_resources_dir="$app_contents/Resources"
app_runtime_root="$app_resources_dir/MFCMouseEffect"
mkdir -p \
    "$app_macos_dir/plugins/wasm" \
    "$app_runtime_root/Assets/Pet3D/source" \
    "$app_runtime_root"

launcher_src="$(mktemp "$output_dir/.mfx-launcher.XXXXXX.swift")"
cleanup_launcher_src() {
    rm -f "$launcher_src"
}
trap cleanup_launcher_src EXIT

cp "$host_bin" "$app_macos_dir/mfx_entry_posix_host"
chmod +x "$app_macos_dir/mfx_entry_posix_host"

cp -R "$webui_dir" "$app_runtime_root/WebUI"
cp "$pet_source_dir/pet-main.usdz" "$app_runtime_root/Assets/Pet3D/source/pet-main.usdz"
cp "$pet_source_dir/pet-actions.json" "$app_runtime_root/Assets/Pet3D/source/pet-actions.json"
cp "$pet_source_dir/pet-appearance.json" "$app_runtime_root/Assets/Pet3D/source/pet-appearance.json"
cp "$pet_source_dir/pet-effects.json" "$app_runtime_root/Assets/Pet3D/source/pet-effects.json"
mkdir -p "$app_macos_dir/plugins/wasm/$plugin_target_name"
cp "$plugin_dist_dir/plugin.json" "$app_macos_dir/plugins/wasm/$plugin_target_name/plugin.json"
cp "$plugin_dist_dir/effect.wasm" "$app_macos_dir/plugins/wasm/$plugin_target_name/effect.wasm"
mkdir -p "$app_runtime_root/theme-catalog"

cat > "$launcher_src" <<'EOF'
import Foundation

let executableURL = URL(fileURLWithPath: CommandLine.arguments[0]).standardizedFileURL
let macosURL = executableURL.deletingLastPathComponent()
let contentsURL = macosURL.deletingLastPathComponent()
let resourcesURL = contentsURL.appendingPathComponent("Resources", isDirectory: true)
let runtimeRootURL = resourcesURL.appendingPathComponent("MFCMouseEffect", isDirectory: true)
let webUIPath = runtimeRootURL.appendingPathComponent("WebUI", isDirectory: true).path
let hostPath = macosURL.appendingPathComponent("mfx_entry_posix_host").path

setenv("MFX_WEBUI_DIR", webUIPath, 1)
setenv("MFX_SCAFFOLD_WEBUI_DIR", webUIPath, 1)
_ = FileManager.default.changeCurrentDirectoryPath(resourcesURL.path)

var execArgs = [hostPath, "--mode=tray"]
execArgs.append(contentsOf: CommandLine.arguments.dropFirst())

var cArgs = execArgs.map { strdup($0) }
cArgs.append(nil)
cArgs.withUnsafeMutableBufferPointer { buffer in
    guard let baseAddress = buffer.baseAddress else {
        fputs("failed to build launcher argv\n", stderr)
        exit(EXIT_FAILURE)
    }
    execv(hostPath, baseAddress)
}
perror("execv")
exit(EXIT_FAILURE)
EOF
xcrun swiftc -O -o "$app_macos_dir/MFCMouseEffect" "$launcher_src"
chmod +x "$app_macos_dir/MFCMouseEffect"

cat > "$app_contents/Info.plist" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleDisplayName</key>
    <string>MFCMouseEffect</string>
    <key>CFBundleExecutable</key>
    <string>MFCMouseEffect</string>
    <key>CFBundleIdentifier</key>
    <string>com.mousefx.mfcmouseeffect</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>MFCMouseEffect</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>0.1.0</string>
    <key>CFBundleVersion</key>
    <string>0.1.0</string>
    <key>LSMinimumSystemVersion</key>
    <string>13.0</string>
    <key>LSUIElement</key>
    <true/>
</dict>
</plist>
EOF

zip_path="$output_dir/$package_name.zip"
if [[ "$create_zip" -eq 1 ]]; then
    rm -f "$zip_path"
    (
        cd "$output_dir"
        ditto -c -k --sequesterRsrc --keepParent "$package_name" "$zip_path"
    )
fi

dmg_path="$output_dir/$package_name.dmg"
if [[ "$create_dmg" -eq 1 ]]; then
    rm -f "$dmg_path"
    dmg_stage_dir="$(mktemp -d "$output_dir/.dmg-stage.XXXXXX")"
    trap 'rm -rf "$dmg_stage_dir"' EXIT
    cp -R "$app_root" "$dmg_stage_dir/$app_name"
    ln -s /Applications "$dmg_stage_dir/Applications"
    hdiutil create \
        -volname "$package_name" \
        -srcfolder "$dmg_stage_dir" \
        -format UDZO \
        "$dmg_path" >/dev/null
    rm -rf "$dmg_stage_dir"
    trap - EXIT
fi

cleanup_launcher_src
trap - EXIT

printf 'package_root=%s\n' "$package_root"
if [[ "$create_zip" -eq 1 ]]; then
    printf 'zip_path=%s\n' "$zip_path"
fi
if [[ "$create_dmg" -eq 1 ]]; then
    printf 'dmg_path=%s\n' "$dmg_path"
fi
