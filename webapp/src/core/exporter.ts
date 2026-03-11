import {
  PartInfo,
  FiducialInfo,
  ExportConfig,
  ExportFile,
  C_NONE,
  C_SEQUENCE,
  C_DESIGNATOR,
  C_VALUE,
  C_PACKAGE,
  C_PRODUCTNR,
  C_X,
  C_Y,
  C_ROTATION,
  C_SIDE,
  C_STAGE,
  C_MOUNT,
  SideSelection,
  SeparatorType,
  UnitType,
  AngleRange,
  ZeroDirection,
} from '../types.js';

const UNIT_NAMES = ['mm', 'inch'];

/**
 * Main export function — pure function that receives parsed parts + config + fiducials
 * and returns CSV files. Reimplements the logic of vp_WriteCentroid from csvexport.c.
 *
 * @param parts     Parsed components from Altium CSV
 * @param config    Export configuration (template settings)
 * @param fiducials Fiducial list with type (origin/reference) — provided by the UI
 * @param baseFilename Base name for output files
 */
export function exportCsv(
  parts: PartInfo[],
  config: ExportConfig,
  fiducials: FiducialInfo[],
  baseFilename: string = 'output'
): ExportFile[] {
  const results: ExportFile[] = [];
  const separator = getSeparatorChar(config.separator);

  const numColumns = countColumns(config.columns);
  const hasMountColumn = config.columns.some(c => c === C_MOUNT);

  // Sort components before separating by side
  const sortedParts = sortComponents(parts.filter(p => !p.isFiducial));

  const topParts = sortedParts.filter(p => p.side === 'Top');
  const bottomParts = sortedParts.filter(p => p.side === 'Bottom');

  const topFiducials = fiducials.filter(f => f.side === 'Top');
  const bottomFiducials = fiducials.filter(f => f.side === 'Bottom');

  switch (config.side) {
    case SideSelection.BothSeparate: {
      if (topParts.length > 0 || topFiducials.length > 0) {
        const topContent = generateSideCsv(
          topParts, 'Top', config, numColumns, separator,
          hasMountColumn, topFiducials
        );
        results.push({ filename: `${baseFilename}_top.csv`, content: topContent, side: 'Top' });
      }
      if (bottomParts.length > 0 || bottomFiducials.length > 0) {
        const bottomContent = generateSideCsv(
          bottomParts, 'Bottom', config, numColumns, separator,
          hasMountColumn, bottomFiducials
        );
        results.push({ filename: `${baseFilename}_bottom.csv`, content: bottomContent, side: 'Bottom' });
      }
      break;
    }
    case SideSelection.BothCombined: {
      const allParts = [...topParts, ...bottomParts];
      const content = generateSideCsv(
        allParts, 'Combined', config, numColumns, separator,
        hasMountColumn, fiducials
      );
      results.push({ filename: `${baseFilename}.csv`, content, side: 'Combined' });
      break;
    }
    case SideSelection.TopOnly: {
      const topContent = generateSideCsv(
        topParts, 'Top', config, numColumns, separator,
        hasMountColumn, topFiducials
      );
      results.push({ filename: `${baseFilename}_top.csv`, content: topContent, side: 'Top' });
      break;
    }
    case SideSelection.BottomOnly: {
      const bottomContent = generateSideCsv(
        bottomParts, 'Bottom', config, numColumns, separator,
        hasMountColumn, bottomFiducials
      );
      results.push({ filename: `${baseFilename}_bottom.csv`, content: bottomContent, side: 'Bottom' });
      break;
    }
  }

  return results;
}

/**
 * Extracts fiducials from parsed parts with auto-detection of origin.
 * The fiducial with the largest X on each side is set as "origin" by default
 * (matching VisualPlace's behavior: nearest to bottom-left after mirroring).
 */
export function extractFiducialsFromParts(parts: PartInfo[]): FiducialInfo[] {
  const fiducials: FiducialInfo[] = [];
  let fidIndex = 1;

  for (const part of parts) {
    if (part.isFiducial) {
      fiducials.push({
        index: fidIndex++,
        x: part.x,
        y: part.y,
        side: part.side,
        type: 'reference',
      });
    }
  }

  return fiducials;
}

/**
 * Generates CSV content for one side (or combined).
 */
