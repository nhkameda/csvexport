import { FastifyInstance } from 'fastify';
import { readFileSync, existsSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';
import { parseTemplateIni, serializeTemplateIni } from '../core/template.js';
import { Template } from '../types.js';

const __dirname = dirname(fileURLToPath(import.meta.url));
const TEMPLATE_FILE = join(__dirname, '../../assets/template/csvexport.ini');

export async function templateRoutes(server: FastifyInstance) {
  /**
   * GET /api/templates — Returns available templates from the INI file.
   */
  server.get('/templates', async () => {
    const templates = loadTemplates();
    return { templates };
  });

  /**
   * POST /api/templates/export — Exports templates as INI content.
   */
  server.post<{ Body: { templates: Template[] } }>('/templates/export', async (request) => {
    const { templates } = request.body;
    const ini = serializeTemplateIni(templates);
    return { ini };
  });
}

function loadTemplates(): Template[] {
  if (!existsSync(TEMPLATE_FILE)) return [];
  const content = readFileSync(TEMPLATE_FILE, 'utf-8');
  return parseTemplateIni(content);
}
