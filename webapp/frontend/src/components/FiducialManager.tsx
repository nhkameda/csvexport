import { useState } from 'react';

interface FiducialInfo {
  index: number;
  x: number;
  y: number;
  side: 'Top' | 'Bottom';
  type: 'origin' | 'reference';
}

interface Props {
  fiducials: FiducialInfo[];
  alignFiducial: boolean;
  onChange: (fiducials: FiducialInfo[]) => void;
}

export default function FiducialManager({ fiducials, alignFiducial, onChange }: Props) {
  const [showAdd, setShowAdd] = useState(false);
  const [newX, setNewX] = useState('');
  const [newY, setNewY] = useState('');
  const [newSide, setNewSide] = useState<'Top' | 'Bottom'>('Bottom');
  const [newType, setNewType] = useState<'origin' | 'reference'>('reference');

  const hasBottomOrigin = fiducials.some(f => f.side === 'Bottom' && f.type === 'origin');
  const hasTopOrigin = fiducials.some(f => f.side === 'Top' && f.type === 'origin');

  const handleTypeChange = (index: number, newType: 'origin' | 'reference') => {
    const updated = fiducials.map(f => {
      if (f.index === index) {
        return { ...f, type: newType };
      }
      // If setting a new origin, clear the previous origin on the same side
      if (newType === 'origin') {
        const target = fiducials.find(ff => ff.index === index);
        if (target && f.side === target.side && f.type === 'origin' && f.index !== index) {
          return { ...f, type: 'reference' as const };
        }
      }
      return f;
    });
    onChange(updated);
  };

  const handleDelete = (index: number) => {
    const updated = fiducials
      .filter(f => f.index !== index)
      .map((f, i) => ({ ...f, index: i + 1 }));
    onChange(updated);
  };

  const handleAdd = () => {
    const x = parseFloat(newX);
    const y = parseFloat(newY);
    if (isNaN(x) || isNaN(y)) return;

    const newFid: FiducialInfo = {
      index: fiducials.length + 1,
      x,
      y,
      side: newSide,
      type: newType,
    };
    // If adding as origin, clear previous origin on same side
    let updated = [...fiducials];
    if (newType === 'origin') {
      updated = updated.map(f =>
        f.side === newSide && f.type === 'origin' ? { ...f, type: 'reference' as const } : f
      );
    }
    onChange([...updated, newFid]);
    setShowAdd(false);
    setNewX('');
    setNewY('');
    setNewType('reference');
  };

  const bottomFids = fiducials.filter(f => f.side === 'Bottom');
  const topFids = fiducials.filter(f => f.side === 'Top');

  return (
    <div className="space-y-4">
      {/* Warning when align-fiducial is ON but no origin */}
      {alignFiducial && !hasBottomOrigin && bottomFids.length > 0 && (
        <div className="bg-yellow-50 border border-yellow-200 text-yellow-800 px-4 py-3 rounded text-sm">
          No "origin" fiducial is set (bottom side). Component positions will be relative to the fiducial nearest the bottom-left corner.
        </div>
      )}

      {alignFiducial && bottomFids.length === 0 && (
        <div className="bg-yellow-50 border border-yellow-200 text-yellow-800 px-4 py-3 rounded text-sm">
          No fiducials detected on the bottom side. Component positions will be relative to the bottom-left corner of the PCB.
        </div>
      )}

      {fiducials.length === 0 ? (
        <p className="text-sm text-gray-500">No fiducials detected in the CSV file.</p>
      ) : (
        <div className="overflow-x-auto">
          <table className="w-full text-sm border-collapse">
            <thead>
              <tr className="border-b border-gray-200">
                <th className="text-left py-2 px-3 font-medium text-gray-600">#</th>
                <th className="text-left py-2 px-3 font-medium text-gray-600">Type</th>
                <th className="text-left py-2 px-3 font-medium text-gray-600">X (mm)</th>
                <th className="text-left py-2 px-3 font-medium text-gray-600">Y (mm)</th>
                <th className="text-left py-2 px-3 font-medium text-gray-600">Side</th>
                <th className="text-right py-2 px-3 font-medium text-gray-600"></th>
              </tr>
            </thead>
            <tbody>
              {fiducials.map(fid => (
                <tr key={fid.index} className="border-b border-gray-100 hover:bg-gray-50">
                  <td className="py-2 px-3 text-gray-500">{fid.index}</td>
                  <td className="py-2 px-3">
                    <select
                      value={fid.type}
                      onChange={(e) => handleTypeChange(fid.index, e.target.value as 'origin' | 'reference')}
                      className={`border rounded px-2 py-1 text-sm ${
                        fid.type === 'origin'
                          ? 'border-green-400 bg-green-50 text-green-800 font-medium'
                          : 'border-gray-300 text-gray-700'
                      }`}
                    >
                      <option value="reference">Reference</option>
                      <option value="origin">Origin</option>
                    </select>
                  </td>
                  <td className="py-2 px-3 font-mono text-gray-700">{fid.x.toFixed(4)}</td>
                  <td className="py-2 px-3 font-mono text-gray-700">{fid.y.toFixed(4)}</td>
                  <td className="py-2 px-3">
                    <span className={`inline-block px-2 py-0.5 rounded text-xs font-medium ${
                      fid.side === 'Top'
                        ? 'bg-blue-100 text-blue-700'
                        : 'bg-orange-100 text-orange-700'
                    }`}>
                      {fid.side}
                    </span>
                  </td>
                  <td className="py-2 px-3 text-right">
                    <button
                      onClick={() => handleDelete(fid.index)}
                      className="text-red-500 hover:text-red-700 text-xs"
                      title="Delete fiducial"
                    >
                      Delete
                    </button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}

      {/* Add Fiducial */}
      <div className="flex items-center gap-2">
        <button
          onClick={() => setShowAdd(!showAdd)}
          className="px-3 py-1.5 bg-green-600 hover:bg-green-700 text-white text-sm rounded"
        >
          Add Fiducial
        </button>
      </div>

      {showAdd && (
        <div className="flex items-end gap-3 bg-gray-50 p-3 rounded">
          <div>
            <label className="block text-xs text-gray-500 mb-1">X (mm)</label>
            <input
              type="number"
              step="0.0001"
              value={newX}
              onChange={(e) => setNewX(e.target.value)}
              className="w-28 border border-gray-300 rounded px-2 py-1.5 text-sm"
              placeholder="0.0000"
            />
          </div>
          <div>
            <label className="block text-xs text-gray-500 mb-1">Y (mm)</label>
            <input
              type="number"
              step="0.0001"
              value={newY}
              onChange={(e) => setNewY(e.target.value)}
              className="w-28 border border-gray-300 rounded px-2 py-1.5 text-sm"
              placeholder="0.0000"
            />
          </div>
          <div>
            <label className="block text-xs text-gray-500 mb-1">Side</label>
            <select
              value={newSide}
              onChange={(e) => setNewSide(e.target.value as 'Top' | 'Bottom')}
              className="border border-gray-300 rounded px-2 py-1.5 text-sm"
            >
              <option value="Top">Top</option>
              <option value="Bottom">Bottom</option>
            </select>
          </div>
          <div>
            <label className="block text-xs text-gray-500 mb-1">Type</label>
            <select
              value={newType}
              onChange={(e) => setNewType(e.target.value as 'origin' | 'reference')}
              className="border border-gray-300 rounded px-2 py-1.5 text-sm"
            >
              <option value="reference">Reference</option>
              <option value="origin">Origin</option>
            </select>
          </div>
          <button
            onClick={handleAdd}
            className="px-3 py-1.5 bg-blue-600 hover:bg-blue-700 text-white text-sm rounded"
          >
            Add
          </button>
          <button
            onClick={() => setShowAdd(false)}
            className="px-3 py-1.5 text-gray-500 text-sm"
          >
            Cancel
          </button>
        </div>
      )}

      {!alignFiducial && fiducials.length > 0 && (
        <p className="text-xs text-gray-400">
          "Align to fiducial" is off — component positions will be relative to the bottom-left corner.
        </p>
      )}
    </div>
  );
}
