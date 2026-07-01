'use strict';

const { spawn } = require('child_process');
const path = require('path');
const config = require('../config');
const logger = require('../utils/logger');
const { EngineExecutionError } = require('../utils/errors');

// EngineBridge is the single point of contact between the Node.js layer and
// the compiled C++ binary. It owns process spawning, stdin/stdout framing,
// timeouts, and error translation -- nothing else in the codebase should
// shell out to the engine directly (single responsibility).
class EngineBridge {
  constructor() {
    this.binaryPath = path.resolve(process.cwd(), config.engine.binaryPath);
  }

  /**
   * Runs the C++ engine with the given request payload.
   * @param {object} request - { mode, sequenceA, sequenceB, sequenceType, matchScore, mismatchPenalty, gapPenalty }
   * @returns {Promise<object>} parsed engine result
   */
  run(request) {
    return new Promise((resolve, reject) => {
      const child = spawn(this.binaryPath, [], {
        stdio: ['pipe', 'pipe', 'pipe'],
      });

      let stdout = '';
      let stderr = '';
      let settled = false;

      const timer = setTimeout(() => {
        if (!settled) {
          settled = true;
          child.kill('SIGKILL');
          reject(new EngineExecutionError('Engine execution timed out'));
        }
      }, config.engine.timeoutMs);

      child.stdout.on('data', (chunk) => {
        stdout += chunk.toString('utf8');
      });

      child.stderr.on('data', (chunk) => {
        stderr += chunk.toString('utf8');
      });

      child.on('error', (err) => {
        if (settled) return;
        settled = true;
        clearTimeout(timer);
        logger.error('Failed to spawn engine process', { error: err.message, binaryPath: this.binaryPath });
        reject(new EngineExecutionError(`Could not start engine binary at ${this.binaryPath}. Did you run "npm run build:engine"?`));
      });

      child.on('close', (code) => {
        if (settled) return;
        settled = true;
        clearTimeout(timer);

        if (code !== 0 && !stdout.trim()) {
          logger.error('Engine process exited with error', { code, stderr });
          reject(new EngineExecutionError(`Engine process failed (exit code ${code}): ${stderr || 'unknown error'}`));
          return;
        }

        try {
          const parsed = JSON.parse(stdout.trim());
          if (parsed.error) {
            reject(new EngineExecutionError(parsed.error));
            return;
          }
          resolve(parsed);
        } catch (parseErr) {
          logger.error('Failed to parse engine output', { stdout, stderr, error: parseErr.message });
          reject(new EngineExecutionError('Engine returned malformed output'));
        }
      });

      // Write the request as a single line of JSON and close stdin.
      child.stdin.write(JSON.stringify(request));
      child.stdin.end();
    });
  }
}

module.exports = new EngineBridge();