function generateSideCsv(
  parts: PartInfo[],
  side: 'Top' | 'Bottom' | 'Combined',
  config: ExportConfig,
  numColumns: number,
  separator: string,
  hasMountColumn: boolean,
  sideFiducials: FiducialInfo[]
): string {
  const lines: string[] = [];
  const unitMult = config.unit === UnitType.Inch ? 1.0 / 25.4 : 1.0;
  const unitName = UNIT_NAMES[config.unit];

  // Compute mirror reference and offset
  let offsx = 0;
  let offsy = 0;
  let mirrorX = 0;

  if (side === 'Bottom') {
    // Mirror X using fiducials or component extents
    if (sideFiducials.length > 0) {
      mirrorX = Math.max(...sideFiducials.map(f => f.x));
    } else {
      const allX = parts.map(p => p.x);
      if (allX.length > 0) mirrorX = Math.max(...allX);
    }

    if (config.alignFiducial) {
      // Align to origin fiducial
      const originFid = sideFiducials.find(f => f.type === 'origin');
      if (originFid) {
        offsx = 0; // After mirroring, origin fiducial X becomes 0
        offsy = originFid.y;
      } else if (sideFiducials.length > 0) {
        // No origin: DLL picks reference fiducial with smallest mirrored X
        // = largest original X for bottom side
        const refFid = sideFiducials.reduce((best, f) => f.x > best.x ? f : best, sideFiducials[0]);
        offsx = 0;
        offsy = refFid.y;
      } else {
        // No fiducials at all: positions relative to bottom-left corner
        const allY = parts.map(p => p.y);
        if (allY.length > 0) {
          offsx = 0;
          offsy = Math.min(...allY);
        }
      }
    } else {
      // Align OFF: positions relative to bottom-left corner
      const allY = parts.map(p => p.y);
      if (allY.length > 0) {
        offsx = 0;
        offsy = Math.min(...allY);
      }
    }
  } else if (side === 'Top') {
    if (config.alignFiducial) {
      const originFid = sideFiducials.find(f => f.type === 'origin');
      if (originFid) {
        offsx = originFid.x;
        offsy = originFid.y;
      } else if (sideFiducials.length > 0) {
        // No origin: DLL picks reference fiducial with smallest X for top
        const refFid = sideFiducials.reduce((best, f) => f.x < best.x ? f : best, sideFiducials[0]);
        offsx = refFid.x;
        offsy = refFid.y;
      }
      // No fiducials on top → no offset (pass through)
    }
    // Align OFF on top: no offset (coordinates pass through as-is)
  }

  // CSV header comments
  if (config.csvHeader) {
    const dirNames = ['top', 'left', 'bottom', 'right'];
    const sideStr = side === 'Combined' ? 'both' : side.toLowerCase();
    lines.push(`# side: ${sideStr}`);
    lines.push(`# stage: all`);
    lines.push(`# unit: ${config.unit === UnitType.Inch ? 'inch' : 'mm'}`);
    lines.push(`# rotation: degrees ${config.angleClockwise ? 'clockwise' : 'counter-clockwise'}`);
    lines.push(`# zero-orientation for non-polarized 2-pin parts: pin 1 at ${dirNames[config.zero1]}`);
    lines.push(`# zero-orientation for polarized 2-pin parts: pin 1 at ${dirNames[config.zero2]} (pin 1 is cathode or + pole)`);
    lines.push(`# zero-orientation for parts with more than 2 pins: pin 1 at ${dirNames[config.zero3]}`);
    lines.push('#');
  }

  // Column header line
  const headerFields: string[] = [];
  for (let i = 0; i < numColumns; i++) {
    let name = '';
    switch (config.columns[i]) {
      case C_NONE: break;
      case C_SEQUENCE: name = 'Row'; break;
      case C_DESIGNATOR: name = 'Designator'; break;
      case C_VALUE: name = 'Value'; break;
      case C_PACKAGE: name = 'Package'; break;
      case C_PRODUCTNR: name = 'ProductNr'; break;
      case C_X: name = `X(${unitName})`; break;
      case C_Y: name = `Y(${unitName})`; break;
      case C_ROTATION: name = 'Rotation'; break;
      case C_SIDE: name = 'Side'; break;
      case C_STAGE: name = 'Stage'; break;
      case C_MOUNT: name = 'Mount'; break;
    }
    headerFields.push(config.enquoteFields ? `"${name}"` : name);
  }
  lines.push(headerFields.join(separator));

  // Write component records
  let seq = 0;
  for (const part of parts) {
    const isNoMount = false;
    if (isNoMount && !hasMountColumn) continue;

    seq++;
    const row = formatPartRow(
      part, config, numColumns, seq, unitMult,
      offsx, offsy, mirrorX, side, isNoMount
    );
    lines.push(row.join(separator));
  }

  // Write fiducials (if include-fiducials is enabled)
  if (config.includeFiducials && sideFiducials.length > 0) {
    for (const fid of sideFiducials) {
      seq++;
      const row = formatFiducialRow(
        fid, fid.index, config, numColumns, seq, unitMult,
        offsx, offsy, mirrorX, side
      );
      lines.push(row.join(separator));
    }
  }

  return lines.join('\n') + '\n';
}

