import { useState } from 'react';

interface ExportFile {
  filename: string;
  content: string;
  side: string;
}

interface Props {
  files: ExportFile[];
}

export default function ExportResult({ files }: Props) {
  const [expandedFile, setExpandedFile] = useState<string | null>(null);

  const handleDownload = (file: ExportFile) => {
    const blob = new Blob([file.content], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = file.filename;
    a.click();
    URL.revokeObjectURL(url);
  };

  const handleDownloadAll = () => {
    files.forEach(f => handleDownload(f));
  };

  return (
    <div className="space-y-4">
      <div className="flex items-center justify-between">
        <h3 className="text-sm font-medium text-gray-600">
          Generated {files.length} file{files.length !== 1 ? 's' : ''}
        </h3>
        {files.length > 1 && (
          <button
            onClick={handleDownloadAll}
            className="text-sm text-blue-600 hover:text-blue-800"
          >
            Download all
          </button>
        )}
      </div>

      {files.map(file => (
        <div key={file.filename} className="border border-gray-200 rounded-lg">
          <div className="flex items-center justify-between px-4 py-3 bg-gray-50 rounded-t-lg">
            <div className="flex items-center gap-3">
              <span className="font-medium text-sm text-gray-700">{file.filename}</span>
              <span className="text-xs text-gray-400">
                ({file.side} — {file.content.split('\n').filter(l => l.trim()).length} lines)
              </span>
            </div>
            <div className="flex items-center gap-2">
              <button
                onClick={() => setExpandedFile(expandedFile === file.filename ? null : file.filename)}
                className="text-sm text-gray-500 hover:text-gray-700"
              >
                {expandedFile === file.filename ? 'Hide' : 'Preview'}
              </button>
              <button
                onClick={() => handleDownload(file)}
                className="px-3 py-1 bg-blue-600 hover:bg-blue-700 text-white text-sm rounded"
              >
                Download
              </button>
            </div>
          </div>
          {expandedFile === file.filename && (
            <pre className="p-4 text-xs overflow-x-auto max-h-64 bg-white">
              {file.content}
            </pre>
          )}
        </div>
      ))}
    </div>
  );
}
