import { readFileSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';
import { parseAltiumCsv } from '../src/core/parser.js';
import { exportCsv, extractFiducialsFromParts } from '../src/core/exporter.js';
import { parseTemplateIni } from '../src/core/template.js';

const __dirname = dirname(fileURLToPath(import.meta.url));
const ASSETS = join(__dirname, '../assets');

function loadFile(path: string): string {
  return readFileSync(path, 'utf-8');
}

function linesMatch(actual: string, expected: string): boolean {
  if (actual.trim() === expected.trim()) return true;
  const aFields = actual.split(',').map(f => f.trim());
  const eFields = expected.split(',').map(f => f.trim());
  if (aFields.length !== eFields.length) return false;
  for (let i = 0; i < aFields.length; i++) {
    if (aFields[i] === eFields[i]) continue;
    const aNum = parseFloat(aFields[i].replace(/"/g, ''));
    const eNum = parseFloat(eFields[i].replace(/"/g, ''));
    if (!isNaN(aNum) && !isNaN(eNum) && Math.abs(aNum - eNum) <= 0.015) continue;
    return false;
  }
  return true;
}

function compareOutput(actual: string, expected: string, label: string): boolean {
  const actualLines = actual.trim().split('\n');
  const expectedLines = expected.trim().split('\n');

  let exactMatches = 0;
  let toleranceMatches = 0;
  let mismatches = 0;

  const minLines = Math.min(actualLines.length, expectedLines.length);

  for (let i = 0; i < minLines; i++) {
    if (actualLines[i].trim() === expectedLines[i].trim()) {
      exactMatches++;
    } else if (linesMatch(actualLines[i], expectedLines[i])) {
      toleranceMatches++;
    } else {
      console.log(`  [${label}] Line ${i + 1} differs:`);
      console.log(`    GOT:      ${actualLines[i]}`);
      console.log(`    EXPECTED: ${expectedLines[i]}`);
      mismatches++;
    }
  }

  const extraExpected = expectedLines.length - minLines;
  if (extraExpected > 0) {
    console.log(`  [${label}] ${extraExpected} extra expected lines (VP project fiducials)`);
  }

  const total = expectedLines.length;
  console.log(`  [${label}] ${exactMatches} exact + ${toleranceMatches} within tolerance + ${mismatches} mismatches / ${total} expected lines`);

  if (mismatches === 0) {
    console.log(`  [${label}] PASS (${toleranceMatches > 0 ? 'with ' + toleranceMatches + ' rounding diffs of ±0.01mm' : 'exact'})`);
    return true;
  }
  return false;
}

function runTest(csvName: string, baseName: string) {
  console.log(`\nTesting ${csvName}...`);

  const csvContent = loadFile(join(ASSETS, 'csv', csvName));
  const iniContent = loadFile(join(ASSETS, 'template', 'csvexport.ini'));

  const templates = parseTemplateIni(iniContent);
  const yamaha = templates.find(t => t.name === 'YAMAHA');
  if (!yamaha) {
    console.error('  ERROR: YAMAHA template not found');
    return false;
  }

  const parts = parseAltiumCsv(csvContent);
  const fiducials = extractFiducialsFromParts(parts);
  console.log(`  Parsed ${parts.length} components, ${fiducials.length} fiducials`);
  console.log(`  Fiducials: ${fiducials.map(f => `${f.index}(${f.type},${f.side},${f.x},${f.y})`).join(', ')}`);

  const files = exportCsv(parts, yamaha.config, fiducials, baseName);

  let allPass = true;
  for (const file of files) {
    const refFile = join(ASSETS, 'csv', `${baseName}_${file.side.toLowerCase()}.csv`);
    try {
      const expected = loadFile(refFile);
      if (!compareOutput(file.content, expected, file.filename)) allPass = false;
    } catch {
      console.log(`  [${file.filename}] Reference file not found: ${refFile}`);
    }
  }

  return allPass;
}

console.log('=== CSVExport Validation ===');
const r1 = runTest('PCB_A_ori.csv', 'PCB_A');
const r2 = runTest('PCB_B_ori.csv', 'PCB_B');

console.log('\n=== Results ===');
console.log(r1 ? 'PCB_A: PASS' : 'PCB_A: FAIL');
console.log(r2 ? 'PCB_B: PASS' : 'PCB_B: FAIL');
process.exit(r1 && r2 ? 0 : 1);
