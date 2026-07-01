'use strict';

const engineBridge = require('./engineBridge');
const benchmarkRepository = require('../db/benchmarkRepository');
const { generateSequence, mutateSequence } = require('./sequenceGenerator');
const logger = require('../utils/logger');
const { ValidationError } = require('../utils/errors');

const ALGORITHM_TO_MODE = {
  lcs: 'lcs',
  'edit-distance': 'edit-distance',
  'needleman-wunsch': 'needleman-wunsch',
  'smith-waterman': 'smith-waterman',
};

const DEFAULT_LENGTH_STEPS = [100, 250, 500, 1000, 1500, 2000, 2500];

class BenchmarkService {
  /**
   * Runs the requested algorithm(s) across a series of sequence lengths
   * (default up to 2500 characters), persists each data point, and returns
   * the full set of results -- suitable for plotting time/memory vs length.
   */
  async runBenchmark({ algorithms, sequenceType = 'dna', lengths = DEFAULT_LENGTH_STEPS, runLabel }) {
    if (!Array.isArray(algorithms) || algorithms.length === 0) {
      throw new ValidationError('algorithms must be a non-empty array');
    }
    for (const algo of algorithms) {
      if (!ALGORITHM_TO_MODE[algo]) {
        throw new ValidationError(`Unknown algorithm: ${algo}`);
      }
    }
    if (!Array.isArray(lengths) || lengths.some((l) => l <= 0 || l > 2500)) {
      throw new ValidationError('lengths must be positive integers no greater than 2500');
    }

    const label = runLabel || `run-${Date.now()}`;
    const allRecords = [];

    for (const algorithm of algorithms) {
      for (const length of lengths) {
        const sequenceA = generateSequence(sequenceType, length);
        const sequenceB = mutateSequence(sequenceA, sequenceType, 0.15);

        const engineResult = await engineBridge.run({
          mode: ALGORITHM_TO_MODE[algorithm],
          sequenceA,
          sequenceB,
          sequenceType,
        });

        allRecords.push({
          algorithm,
          sequenceType,
          inputLength: length,
          executionTimeMs: engineResult.executionTimeMs,
          peakMemoryKb: engineResult.peakMemoryKb,
          runLabel: label,
        });

        logger.info('Benchmark point captured', {
          algorithm,
          length,
          executionTimeMs: engineResult.executionTimeMs,
        });
      }
    }

    const saved = await benchmarkRepository.insertMany(allRecords);
    return { runLabel: label, points: saved.map(this._toApiShape) };
  }

  async getResults({ algorithm, runLabel, limit }) {
    const rows = await benchmarkRepository.findAll({ algorithm, runLabel, limit });
    return rows.map(this._toApiShape);
  }

  _toApiShape(row) {
    return {
      id: row.id,
      algorithm: row.algorithm,
      sequenceType: row.sequence_type,
      inputLength: row.input_length,
      executionTimeMs: Number(row.execution_time_ms),
      peakMemoryKb: Number(row.peak_memory_kb),
      runLabel: row.run_label,
      createdAt: row.created_at,
    };
  }
}

module.exports = new BenchmarkService();