const PLATFORM_WINDOWS = 'windows';
const PLATFORM_MACOS = 'macos';
const PLATFORM_LINUX = 'linux';

function asText(value) {
  return `${value || ''}`.trim().toLowerCase();
}

function canonicalPlatform(value) {
  const text = asText(value);
  if (!text) {
    return '';
  }

  if (
    text === PLATFORM_WINDOWS ||
    text.startsWith('win')
  ) {
    return PLATFORM_WINDOWS;
  }

  if (
    text === PLATFORM_MACOS ||
    text === 'mac' ||
    text === 'darwin' ||
    text === 'osx' ||
    text === 'macosx' ||
    text.includes('mac') ||
    text.includes('darwin')
  ) {
    return PLATFORM_MACOS;
  }

  if (
    text === PLATFORM_LINUX ||
    text.includes('linux') ||
    text.includes('x11')
  ) {
    return PLATFORM_LINUX;
  }

  return '';
}

function detectBrowserPlatform() {
  if (typeof navigator === 'undefined') {
    return PLATFORM_WINDOWS;
  }

  const candidates = [
    navigator.userAgentData?.platform,
    navigator.platform,
    navigator.userAgent,
  ];

  for (const candidate of candidates) {
    const matched = canonicalPlatform(candidate);
    if (matched) {
      return matched;
    }
  }

  return PLATFORM_WINDOWS;
}

export function normalizeRuntimePlatform(value) {
  return canonicalPlatform(value) || detectBrowserPlatform();
}

export function isWindowsPlatform(value) {
  return normalizeRuntimePlatform(value) === PLATFORM_WINDOWS;
}

export function isMacosPlatform(value) {
  return normalizeRuntimePlatform(value) === PLATFORM_MACOS;
}

export function defaultProcessSuffix(platform) {
  if (isMacosPlatform(platform)) {
    return 'app';
  }
  if (isWindowsPlatform(platform)) {
    return 'exe';
  }
  return '';
}

export {
  PLATFORM_WINDOWS,
  PLATFORM_MACOS,
  PLATFORM_LINUX,
};
