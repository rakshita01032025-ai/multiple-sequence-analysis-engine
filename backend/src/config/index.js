'use strict';

require('dotenv').config();

// Centralizes all environment-derived configuration in one place so the rest
// of the codebase never reads process.env directly. This makes config
// testable, discoverable, and easy to validate at startup.
const config = {
  env: process.env.NODE_ENV || 'development',
  port: parseInt(process.env.PORT, 10) || 3000,

  db: {
    host: process.env.PGHOST || 'localhost',
    port: parseInt(process.env.PGPORT, 10) || 5432,
    database: process.env.PGDATABASE || 'msae',
    user: process.env.PGUSER || 'msae_user',
    password: process.env.PGPASSWORD || '',
  },

  engine: {
    binaryPath: process.env.ENGINE_BINARY_PATH || 'engine/bin/mse_engine',
    timeoutMs: parseInt(process.env.ENGINE_TIMEOUT_MS, 10) || 15000,
  },

  limits: {
    maxSequenceLength: parseInt(process.env.MAX_SEQUENCE_LENGTH, 10) || 2500,
    rateLimitWindowMs: parseInt(process.env.RATE_LIMIT_WINDOW_MS, 10) || 60000,
    rateLimitMaxRequests: parseInt(process.env.RATE_LIMIT_MAX_REQUESTS, 10) || 120,
  },
};

module.exports = config;