import { PartInfo } from '../types.js';

/**
 * Parses an Altium Designer Pick and Place CSV file.
 * Skips header lines until finding the column headers row containing "Designator".
 * Auto-detects separator (comma or semicolon).
 */
export function parseAltiumCsv(csvContent: string): PartInfo[] {
  const lines = csvContent.replace(/\r\n/g, '\n').replace(/\r/g, '\n').split('\n');
  const parts: PartInfo[] = [];

  // Find the header line (contains "Designator")
  let headerIndex = -1;
  for (let i = 0; i < lines.length; i++) {
    if (lines[i].includes('Designator') && lines[i].includes('Footprint')) {
      headerIndex = i;
      break;
    }
  }

  if (headerIndex === -1) {
    throw new Error('Could not find header line containing "Designator" in CSV');
  }

  const headerLine = lines[headerIndex];
  const separator = detectSeparator(headerLine);
  const headers = parseCSVLine(headerLine, separator);

  // Map header names to indices
  const colMap = mapColumns(headers);

  // Parse data rows
  for (let i = headerIndex + 1; i < lines.length; i++) {
    const line = lines[i].trim();
    if (!line) continue;

    const fields = parseCSVLine(line, separator);
    if (fields.length < 4) continue;

    const layer = getField(fields, colMap.layer);
    const side = layerToSide(layer);
    if (!side) continue;

    const designator = getField(fields, colMap.designator);
    const comment = getField(fields, colMap.comment);
    const footprint = getField(fields, colMap.footprint);
    const xStr = getField(fields, colMap.x);
    const yStr = getField(fields, colMap.y);
    const rotStr = getField(fields, colMap.rotation);
    const description = getField(fields, colMap.description);

    const x = parseFloat(xStr) || 0;
    const y = parseFloat(yStr) || 0;
    let rotation = parseFloat(rotStr) || 0;

    // Normalize rotation: 360 → 0
    while (rotation >= 360) rotation -= 360;
    while (rotation < 0) rotation += 360;

    const isFiducial = detectFiducial(designator, comment, footprint);

    parts.push({
      designator,
      comment,
      layer,
      footprint,
      x,
      y,
      rotation,
      description,
      side,
      isFiducial,
    });
  }

  return parts;
}

/**
 * Detects the CSV separator by counting occurrences in the header line.
 */
export function detectSeparator(line: string): string {
  const commaCount = (line.match(/,/g) || []).length;
  const semicolonCount = (line.match(/;/g) || []).length;
  return semicolonCount > commaCount ? ';' : ',';
}

/**
 * Parses a CSV line respecting quoted fields.
 */
function parseCSVLine(line: string, separator: string): string[] {
  const fields: string[] = [];
  let current = '';
  let inQuotes = false;

  for (let i = 0; i < line.length; i++) {
    const ch = line[i];

    if (ch === '"') {
      if (inQuotes && i + 1 < line.length && line[i + 1] === '"') {
        current += '"';
        i++;
      } else {
        inQuotes = !inQuotes;
      }
    } else if (ch === separator && !inQuotes) {
      fields.push(current.trim());
      current = '';
    } else {
      current += ch;
    }
  }
  fields.push(current.trim());

  return fields;
}

interface ColumnMap {
  designator: number;
  comment: number;
  layer: number;
  footprint: number;
  x: number;
  y: number;
  rotation: number;
  description: number;
}

/**
 * Maps known Altium header names to column indices.
 */
function mapColumns(headers: string[]): ColumnMap {
  const map: ColumnMap = {
    designator: -1,
    comment: -1,
    layer: -1,
    footprint: -1,
    x: -1,
    y: -1,
    rotation: -1,
    description: -1,
  };

  for (let i = 0; i < headers.length; i++) {
    const h = headers[i].toLowerCase().replace(/"/g, '');
    if (h === 'designator') map.designator = i;
    else if (h === 'comment' || h === 'value') map.comment = i;
    else if (h === 'layer') map.layer = i;
    else if (h === 'footprint') map.footprint = i;
    else if (h.includes('center-x') || h === 'x(mm)' || h === 'mid x') map.x = i;
    else if (h.includes('center-y') || h === 'y(mm)' || h === 'mid y') map.y = i;
    else if (h === 'rotation' || h === 'rotate') map.rotation = i;
    else if (h === 'description') map.description = i;
  }

  if (map.designator === -1) throw new Error('Column "Designator" not found');
  if (map.x === -1) throw new Error('X coordinate column not found');
  if (map.y === -1) throw new Error('Y coordinate column not found');

  return map;
}

function getField(fields: string[], index: number): string {
  if (index < 0 || index >= fields.length) return '';
  return fields[index].replace(/^"|"$/g, '');
}

function layerToSide(layer: string): 'Top' | 'Bottom' | null {
  const l = layer.toLowerCase();
  if (l.includes('top')) return 'Top';
  if (l.includes('bottom')) return 'Bottom';
  return null;
}

/**
 * Detects if a component is a fiducial based on designator, comment, or footprint.
 */
function detectFiducial(designator: string, comment: string, footprint: string): boolean {
  const combined = `${designator} ${comment} ${footprint}`.toLowerCase();
  return combined.includes('fiducial') || combined.includes('fid');
}
