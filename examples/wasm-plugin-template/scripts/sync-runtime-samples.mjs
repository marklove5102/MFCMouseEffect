import { spawnSync } from "node:child_process";
import { cpSync, existsSync, mkdirSync, readFileSync, readdirSync, rmSync } from "node:fs";
import { homedir, platform } from "node:os";
import { resolve } from "node:path";

import {
  loadTemplateManifest,
  parseArgs,
  resolveTemplateRoot,
} from "./build-lib.mjs";
import { SAMPLE_PRESETS } from "./sample-presets.mjs";

function parseBoolFlag(value) {
  const normalized = `${value || ""}`.trim().toLowerCase();
  return normalized === "1" || normalized === "true" || normalized === "yes" || normalized === "on";
}

function resolveDefaultRuntimeRoot() {
  const homeDir = homedir();
  const currentPlatform = platform();
  if (currentPlatform === "darwin") {
    return resolve(homeDir, "Library", "Application Support", "MFCMouseEffect", "plugins", "wasm");
  }
  if (currentPlatform === "win32") {
    const appData = process.env.APPDATA && process.env.APPDATA.trim().length > 0
      ? process.env.APPDATA
      : resolve(homeDir, "AppData", "Roaming");
    return resolve(appData, "MFCMouseEffect", "plugins", "wasm");
  }

  const xdgConfigHome = process.env.XDG_CONFIG_HOME && process.env.XDG_CONFIG_HOME.trim().length > 0
    ? process.env.XDG_CONFIG_HOME
    : resolve(homeDir, ".config");
  return resolve(xdgConfigHome, "MFCMouseEffect", "plugins", "wasm");
}

function runBuildScript(rootDir, scriptName) {
  const scriptPath = resolve(rootDir, "scripts", scriptName);
  const result = spawnSync(process.execPath, [scriptPath], {
    cwd: rootDir,
    stdio: "inherit",
  });
  if (result.status !== 0) {
    throw new Error(`Runtime sample sync build step failed: ${scriptName}`);
  }
}

function readManifestId(pluginDir) {
  const manifestPath = resolve(pluginDir, "plugin.json");
  if (!existsSync(manifestPath)) {
    return "";
  }
  try {
    const manifest = JSON.parse(readFileSync(manifestPath, "utf8"));
    return `${manifest?.id || ""}`.trim();
  } catch {
    return "";
  }
}

function ensureDirectory(dirPath) {
  mkdirSync(dirPath, { recursive: true });
}

function replaceDirectory(sourceDir, targetDir) {
  rmSync(targetDir, { recursive: true, force: true });
  ensureDirectory(resolve(targetDir, ".."));
  cpSync(sourceDir, targetDir, { recursive: true });
}

function deriveLegacyIds(currentIds) {
  const out = new Set();
  for (const id of currentIds) {
    if (!id.endsWith(".v2")) {
      continue;
    }
    out.add(`${id.slice(0, -3)}.v1`);
  }
  return out;
}

function collectDesiredBundles(rootDir) {
  const desiredBundles = [];
  const distDir = resolve(rootDir, "dist");
  const templateManifest = loadTemplateManifest(rootDir);
  desiredBundles.push({
    id: `${templateManifest.id || ""}`.trim(),
    sourceDir: distDir,
    label: "template-default",
  });

  for (const preset of SAMPLE_PRESETS) {
    desiredBundles.push({
      id: `${preset.id || ""}`.trim(),
      sourceDir: resolve(distDir, "samples", preset.key),
      label: preset.key,
    });
  }

  for (const bundle of desiredBundles) {
    if (!bundle.id) {
      throw new Error(`Runtime sample sync found empty plugin id for bundle "${bundle.label}".`);
    }
    const wasmPath = resolve(bundle.sourceDir, "effect.wasm");
    const manifestPath = resolve(bundle.sourceDir, "plugin.json");
    if (!existsSync(wasmPath) || !existsSync(manifestPath)) {
      throw new Error(
        `Runtime sample sync missing built bundle "${bundle.label}" at ${bundle.sourceDir}. ` +
        `Run the build step first or omit --skip-build.`
      );
    }
  }
  return desiredBundles;
}

function collectManagedIds(desiredBundles) {
  const currentIds = new Set(desiredBundles.map((bundle) => bundle.id));
  const legacyIds = deriveLegacyIds(currentIds);
  return new Set([...currentIds, ...legacyIds]);
}

function syncRuntimeRoot(runtimeRoot, desiredBundles) {
  ensureDirectory(runtimeRoot);

  const desiredById = new Map(desiredBundles.map((bundle) => [bundle.id, bundle]));
  const managedIds = collectManagedIds(desiredBundles);
  const removedDirs = [];
  const keptCustomDirs = [];

  for (const entry of readdirSync(runtimeRoot, { withFileTypes: true })) {
    if (!entry.isDirectory()) {
      continue;
    }
    const entryPath = resolve(runtimeRoot, entry.name);
    const manifestId = readManifestId(entryPath);
    const managedId = managedIds.has(manifestId)
      ? manifestId
      : (managedIds.has(entry.name) ? entry.name : "");
    if (!managedId) {
      keptCustomDirs.push(entry.name);
      continue;
    }
    rmSync(entryPath, { recursive: true, force: true });
    removedDirs.push(managedId);
  }

  const syncedIds = [];
  for (const [id, bundle] of desiredById.entries()) {
    const targetDir = resolve(runtimeRoot, id);
    replaceDirectory(bundle.sourceDir, targetDir);
    syncedIds.push(id);
  }

  return {
    runtimeRoot,
    removedDirs,
    keptCustomDirs,
    syncedIds,
  };
}

function printSummary(summary) {
  console.log(`Runtime sample sync root: ${summary.runtimeRoot}`);
  console.log(`Removed managed dirs: ${summary.removedDirs.length}`);
  for (const id of summary.removedDirs.sort()) {
    console.log(`  - ${id}`);
  }
  console.log(`Synced official bundles: ${summary.syncedIds.length}`);
  for (const id of summary.syncedIds.sort()) {
    console.log(`  + ${id}`);
  }
  console.log(`Preserved custom dirs: ${summary.keptCustomDirs.length}`);
  for (const name of summary.keptCustomDirs.sort()) {
    console.log(`  = ${name}`);
  }
}

const rootDir = resolveTemplateRoot(import.meta.url);
const args = parseArgs(process.argv.slice(2));
const runtimeRoot = `${args["runtime-root"] || ""}`.trim() || resolveDefaultRuntimeRoot();
const skipBuild = parseBoolFlag(args["skip-build"]);

try {
  if (!skipBuild) {
    runBuildScript(rootDir, "build.mjs");
    runBuildScript(rootDir, "build-all-samples.mjs");
  }

  const desiredBundles = collectDesiredBundles(rootDir);
  const summary = syncRuntimeRoot(runtimeRoot, desiredBundles);
  printSummary(summary);
} catch (error) {
  console.error(error?.message || error);
  process.exit(1);
}
