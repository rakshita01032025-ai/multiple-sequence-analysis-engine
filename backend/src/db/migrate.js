'use strict';

// Simple, dependency-free migration runner: applies sql/schema.sql and
// optionally sql/seed.sql. For a larger project this would be replaced by a
// proper migration framework (node-pg-migrate, Flyway, etc.), but a single
// idempotent schema file is appropriate and transparent for this project's
// scope.

const fs = require('fs');
const path = require('path');
const { pool } = require('./pool');
const logger = require('../utils/logger');

async function runFile(filePath) {
  const sql = fs.readFileSync(filePath, 'utf8');
  await pool.query(sql);
  logger.info(`Applied SQL file: ${path.basename(filePath)}`);
}

async function migrate() {
  try {
    const schemaPath = path.resolve(__dirname, '../../../sql/schema.sql');
    await runFile(schemaPath);

    if (process.argv.includes('--seed')) {
      const seedPath = path.resolve(__dirname, '../../../sql/seed.sql');
      await runFile(seedPath);
    }

    logger.info('Migration complete.');
    await pool.end();
    process.exit(0);
  } catch (err) {
    logger.error('Migration failed', { error: err.message });
    await pool.end();
    process.exit(1);
  }
}

migrate();