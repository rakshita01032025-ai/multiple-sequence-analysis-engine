'use strict';

const { Pool } = require('pg');
const config = require('../config');
const logger = require('../utils/logger');

// A single shared connection pool for the whole application. Using a pool
// (rather than one client per request) lets pg manage concurrency and
// connection reuse efficiently.
const pool = new Pool({
  host: config.db.host,
  port: config.db.port,
  database: config.db.database,
  user: config.db.user,
  password: config.db.password,
  max: 10,
  idleTimeoutMillis: 30000,
  connectionTimeoutMillis: 5000,
});

pool.on('error', (err) => {
  // Errors on idle clients (e.g. connection dropped by the server) should
  // never crash the process; log and let the pool recover.
  logger.error('Unexpected PostgreSQL pool error', { error: err.message });
});

async function query(text, params) {
  const start = Date.now();
  const result = await pool.query(text, params);
  const durationMs = Date.now() - start;
  logger.debug('Executed query', { text, durationMs, rows: result.rowCount });
  return result;
}

module.exports = { pool, query };