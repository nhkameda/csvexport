import Fastify from 'fastify';
import cors from '@fastify/cors';
import multipart from '@fastify/multipart';
import { exportRoutes } from './routes/export.js';
import { templateRoutes } from './routes/templates.js';

const server = Fastify({ logger: true });

async function start() {
  await server.register(cors, { origin: true });
  await server.register(multipart, { limits: { fileSize: 10 * 1024 * 1024 } });

  await server.register(exportRoutes, { prefix: '/api' });
  await server.register(templateRoutes, { prefix: '/api' });

  server.get('/api/health', async () => ({ status: 'ok' }));

  try {
    await server.listen({ port: 3333, host: '0.0.0.0' });
    console.log('Server running at http://localhost:3333');
  } catch (err) {
    server.log.error(err);
    process.exit(1);
  }
}

start();
