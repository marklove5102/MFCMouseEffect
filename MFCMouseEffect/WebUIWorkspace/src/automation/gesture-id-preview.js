function asText(value) {
  return `${value || ''}`.trim().toLowerCase();
}

function asFiniteNumber(value, fallback) {
  const parsed = Number(value);
  return Number.isFinite(parsed) ? parsed : fallback;
}

function clamp(value, min, max) {
  return Math.min(max, Math.max(min, value));
}

const DIRECTION_VECTORS = {
  left: [-1, 0],
  right: [1, 0],
  up: [0, -1],
  down: [0, 1],
  diag_up_left: [-1, -1],
  diag_up_right: [1, -1],
  diag_down_left: [-1, 1],
  diag_down_right: [1, 1],
};

const DIRECTION_ARROWS = {
  left: '←',
  right: '→',
  up: '↑',
  down: '↓',
  diag_up_left: '↖',
  diag_up_right: '↗',
  diag_down_left: '↙',
  diag_down_right: '↘',
};

const GESTURE_ALIASES = {
  line_right: 'right',
  line_left: 'left',
  line_up: 'up',
  line_down: 'down',
  hline: 'right',
  vline: 'down',
  slash: 'diag_down_right',
  backslash: 'diag_down_left',
  v: 'diag_down_right_diag_up_right',
  w: 'diag_down_right_diag_up_right_diag_down_right',
};

function normalizeGestureId(value) {
  const text = asText(value).replace(/[\s-]+/g, '_');
  return GESTURE_ALIASES[text] || text;
}

export function parseGestureDirections(gestureId) {
  const normalized = normalizeGestureId(gestureId);
  if (!normalized) {
    return [];
  }
  const parts = normalized.split('_').filter((part) => !!part);
  const directions = [];

  for (let index = 0; index < parts.length;) {
    if (parts[index] === 'diag' && index + 2 < parts.length) {
      const diagonal = `diag_${parts[index + 1]}_${parts[index + 2]}`;
      if (DIRECTION_VECTORS[diagonal]) {
        directions.push(diagonal);
        index += 3;
        continue;
      }
    }

    const single = parts[index];
    if (DIRECTION_VECTORS[single]) {
      directions.push(single);
      index += 1;
      continue;
    }

    return [];
  }

  return directions;
}

export function gestureIdDirectionHint(gestureId) {
  const directions = parseGestureDirections(gestureId);
  if (directions.length <= 0) {
    return '';
  }
  return directions.map((direction) => DIRECTION_ARROWS[direction]).join('');
}

function buildRawPoints(directions) {
  let x = 0;
  let y = 0;
  const points = [{ x, y }];
  for (const direction of directions) {
    const vector = DIRECTION_VECTORS[direction];
    if (!vector) {
      continue;
    }
    x += vector[0];
    y += vector[1];
    points.push({ x, y });
  }
  return points;
}

function fitPoints(points, width, height, padding, fitRatio) {
  if (!Array.isArray(points) || points.length <= 0) {
    return [];
  }

  let minX = points[0].x;
  let maxX = points[0].x;
  let minY = points[0].y;
  let maxY = points[0].y;
  for (const point of points) {
    minX = Math.min(minX, point.x);
    maxX = Math.max(maxX, point.x);
    minY = Math.min(minY, point.y);
    maxY = Math.max(maxY, point.y);
  }

  const spanX = Math.max(1, maxX - minX);
  const spanY = Math.max(1, maxY - minY);
  const drawWidth = Math.max(1, width - padding * 2);
  const drawHeight = Math.max(1, height - padding * 2);
  const scale = Math.min(drawWidth / spanX, drawHeight / spanY) * fitRatio;
  const centerX = (minX + maxX) / 2;
  const centerY = (minY + maxY) / 2;

  return points.map((point) => ({
    x: clamp(width / 2 + ((point.x - centerX) * scale), padding, width - padding),
    y: clamp(height / 2 + ((point.y - centerY) * scale), padding, height - padding),
  }));
}

function svgPathFromPoints(points) {
  if (!Array.isArray(points) || points.length <= 0) {
    return '';
  }
  return points.map((point, index) => (
    `${index === 0 ? 'M' : 'L'} ${point.x.toFixed(2)} ${point.y.toFixed(2)}`
  )).join(' ');
}

function endArrowPath(points, length = 8.2, width = 6.8) {
  if (!Array.isArray(points) || points.length < 2) {
    return '';
  }

  const tip = points[points.length - 1];
  let from = null;
  for (let index = points.length - 2; index >= 0; index -= 1) {
    const candidate = points[index];
    if (candidate.x !== tip.x || candidate.y !== tip.y) {
      from = candidate;
      break;
    }
  }
  if (!from) {
    return '';
  }

  const dx = tip.x - from.x;
  const dy = tip.y - from.y;
  const vectorLength = Math.hypot(dx, dy);
  if (vectorLength < 0.01) {
    return '';
  }

  const ux = dx / vectorLength;
  const uy = dy / vectorLength;
  const nx = -uy;
  const ny = ux;
  const baseX = tip.x - (ux * length);
  const baseY = tip.y - (uy * length);
  const halfWidth = width / 2;
  const leftX = baseX + (nx * halfWidth);
  const leftY = baseY + (ny * halfWidth);
  const rightX = baseX - (nx * halfWidth);
  const rightY = baseY - (ny * halfWidth);
  return `M ${leftX.toFixed(2)} ${leftY.toFixed(2)} L ${tip.x.toFixed(2)} ${tip.y.toFixed(2)} L ${rightX.toFixed(2)} ${rightY.toFixed(2)}`;
}

export function buildGesturePreviewFromId(gestureId, options = {}) {
  const directions = parseGestureDirections(gestureId);
  if (directions.length <= 0) {
    return null;
  }

  const width = Math.max(40, asFiniteNumber(options.width, 88));
  const height = Math.max(24, asFiniteNumber(options.height, 40));
  const padding = clamp(asFiniteNumber(options.padding, 4), 2, Math.min(width, height) / 3);
  const fitRatio = clamp(asFiniteNumber(options.fitRatio, 0.82), 0.45, 1);

  const rawPoints = buildRawPoints(directions);
  const points = fitPoints(rawPoints, width, height, padding, fitRatio);
  if (points.length <= 0) {
    return null;
  }

  return {
    width,
    height,
    directions,
    directionHint: directions.map((direction) => DIRECTION_ARROWS[direction]).join(''),
    path: svgPathFromPoints(points),
    startPoint: points[0],
    arrowPath: endArrowPath(points),
  };
}

