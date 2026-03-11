interface ExportConfig {
  columns: number[];
  zero1: number;
  zero2: number;
  zero3: number;
  side: number;
  separator: number;
  unit: number;
  enquoteFields: boolean;
  alignFiducial: boolean;
  includeFiducials: boolean;
  invertY: boolean;
  angleRange: number;
  angleClockwise: boolean;
  csvHeader: boolean;
  includeNoMount: boolean;
}

interface Props {
  config: ExportConfig;
  onChange: (config: ExportConfig) => void;
}

const SIDE_OPTIONS = ['Top+Bottom separate', 'Top+Bottom combined', 'Top only', 'Bottom only'];
const SEPARATOR_OPTIONS = ['Comma', 'Semicolon', 'Tab'];
const UNIT_OPTIONS = ['mm', 'inch'];
const ANGLE_RANGE_OPTIONS = ['0 .. 360', '-180 .. 180'];
const ZERO_OPTIONS = ['Up', 'Left', 'Down', 'Right'];

export default function ExportOptions({ config, onChange }: Props) {
  const set = <K extends keyof ExportConfig>(key: K, value: ExportConfig[K]) => {
    onChange({ ...config, [key]: value });
  };

  return (
    <div className="space-y-6">
      {/* Row 1: Side, Separator, Unit */}
      <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
        <div>
          <label className="block text-sm font-medium text-gray-600 mb-1">Side</label>
          <select
            value={config.side}
            onChange={(e) => set('side', parseInt(e.target.value))}
            className="w-full border border-gray-300 rounded px-3 py-2 text-sm"
          >
            {SIDE_OPTIONS.map((label, i) => (
              <option key={i} value={i}>{label}</option>
            ))}
          </select>
        </div>
        <div>
          <label className="block text-sm font-medium text-gray-600 mb-1">Separator</label>
          <select
            value={config.separator}
            onChange={(e) => set('separator', parseInt(e.target.value))}
            className="w-full border border-gray-300 rounded px-3 py-2 text-sm"
          >
            {SEPARATOR_OPTIONS.map((label, i) => (
              <option key={i} value={i}>{label}</option>
            ))}
          </select>
        </div>
        <div>
          <label className="block text-sm font-medium text-gray-600 mb-1">Unit</label>
          <select
            value={config.unit}
            onChange={(e) => set('unit', parseInt(e.target.value))}
            className="w-full border border-gray-300 rounded px-3 py-2 text-sm"
          >
            {UNIT_OPTIONS.map((label, i) => (
              <option key={i} value={i}>{label}</option>
            ))}
          </select>
        </div>
      </div>

      {/* Row 2: Angle settings */}
      <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
        <div>
          <label className="block text-sm font-medium text-gray-600 mb-1">Angle Range</label>
          <select
            value={config.angleRange}
            onChange={(e) => set('angleRange', parseInt(e.target.value))}
            className="w-full border border-gray-300 rounded px-3 py-2 text-sm"
          >
            {ANGLE_RANGE_OPTIONS.map((label, i) => (
              <option key={i} value={i}>{label}</option>
            ))}
          </select>
        </div>
        <div className="flex items-end">
          <label className="flex items-center gap-2 text-sm text-gray-600">
            <input
              type="checkbox"
              checked={config.angleClockwise}
              onChange={(e) => set('angleClockwise', e.target.checked)}
              className="rounded"
            />
            Clockwise rotation
          </label>
        </div>
      </div>

      {/* Row 3: Zero-orientation */}
      <div>
        <label className="block text-sm font-medium text-gray-600 mb-2">Zero-Orientation (pin 1 position)</label>
        <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
          <div>
            <label className="block text-xs text-gray-500 mb-1">Passives (R, C, L, X)</label>
            <select
              value={config.zero1}
              onChange={(e) => set('zero1', parseInt(e.target.value))}
              className="w-full border border-gray-300 rounded px-3 py-2 text-sm"
            >
              {ZERO_OPTIONS.map((label, i) => (
                <option key={i} value={i}>{label}</option>
              ))}
            </select>
          </div>
          <div>
            <label className="block text-xs text-gray-500 mb-1">Diodes / Polarized</label>
            <select
              value={config.zero2}
              onChange={(e) => set('zero2', parseInt(e.target.value))}
              className="w-full border border-gray-300 rounded px-3 py-2 text-sm"
            >
              {ZERO_OPTIONS.map((label, i) => (
                <option key={i} value={i}>{label}</option>
              ))}
            </select>
          </div>
          <div>
            <label className="block text-xs text-gray-500 mb-1">Misc (&gt;2 pins)</label>
            <select
              value={config.zero3}
              onChange={(e) => set('zero3', parseInt(e.target.value))}
              className="w-full border border-gray-300 rounded px-3 py-2 text-sm"
            >
              {ZERO_OPTIONS.map((label, i) => (
                <option key={i} value={i}>{label}</option>
              ))}
            </select>
          </div>
        </div>
      </div>

      {/* Row 4: Checkboxes */}
      <div className="grid grid-cols-2 sm:grid-cols-3 gap-3">
        {[
          { key: 'enquoteFields' as const, label: 'Enquote fields' },
          { key: 'invertY' as const, label: 'Invert Y axis' },
          { key: 'alignFiducial' as const, label: 'Align to fiducial' },
          { key: 'includeFiducials' as const, label: 'Include fiducials' },
          { key: 'csvHeader' as const, label: 'CSV header comments' },
          { key: 'includeNoMount' as const, label: 'Include no-mount' },
        ].map(({ key, label }) => (
          <label key={key} className="flex items-center gap-2 text-sm text-gray-600">
            <input
              type="checkbox"
              checked={config[key] as boolean}
              onChange={(e) => set(key, e.target.checked)}
              className="rounded"
            />
            {label}
          </label>
        ))}
      </div>
    </div>
  );
}
