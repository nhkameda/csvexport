import { useCallback, useRef, useState } from 'react';

interface Props {
  onFileLoaded: (content: string, filename: string) => void;
}

export default function FileUpload({ onFileLoaded }: Props) {
  const [isDragging, setIsDragging] = useState(false);
  const [filename, setFilename] = useState('');
  const inputRef = useRef<HTMLInputElement>(null);

  const handleFile = useCallback((file: File) => {
    setFilename(file.name);
    const reader = new FileReader();
    reader.onload = (e) => {
      const content = e.target?.result as string;
      onFileLoaded(content, file.name);
    };
    reader.readAsText(file);
  }, [onFileLoaded]);

  const handleDrop = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    setIsDragging(false);
    const file = e.dataTransfer.files[0];
    if (file) handleFile(file);
  }, [handleFile]);

  const handleDragOver = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    setIsDragging(true);
  }, []);

  const handleDragLeave = useCallback(() => setIsDragging(false), []);

  const handleClick = useCallback(() => inputRef.current?.click(), []);

  const handleChange = useCallback((e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (file) handleFile(file);
  }, [handleFile]);

  return (
    <div>
      <div
        onDrop={handleDrop}
        onDragOver={handleDragOver}
        onDragLeave={handleDragLeave}
        onClick={handleClick}
        className={`border-2 border-dashed rounded-lg p-8 text-center cursor-pointer transition-colors ${
          isDragging ? 'border-blue-500 bg-blue-50' : 'border-gray-300 hover:border-gray-400'
        }`}
      >
        <input
          ref={inputRef}
          type="file"
          accept=".csv"
          onChange={handleChange}
          className="hidden"
        />
        {filename ? (
          <p className="text-gray-700">
            <span className="font-medium">{filename}</span>
            <span className="text-gray-400 ml-2">— click or drop to replace</span>
          </p>
        ) : (
          <p className="text-gray-500">
            Drag &amp; drop a CSV file here, or click to select
          </p>
        )}
      </div>
    </div>
  );
}
