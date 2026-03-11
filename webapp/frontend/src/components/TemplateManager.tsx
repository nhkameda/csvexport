import { useState, useEffect, useCallback } from 'react';

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

interface Template {
  name: string;
  config: ExportConfig;
}

interface Props {
  config: ExportConfig;
  onTemplateLoaded: (config: ExportConfig) => void;
}

export default function TemplateManager({ config, onTemplateLoaded }: Props) {
  const [templates, setTemplates] = useState<Template[]>([]);
  const [selected, setSelected] = useState('');
  const [newName, setNewName] = useState('');
  const [showNewModal, setShowNewModal] = useState(false);

  // Load templates from server
  useEffect(() => {
    fetch('/api/templates')
      .then(r => r.json())
      .then(data => {
        if (data.templates?.length) {
          setTemplates(data.templates);
          // Auto-select YAMAHA if available
          const yamaha = data.templates.find((t: Template) => t.name === 'YAMAHA');
          if (yamaha) {
            setSelected('YAMAHA');
            onTemplateLoaded(yamaha.config);
          }
        }
      })
      .catch(() => {});
  }, []);

  const handleSelect = useCallback((name: string) => {
    setSelected(name);
    const tmpl = templates.find(t => t.name === name);
    if (tmpl) onTemplateLoaded(tmpl.config);
  }, [templates, onTemplateLoaded]);

  const handleSave = useCallback(() => {
    if (!selected) return;
    const updated = templates.map(t =>
      t.name === selected ? { ...t, config: { ...config } } : t
    );
    setTemplates(updated);
    // Save to localStorage
    localStorage.setItem('csvexport-templates', JSON.stringify(updated));
  }, [selected, config, templates]);

  const handleNew = useCallback(() => {
    if (!newName.trim()) return;
    const exists = templates.find(t => t.name === newName);
    if (exists) return;
    const newTemplate: Template = { name: newName, config: { ...config } };
    const updated = [...templates, newTemplate];
    setTemplates(updated);
    setSelected(newName);
    setShowNewModal(false);
    setNewName('');
    localStorage.setItem('csvexport-templates', JSON.stringify(updated));
  }, [newName, config, templates]);

  const handleExportIni = useCallback(async () => {
    const res = await fetch('/api/templates/export', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ templates }),
    });
    const data = await res.json();
    const blob = new Blob([data.ini], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'csvexport.ini';
    a.click();
    URL.revokeObjectURL(url);
  }, [templates]);

  return (
    <div className="space-y-3">
      <div className="flex items-center gap-3">
        <select
          value={selected}
          onChange={(e) => handleSelect(e.target.value)}
          className="flex-1 border border-gray-300 rounded px-3 py-2 text-sm"
        >
          <option value="">-- Select template --</option>
          {templates.map(t => (
            <option key={t.name} value={t.name}>{t.name}</option>
          ))}
        </select>
        <button
          onClick={() => setShowNewModal(true)}
          className="px-3 py-2 bg-green-600 hover:bg-green-700 text-white text-sm rounded"
        >
          New
        </button>
        <button
          onClick={handleSave}
          disabled={!selected}
          className="px-3 py-2 bg-gray-600 hover:bg-gray-700 disabled:bg-gray-300 text-white text-sm rounded"
        >
          Save
        </button>
        <button
          onClick={handleExportIni}
          disabled={templates.length === 0}
          className="px-3 py-2 bg-gray-600 hover:bg-gray-700 disabled:bg-gray-300 text-white text-sm rounded"
        >
          Export .ini
        </button>
      </div>

      {showNewModal && (
        <div className="flex items-center gap-2 bg-gray-50 p-3 rounded">
          <input
            type="text"
            value={newName}
            onChange={(e) => setNewName(e.target.value)}
            placeholder="Template name"
            className="flex-1 border border-gray-300 rounded px-3 py-1.5 text-sm"
            autoFocus
            onKeyDown={(e) => e.key === 'Enter' && handleNew()}
          />
          <button onClick={handleNew} className="px-3 py-1.5 bg-blue-600 text-white text-sm rounded">
            Create
          </button>
          <button onClick={() => setShowNewModal(false)} className="px-3 py-1.5 text-gray-500 text-sm">
            Cancel
          </button>
        </div>
      )}
    </div>
  );
}
