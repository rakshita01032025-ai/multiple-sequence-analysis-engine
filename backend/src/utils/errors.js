'use strict';

// Base class for all operational (expected) errors. Operational errors are
// safe to expose to the client with a clear message and an appropriate HTTP
// status code, as opposed to programmer errors / bugs.
class AppError extends Error {
  constructor(message, statusCode = 500) {
    super(message);
    this.name = this.constructor.name;
    this.statusCode = statusCode;
    this.isOperational = true;
    Error.captureStackTrace(this, this.constructor);
  }
}

class ValidationError extends AppError {
  constructor(message) {
    super(message, 400);
  }
}

class NotFoundError extends AppError {
  constructor(message = 'Resource not found') {
    super(message, 404);
  }
}

class EngineExecutionError extends AppError {
  constructor(message) {
    super(message, 502);
  }
}

class DatabaseError extends AppError {
  constructor(message) {
    super(message, 500);
  }
}

module.exports = {
  AppError,
  ValidationError,
  NotFoundError,
  EngineExecutionError,
  DatabaseError,
};