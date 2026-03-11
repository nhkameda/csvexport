# CSVExport

Web application for converting Altium Designer Pick & Place CSV files into machine-specific formats for SMT (Surface Mount Technology) pick-and-place equipment.

This project is a reimplementation of the **csvexport.dll** plugin originally developed for [VisualPlace](https://www.compuphase.com/), now as a standalone web application accessible from any browser.

## Features

- **Altium CSV Parsing** — Automatic separator detection, header recognition, and component extraction from Altium Designer Pick & Place output files
- **Template System** — Load and save machine configurations using INI-format templates (compatible with VisualPlace template format)
- **Column Mapping** — Configurable 11-column output mapping (Designator, Value, Package, X, Y, Rotation, Side, Mount, etc.)
- **Fiducial Management** — View, add, and delete fiducial markers; set fiducial type (Reference or Origin) for coordinate alignment
- **Coordinate Transformation** — Bottom-side X-axis mirroring, fiducial-based origin alignment, unit conversion (mm/inch), Y-axis inversion
- **Rotation Adjustment** — Zero-orientation settings for non-polarized passives, polarized components, and multi-pin ICs
- **Side Selection** — Export Top only, Bottom only, both as separate files, or combined
- **Export Options** — Field quoting, angle range (0-360 or -180 to 180), clockwise/counter-clockwise rotation, CSV headers

## Tech Stack

- **Backend**: Fastify 4 + TypeScript
- **Frontend**: React 18 + Vite 5 + TailwindCSS 3

## Getting Started

### Prerequisites

- Node.js 18+
- npm

### Installation

```bash
# Clone the repository
git clone https://github.com/nhkameda/csvexport.git
cd csvexport/webapp

# Install backend dependencies
npm install

# Install frontend dependencies
cd frontend
npm install
cd ..
```

### Running

```bash
# Start the backend server (port 3333)
npx tsx src/server.ts &

# Start the frontend dev server (port 5173)
cd frontend
npm run dev
```

Open http://localhost:5173 in your browser.

### Validation

Reference CSV files are included in `assets/csv/`. To verify the export logic against expected output:

```bash
cd webapp
npx tsx test/validate.ts
```

## Usage

1. **Upload** an Altium Designer Pick & Place CSV file
2. **Load a template** or configure column mapping and export options manually
3. **Review fiducials** detected from the CSV; set one as Origin if needed for coordinate alignment
4. **Configure export options** (units, angle format, side selection, etc.)
5. **Export** — download the generated CSV files for your pick-and-place machine

## Project Structure

```
assets/
  csv/          # Reference input/output CSV files
  dll/          # Original csvexport.dll source code (C)
  template/     # INI template files
webapp/
  src/
    core/       # Parser, exporter, and template logic
    routes/     # API endpoints (parse, export, templates)
    server.ts   # Fastify server entry point
    types.ts    # TypeScript type definitions
  frontend/
    src/
      components/   # React UI components
      App.tsx        # Main application
  test/         # Validation tests
```

## Acknowledgments

This project is based on the **csvexport.dll** plugin for [VisualPlace](https://www.compuphase.com/), a centroid viewer and editor for PCB assembly.

Special thanks to **Thiadmer Riemersma** from [CompuPhase](https://www.compuphase.com/) for generously providing the original source code of csvexport.dll, which made this reimplementation possible.

## License

MIT
