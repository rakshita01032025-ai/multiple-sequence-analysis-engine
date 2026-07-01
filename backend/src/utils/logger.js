'use strict';

const winston = require('winston');
const config = require('../config');

// A single shared logger instance. JSON output in production for log
// aggregation systems; human-readable colorized output in development.
const logger = winston.createLogger({
  level: config.env === 'production' ? 'info' : 'debug',
  format: winston.format.combine(
    winston.format.timestamp(),
    winston.format.errors({ stack: true }),
    config.env === 'production'
      ? winston.format.json()
      : winston.format.combine(
          winston.format.colorize(),
          winston.format.printf(({ timestamp, level, message, stack, ...meta }) => {
            const metaStr = Object.keys(meta).length ? ` ${JSON.stringify(meta)}` : '';
            return `${timestamp} [${level}] ${stack || message}${metaStr}`;
          })
        )
  ),
  transports: [new winston.transports.Console()],
});

module.exports = logger;