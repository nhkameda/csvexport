interface Props {
  columns: number[];
  onChange: (columns: number[]) => void;
}

const COLUMN_OPTIONS = [
  { value: 0, label: '(none)' },
  { value: 1, label: 'Sequence' },
  { value: 2, label: 'Designator' },
  { value: 3, label: 'Value' },
  { value: 4, label: 'Package' },
  { value: 5, label: 'Product Nr' },
  { value: 6, label: 'X' },
  { value: 7, label: 'Y' },
  { value: 8, label: 'Rotation' },
  { value: 9, label: 'Side' },
  { value: 10, label: 'Stage' },
  { value: 11, label: 'Mount Y/N' },
];

export default function ColumnMapper({ columns, onChange }: Props) {
  const handleChange = (index: number, value: number) => {
    const newColumns = [...columns];
    newColumns[index] = value;
    onChange(newColumns);
  };

  return (
    <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 lg:grid-cols-6 gap-3">
      {Array.from({ length: 11 }, (_, i) => (
        <div key={i}>
          <label className="block text-xs font-medium text-gray-500 mb-1">
            Column {i + 1}
          </label>
          <select
            value={columns[i]}
            onChange={(e) => handleChange(i, parseInt(e.target.value))}
            className="w-full border border-gray-300 rounded px-2 py-1.5 text-sm focus:ring-blue-500 focus:border-blue-500"
          >
            {COLUMN_OPTIONS.map(opt => (
              <option key={opt.value} value={opt.value}>{opt.label}</option>
            ))}
          </select>
        </div>
      ))}
    </div>
  );
}
