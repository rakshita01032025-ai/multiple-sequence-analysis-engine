'use strict';

const logger = require('../utils/logger');
const { AppError } = require('../utils/errors');

/**
 * errorHandler
 * ============
 * Centralized Express error-handling middleware (must be registered LAST,
 * after all routes, with exactly four parameters so Express recognises it
 * as an error handler rather than a regular middleware).
 *
 * Strategy
 * --------
 *  - Operational errors (AppError subclasses): safe to expose the message
 *    and status code directly to the client.
 *  - Programmer / unexpected errors: log the full stack, return a generic
 *    500 so internal details are never leaked to callers.
 *
 * All responses follow the same envelope:
 *   { success: false, error: { message, code } }
 */
// eslint-disable-next-line no-unused-vars
function errorHandler(err, req, res, next) {
  // Preserve any status code already set on the response (e.g. from
  // res.status(400).json(...) patterns elsewhere) then fall back to the
  // error's own statusCode, then 500.
  const statusCode =
    res.statusCode && res.statusCode !== 200
      ? res.statusCode
      : err.statusCode || 500;

  const isOperational = err.isOperational === true;

  // Always log the error; use 'warn' for expected operational errors and
  // 'error' for unexpected ones so alert thresholds can be tuned.
  if (isOperational) {
    logger.warn('Operational error', {
      name: err.name,
      message: err.message,
      statusCode,
      path: req.path,
      method: req.method,
    });
  } else {
    logger.error('Unexpected error', {
      name: err.name,
      message: err.message,
      stack: err.stack,
      path: req.path,
      method: req.method,
    });
  }

  // Map AppError subclass names to short machine-readable codes that clients
  // can branch on without string-parsing the human message.
  const CODE_MAP = {
    ValidationError: 'VALIDATION_ERROR',
    NotFoundError: 'NOT_FOUND',
    EngineExecutionError: 'ENGINE_ERROR',
    DatabaseError: 'DATABASE_ERROR',
    AppError: 'INTERNAL_ERROR',
  };

  const code = CODE_MAP[err.name] || 'INTERNAL_ERROR';

  res.status(statusCode).json({
    success: false,
    error: {
      message: isOperational ? err.message : 'An unexpected internal error occurred.',
      code,
    },
  });
}

module.exports = errorHandler;