function formatPartRow(
  part: PartInfo,
  config: ExportConfig,
  numColumns: number,
  seq: number,
  unitMult: number,
  offsx: number,
  offsy: number,
  mirrorX: number,
  side: 'Top' | 'Bottom' | 'Combined',
  isNoMount: boolean
): string[] {
  const fields: string[] = [];

  for (let i = 0; i < numColumns; i++) {
    let value = '';

    switch (config.columns[i]) {
      case C_NONE: break;
      case C_SEQUENCE: value = String(seq); break;
      case C_DESIGNATOR: value = part.designator; break;
      case C_VALUE: value = normalizeValue(part.comment); break;
      case C_PACKAGE: value = part.footprint; break;
      case C_PRODUCTNR: value = ''; break;

      case C_X: {
        let x = part.x;
        if (part.side === 'Bottom' && mirrorX > 0) x = mirrorX - x;
        x = (x - offsx) * unitMult;
        value = formatCoordinate(x, config.unit);
        break;
      }

      case C_Y: {
        let y = part.y;
        if (config.invertY) y = -y;
        y = (y - offsy) * unitMult;
        value = formatCoordinate(y, config.unit);
        break;
      }

      case C_ROTATION:
        value = formatRotation(computeRotation(part, config));
        break;

      case C_SIDE:
        value = part.side === 'Top' ? 'Top' : 'Bottom';
        break;

      case C_STAGE: value = ''; break;

      case C_MOUNT:
        value = isNoMount ? 'N' : 'Y';
        break;
    }

    fields.push(config.enquoteFields ? `"${value}"` : value);
  }

  return fields;
}

/**
 * Formats a fiducial row.
 * Replicates the DLL's fiducial Y offset behavior (offsy in inches, not mm).
 */
function formatFiducialRow(
  fid: FiducialInfo,
  displayIndex: number,
  config: ExportConfig,
  numColumns: number,
  seq: number,
  unitMult: number,
  offsx: number,
  offsy: number,
  mirrorX: number,
  side: 'Top' | 'Bottom' | 'Combined'
): string[] {
  const fields: string[] = [];

  // DLL bug replication: for fiducials, offsy is applied in inches to mm values
  const fidOffsyMm = offsy / 25.4;

  for (let i = 0; i < numColumns; i++) {
    let value = '';

    switch (config.columns[i]) {
      case C_NONE:
      case C_PACKAGE:
      case C_PRODUCTNR:
      case C_STAGE:
        break;
      case C_ROTATION: break;
      case C_SEQUENCE: value = String(seq); break;
      case C_DESIGNATOR: value = `Fid${displayIndex}`; break;
      case C_VALUE: value = 'Fiducial'; break;

      case C_X: {
        let x = fid.x;
        if (fid.side === 'Bottom' && mirrorX > 0) x = mirrorX - x;
        x = (x - offsx) * unitMult;
        value = formatCoordinate(x, config.unit);
        break;
      }

      case C_Y: {
        let y = fid.y;
        if (config.invertY) y = -y;
        y = y * unitMult - fidOffsyMm;
        value = formatCoordinate(y, config.unit);
        break;
      }

      case C_SIDE:
        value = fid.side === 'Top' ? 'Top' : 'Bottom';
        break;

      case C_MOUNT: value = 'N'; break;
    }

    fields.push(config.enquoteFields ? `"${value}"` : value);
  }

  return fields;
}

