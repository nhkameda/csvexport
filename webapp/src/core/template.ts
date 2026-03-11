import {
  ExportConfig,
  Template,
  ZeroDirection,
  SideSelection,
  SeparatorType,
  UnitType,
  AngleRange,
} from '../types.js';

/**
 * Parses an INI file containing csvexport templates.
 * Sections follow the format [cpl:TEMPLATE_NAME].
 */
export function parseTemplateIni(iniContent: string): Template[] {
  const templates: Template[] = [];
  const lines = iniContent.replace(/\r\n/g, '\n').replace(/\r/g, '\n').split('\n');

  let currentTemplate: Template | null = null;

  for (const line of lines) {
    const trimmed = line.trim();
    if (!trimmed || trimmed.startsWith(';') || trimmed.startsWith('#')) continue;

    // Check for section header
    const sectionMatch = trimmed.match(/^\[cpl:(.+)\]$/i);
    if (sectionMatch) {
      if (currentTemplate) templates.push(currentTemplate);
      currentTemplate = {
        name: sectionMatch[1],
        config: getDefaultConfig(),
      };
      continue;
    }

    // Parse key=value
    if (currentTemplate) {
      const kvMatch = trimmed.match(/^([^=]+)=(.*)$/);
      if (kvMatch) {
        const key = kvMatch[1].trim();
        const value = kvMatch[2].trim();
        applyConfigValue(currentTemplate.config, key, value);
      }
    }
  }

  if (currentTemplate) templates.push(currentTemplate);
  return templates;
}

/**
 * Serializes a template to INI format.
 */
export function serializeTemplateIni(templates: Template[]): string {
  const lines: string[] = [];

  for (const template of templates) {
    const c = template.config;
    lines.push(`[cpl:${template.name}]`);

    for (let i = 0; i < 11; i++) {
      lines.push(`centroid${i + 1}=${c.columns[i]}`);
    }

    lines.push(`zero1=${c.zero1}`);
    lines.push(`zero2=${c.zero2}`);
    lines.push(`zero3=${c.zero3}`);
    lines.push(`side=${c.side}`);
    lines.push(`stage=0`);
    lines.push(`ext-centroid=CSV`);
    lines.push(`angle-range=${c.angleRange}`);
    lines.push(`angle-clockwise=${c.angleClockwise ? 1 : 0}`);
    lines.push(`separator=${c.separator}`);
    lines.push(`unit=${c.unit}`);
    lines.push(`enquote-fields=${c.enquoteFields ? 1 : 0}`);
    lines.push(`align-fiducial=${c.alignFiducial ? 1 : 0}`);
    lines.push(`include-fiducials=${c.includeFiducials ? 1 : 0}`);
    lines.push(`invert-y=${c.invertY ? 1 : 0}`);
    lines.push(`csvheader=${c.csvHeader ? 1 : 0}`);
    lines.push('');
  }

  return lines.join('\n');
}

/**
 * Returns default export configuration.
 */
export function getDefaultConfig(): ExportConfig {
  return {
    columns: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    zero1: ZeroDirection.Left,
    zero2: ZeroDirection.Left,
    zero3: ZeroDirection.Up,
    side: SideSelection.BothSeparate,
    separator: SeparatorType.Comma,
    unit: UnitType.MM,
    enquoteFields: true,
    alignFiducial: false,
    includeFiducials: false,
    invertY: false,
    angleRange: AngleRange.Range0to360,
    angleClockwise: false,
    csvHeader: false,
    includeNoMount: false,
  };
}

/**
 * Applies a single key=value from INI to the config.
 */
function applyConfigValue(config: ExportConfig, key: string, value: string): void {
  const num = parseInt(value, 10);

  // Column assignments
  const colMatch = key.match(/^centroid(\d+)$/);
  if (colMatch) {
    const idx = parseInt(colMatch[1], 10) - 1;
    if (idx >= 0 && idx < 11) config.columns[idx] = num;
    return;
  }

  switch (key) {
    case 'zero1': config.zero1 = num as ZeroDirection; break;
    case 'zero2': config.zero2 = num as ZeroDirection; break;
    case 'zero3': config.zero3 = num as ZeroDirection; break;
    case 'side': config.side = num as SideSelection; break;
    case 'separator': config.separator = num as SeparatorType; break;
    case 'unit': config.unit = num as UnitType; break;
    case 'enquote-fields': config.enquoteFields = num !== 0; break;
    case 'align-fiducial': config.alignFiducial = num !== 0; break;
    case 'include-fiducials': config.includeFiducials = num !== 0; break;
    case 'invert-y': config.invertY = num !== 0; break;
    case 'angle-range': config.angleRange = num as AngleRange; break;
    case 'angle-clockwise': config.angleClockwise = num !== 0; break;
    case 'csvheader': config.csvHeader = num !== 0; break;
  }
}
