import { useState, useCallback } from 'react';
import FileUpload from './components/FileUpload';
import TemplateManager from './components/TemplateManager';
import ColumnMapper from './components/ColumnMapper';
import ExportOptions from './components/ExportOptions';
import FiducialManager from './components/FiducialManager';
import ExportResult from './components/ExportResult';

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

interface FiducialInfo {
  index: number;
  x: number;
  y: number;
  side: 'Top' | 'Bottom';
  type: 'origin' | 'reference';
}

interface ExportFile {
  filename: string;
  content: string;
  side: string;
}

const DEFAULT_CONFIG: ExportConfig = {
  columns: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
  zero1: 1,
  zero2: 1,
  zero3: 0,
  side: 0,
  separator: 0,
  unit: 0,
  enquoteFields: true,
  alignFiducial: false,
  includeFiducials: false,
  invertY: false,
  angleRange: 0,
  angleClockwise: false,
  csvHeader: false,
  includeNoMount: false,
};

export default function App() {
  const [csvContent, setCsvContent] = useState<string>('');
  const [csvFilename, setCsvFilename] = useState<string>('');
  const [csvPreview, setCsvPreview] = useState<string[]>([]);
  const [parseInfo, setParseInfo] = useState<{ topParts: number; bottomParts: number; fiducials: number } | null>(null);
  const [fiducials, setFiducials] = useState<FiducialInfo[]>([]);
  const [config, setConfig] = useState<ExportConfig>(DEFAULT_CONFIG);
  const [exportFiles, setExportFiles] = useState<ExportFile[]>([]);
  const [isExporting, setIsExporting] = useState(false);
  const [error, setError] = useState<string>('');
  const [warnings, setWarnings] = useState<string[]>([]);

  const handleFileLoaded = useCallback((content: string, filename: string) => {
    setCsvContent(content);
    setCsvFilename(filename);
    setExportFiles([]);
    setFiducials([]);
    setError('');

    const lines = content.split('\n').filter(l => l.trim());
    setCsvPreview(lines.slice(0, 15));

    // Parse on server
    fetch('/api/parse', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ csvContent: content }),
    })
      .then(r => r.json())
      .then(data => {
        if (data.error) {
          setError(data.error);
        } else {
          setParseInfo({ topParts: data.topParts, bottomParts: data.bottomParts, fiducials: data.fiducials?.length ?? 0 });
          if (data.fiducials) {
            setFiducials(data.fiducials);
          }
        }
      })
      .catch(e => setError(e.message));
  }, []);

  const handleTemplateLoaded = useCallback((templateConfig: ExportConfig) => {
    setConfig(templateConfig);
  }, []);

  const handleExport = useCallback(async () => {
    if (!csvContent) {
      setError('Please upload a CSV file first');
      return;
    }
    setIsExporting(true);
    setError('');
    setWarnings([]);

    // Check for missing origin fiducials when align-fiducial is ON
    if (config.alignFiducial) {
      const msgs: string[] = [];
      const bottomFids = fiducials.filter(f => f.side === 'Bottom');
      const topFids = fiducials.filter(f => f.side === 'Top');
      const hasBottomOrigin = bottomFids.some(f => f.type === 'origin');
      const hasTopOrigin = topFids.some(f => f.type === 'origin');
      const exportBottom = config.side === 0 || config.side === 3; // BothSeparate or BottomOnly
      const exportTop = config.side === 0 || config.side === 2; // BothSeparate or TopOnly

      if (exportBottom && bottomFids.length > 0 && !hasBottomOrigin) {
        msgs.push('No "origin" fiducial is set (bottom side). Component positions will be relative to the fiducial nearest the bottom-left corner.');
      }
      if (exportTop && topFids.length > 0 && !hasTopOrigin) {
        msgs.push('No "origin" fiducial is set (top side). Component positions will be relative to the fiducial nearest the bottom-left corner.');
      }
      setWarnings(msgs);
    }

    try {
      const baseName = csvFilename.replace(/\.csv$/i, '').replace(/_ori$/i, '');
      const res = await fetch('/api/export', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ csvContent, config, fiducials, baseFilename: baseName }),
      });
      const data = await res.json();
      if (data.error) {
        setError(data.error);
      } else {
        setExportFiles(data.files);
      }
    } catch (e: any) {
      setError(e.message);
    } finally {
      setIsExporting(false);
    }
  }, [csvContent, csvFilename, config, fiducials]);

  return (
    <div className="max-w-5xl mx-auto px-4 py-8">
      <header className="mb-8">
        <h1 className="text-3xl font-bold text-gray-800">CSVExport</h1>
        <p className="text-gray-500 mt-1">Pick &amp; Place CSV Exporter — Altium Designer to Machine Format</p>
      </header>

      {error && (
        <div className="bg-red-50 border border-red-200 text-red-700 px-4 py-3 rounded mb-6">
          {error}
        </div>
      )}

      {warnings.length > 0 && (
        <div className="space-y-2 mb-6">
          {warnings.map((msg, i) => (
            <div key={i} className="bg-yellow-50 border border-yellow-200 text-yellow-800 px-4 py-3 rounded text-sm">
              {msg}
            </div>
          ))}
        </div>
      )}

      {/* Section 1: File Upload */}
      <section className="bg-white rounded-lg shadow p-6 mb-6">
        <h2 className="text-lg font-semibold text-gray-700 mb-4">1. Upload CSV File</h2>
        <FileUpload onFileLoaded={handleFileLoaded} />
        {parseInfo && (
          <div className="mt-3 text-sm text-gray-600">
            Components: <span className="font-medium">{parseInfo.topParts} Top</span>,{' '}
            <span className="font-medium">{parseInfo.bottomParts} Bottom</span>,{' '}
            <span className="font-medium">{parseInfo.fiducials} Fiducials</span>
          </div>
        )}
        {csvPreview.length > 0 && (
          <div className="mt-4">
            <h3 className="text-sm font-medium text-gray-500 mb-2">Preview:</h3>
            <pre className="bg-gray-50 p-3 rounded text-xs overflow-x-auto max-h-48">
              {csvPreview.join('\n')}
            </pre>
          </div>
        )}
      </section>

      {/* Section 2: Template */}
      <section className="bg-white rounded-lg shadow p-6 mb-6">
        <h2 className="text-lg font-semibold text-gray-700 mb-4">2. Template</h2>
        <TemplateManager config={config} onTemplateLoaded={handleTemplateLoaded} />
      </section>

      {/* Section 3: Column Mapping */}
      <section className="bg-white rounded-lg shadow p-6 mb-6">
        <h2 className="text-lg font-semibold text-gray-700 mb-4">3. Column Mapping</h2>
        <ColumnMapper
          columns={config.columns}
          onChange={(columns) => setConfig(prev => ({ ...prev, columns }))}
        />
      </section>

      {/* Section 4: Fiducials */}
      {csvContent && (
        <section className="bg-white rounded-lg shadow p-6 mb-6">
          <h2 className="text-lg font-semibold text-gray-700 mb-4">4. Fiducials</h2>
          <p className="text-sm text-gray-500 mb-4">
            This table has the positions of the known fiducials.<br />
            The primary fiducial may be used as the new origin by a pick-&amp;-place machine (this depends on the machine).<br />
            To erase a fiducial, clear its X and Y positions. To add a fiducial, use the "Add fiducial..." button below the table.
          </p>
          <FiducialManager
            fiducials={fiducials}
            alignFiducial={config.alignFiducial}
            onChange={setFiducials}
          />
        </section>
      )}

      {/* Section 5: Export Options */}
      <section className="bg-white rounded-lg shadow p-6 mb-6">
        <h2 className="text-lg font-semibold text-gray-700 mb-4">5. Export Options</h2>
        <ExportOptions config={config} onChange={setConfig} />
      </section>

      {/* Section 6: Export */}
      <section className="bg-white rounded-lg shadow p-6 mb-6">
        <h2 className="text-lg font-semibold text-gray-700 mb-4">6. Export</h2>
        <button
          onClick={handleExport}
          disabled={!csvContent || isExporting}
          className="w-full py-3 px-6 bg-blue-600 hover:bg-blue-700 disabled:bg-gray-300 text-white font-semibold rounded-lg text-lg transition-colors"
        >
          {isExporting ? 'Exporting...' : 'Export'}
        </button>
        {exportFiles.length > 0 && (
          <div className="mt-6">
            <ExportResult files={exportFiles} />
          </div>
        )}
      </section>
    </div>
  );
}