function computeRotation(part: PartInfo, config: ExportConfig): number {
  let pos = part.rotation;

  const isSideways = isTwoPinComponent(part);

  let zeroSetting: ZeroDirection;
  let defaultZero: ZeroDirection;

  if (isSideways) {
    const prefix = part.designator[0].toUpperCase();
    if (prefix === 'C' || prefix === 'R' || prefix === 'L' || prefix === 'X') {
      zeroSetting = config.zero1;
    } else {
      zeroSetting = config.zero2;
    }
    defaultZero = ZeroDirection.Left;
  } else {
    zeroSetting = config.zero3;
    defaultZero = ZeroDirection.Up;
  }

  let val = zeroSetting;
  while (val !== defaultZero) {
    pos += 90;
    val = ((val + 1) % 4) as ZeroDirection;
  }

  if (config.angleClockwise) pos = 360.0 - pos;

  if (config.angleRange === AngleRange.Range0to360) {
    while (pos < -0.5) pos += 360.0;
    while (pos > 360.0 - 0.5) pos -= 360.0;
  } else {
    while (pos < -180.0 - 0.5) pos += 360.0;
    while (pos > 180.0 - 0.5) pos -= 360.0;
  }

  return pos;
}

function isTwoPinComponent(part: PartInfo): boolean {
  const prefix = part.designator[0]?.toUpperCase() || '';
  if (['R', 'C', 'L', 'X', 'D', 'F'].includes(prefix)) {
    const fp = part.footprint.toUpperCase();
    if (fp.includes('SOT') || fp.includes('QFP') || fp.includes('BGA') ||
        fp.includes('SOP') || fp.includes('SSOP') || fp.includes('DIP')) {
      return false;
    }
    return true;
  }
  return false;
}

function sortComponents(parts: PartInfo[]): PartInfo[] {
  return [...parts].sort((a, b) => {
    const prefA = extractDesignatorPrefix(a.designator);
    const prefB = extractDesignatorPrefix(b.designator);

    const priA = getPrefixPriority(prefA);
    const priB = getPrefixPriority(prefB);
    if (priA !== priB) return priA - priB;

    const valA = parseComponentValue(a.comment);
    const valB = parseComponentValue(b.comment);
    if (valA !== valB) return valA - valB;

    if (a.comment !== b.comment) return a.comment.localeCompare(b.comment);
    if (a.footprint !== b.footprint) return a.footprint.localeCompare(b.footprint);

    const numA = extractDesignatorNumber(a.designator);
    const numB = extractDesignatorNumber(b.designator);
    return numA - numB;
  });
}

function extractDesignatorPrefix(designator: string): string {
  const match = designator.match(/^([A-Za-z_]+)/);
  return match ? match[1].toUpperCase() : '';
}

function extractDesignatorNumber(designator: string): number {
  const match = designator.match(/(\d+)$/);
  return match ? parseInt(match[1], 10) : 0;
}

function getPrefixPriority(prefix: string): number {
  const order: Record<string, number> = {
    C: 1, R: 2, L: 3, X: 4, D: 5, Q: 6, U: 7, P: 8, S: 9,
  };
  if (prefix.length === 1) return order[prefix] ?? 999;
  return 10000;
}

function parseComponentValue(comment: string): number {
  const match = comment.match(/^(\d+\.?\d*)\s*([pnumkKMGR]?)(F|H|R|V|A|Hz|Ohm)?/i);
  if (!match) return Infinity;
  const num = parseFloat(match[1]);
  const prefix = match[2] || '';
  const multiplier: Record<string, number> = {
    'p': 1e-12, 'n': 1e-9, 'u': 1e-6, 'm': 1e-3,
    '': 1, 'R': 1, 'k': 1e3, 'K': 1e3, 'M': 1e6, 'G': 1e9,
  };
  return num * (multiplier[prefix] ?? 1);
}

function normalizeValue(comment: string): string {
  return comment.replace(/(\d)K$/i, '$1k');
}

function getSeparatorChar(sep: SeparatorType): string {
  switch (sep) {
    case SeparatorType.Comma: return ',';
    case SeparatorType.Semicolon: return ';';
    case SeparatorType.Tab: return '\t';
  }
}

function formatCoordinate(value: number, unit: UnitType): string {
  return unit === UnitType.Inch ? value.toFixed(3) : value.toFixed(2);
}

function formatRotation(value: number): string {
  return String(Math.round(value));
}

function countColumns(columns: number[]): number {
  let last = 0;
  for (let i = 0; i < columns.length; i++) {
    if (columns[i] > 0) last = i + 1;
  }
  return last;
}
