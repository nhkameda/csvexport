import { FastifyInstance } from 'fastify';
import { parseAltiumCsv } from '../core/parser.js';
import { exportCsv, extractFiducialsFromParts } from '../core/exporter.js';
import { ExportConfig, FiducialInfo } from '../types.js';

interface ExportBody {
  csvContent: string;
  config: ExportConfig;
  fiducials?: FiducialInfo[];
  baseFilename?: string;
}

export async function exportRoutes(server: FastifyInstance) {
  /**
   * POST /api/export — Processes an Altium CSV with the given config and returns exported files.
   * Accepts optional fiducials array; if not provided, auto-detects from CSV.
   */
  server.post<{ Body: ExportBody }>('/export', async (request, reply) => {
    const { csvContent, config, fiducials: providedFiducials, baseFilename } = request.body;

    if (!csvContent || !config) {
      return reply.status(400).send({ error: 'Missing csvContent or config' });
    }

    try {
      const parts = parseAltiumCsv(csvContent);
      const fiducials = providedFiducials ?? extractFiducialsFromParts(parts);
      const files = exportCsv(parts, config, fiducials, baseFilename || 'output');
      return { files };
    } catch (err: any) {
      return reply.status(400).send({ error: err.message });
    }
  });

  /**
   * POST /api/parse — Parses an Altium CSV and returns detected components + fiducials.
   */
  server.post<{ Body: { csvContent: string } }>('/parse', async (request, reply) => {
    const { csvContent } = request.body;

    if (!csvContent) {
      return reply.status(400).send({ error: 'Missing csvContent' });
    }

    try {
      const parts = parseAltiumCsv(csvContent);
      const fiducials = extractFiducialsFromParts(parts);
      return {
        totalParts: parts.length,
        topParts: parts.filter(p => p.side === 'Top' && !p.isFiducial).length,
        bottomParts: parts.filter(p => p.side === 'Bottom' && !p.isFiducial).length,
        fiducials,
        parts: parts.slice(0, 10),
      };
    } catch (err: any) {
      return reply.status(400).send({ error: err.message });
    }
  });
}
