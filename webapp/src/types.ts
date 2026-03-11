/** Column type constants matching the DLL's csvexport.h */
export const C_NONE = 0;
export const C_SEQUENCE = 1;
export const C_DESIGNATOR = 2;
export const C_VALUE = 3;
export const C_PACKAGE = 4;
export const C_PRODUCTNR = 5;
export const C_X = 6;
export const C_Y = 7;
export const C_ROTATION = 8;
export const C_SIDE = 9;
export const C_STAGE = 10;
export const C_MOUNT = 11;

/** Zero-orientation direction */
export enum ZeroDirection {
  Up = 0,
  Left = 1,
  Down = 2,
  Right = 3,
}

/** Side selection for export */
export enum SideSelection {
  BothSeparate = 0,
  BothCombined = 1,
  TopOnly = 2,
  BottomOnly = 3,
}

/** Separator type */
export enum SeparatorType {
  Comma = 0,
  Semicolon = 1,
  Tab = 2,
}

/** Unit type */
export enum UnitType {
  MM = 0,
  Inch = 1,
}

/** Angle range type */
export enum AngleRange {
  Range0to360 = 0,
  RangeMinus180to180 = 1,
}

/** Parsed component from Altium CSV */
export interface PartInfo {
  designator: string;
  comment: string;
  layer: string;
  footprint: string;
  x: number;
  y: number;
  rotation: number;
  description: string;
  side: 'Top' | 'Bottom';
  isFiducial: boolean;
}

/** Fiducial type: origin is the reference point, reference is just a marker */
export type FiducialType = 'origin' | 'reference';

/** Fiducial data extracted from the CSV or added manually */
export interface FiducialInfo {
  index: number;
  x: number;
  y: number;
  side: 'Top' | 'Bottom';
  type: FiducialType;
}

/** Export configuration (matches DLL template settings) */
export interface ExportConfig {
  columns: number[];       // 11 column assignments (C_NONE..C_MOUNT)
  zero1: ZeroDirection;    // non-polarized passives (R, C, L, X)
  zero2: ZeroDirection;    // polarized/diodes
  zero3: ZeroDirection;    // misc (>2 pins)
  side: SideSelection;
  separator: SeparatorType;
  unit: UnitType;
  enquoteFields: boolean;
  alignFiducial: boolean;
  includeFiducials: boolean;
  invertY: boolean;
  angleRange: AngleRange;
  angleClockwise: boolean;
  csvHeader: boolean;
  includeNoMount: boolean;
}

/** Template definition */
export interface Template {
  name: string;
  config: ExportConfig;
}

/** Export result for a single file */
export interface ExportFile {
  filename: string;
  content: string;
  side: 'Top' | 'Bottom' | 'Combined';
}
