'use strict';

const app = require('./app');
const config = require('./src/config');
const logger = require('./src/utils/logger');

const PORT = config.port || process.env.PORT || 3000;

const server = app.listen(PORT, () => {
  logger.info(`Server running on port ${PORT}`);
  logger.info(`Health check: http://localhost:${PORT}/health`);
});

process.on('SIGTERM', () => {
  logger.info('SIGTERM received. Closing server...');
  server.close(() => {
    logger.info('Server closed.');
    process.exit(0);
  });
});

process.on('SIGINT', () => {
  logger.info('SIGINT received. Closing server...');
  server.close(() => {
    logger.info('Server closed.');
    process.exit(0);
  });
});