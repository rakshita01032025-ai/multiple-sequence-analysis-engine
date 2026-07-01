'use strict';

const engineBridge = require('./engineBridge');
const comparisonRepository = require('../db/comparisonRepository');
const logger = require('../utils/logger');

// Maps our public-facing algorithm names to the engine's CLI "mode" values.
const ALGORITHM_TO_MODE = {
  lcs: 'lcs',
  'edit-distance': 'edit-distance',
  'needleman-wunsch': 'needleman-wunsch',
  'smith-waterman': 'smith-waterman',
};

/**
 * ComparisonService coordinates between the C++ engine (via EngineBridge)
 * and the persistence layer (via ComparisonRepository). Controllers should
 * only ever talk to this service, never to the engine or DB directly --
 * this keeps the HTTP layer thin and the business logic testable in
 * isolation from Express.
 */
class ComparisonService {
  async runAlgorithm(algorithm, { sequenceA, sequenceB, sequenceType, scoring = {} }, { persist = true } = {}) {
    const mode = ALGORITHM_TO_MODE[algorithm];

    const engineRequest = {
      mode,
      sequenceA,
      sequenceB,
      sequenceType,
      matchScore: scoring.matchScore,
      mismatchPenalty: scoring.mismatchPenalty,
      gapPenalty: scoring.gapPenalty,
    };

    const engineResult = await engineBridge.run(engineRequest);

    const record = {
      algorithm,
      sequenceType,
      sequenceA,
      sequenceB,
      score: engineResult.score,
      alignedA: engineResult.alignedA,
      alignedB: engineResult.alignedB,
      resultSequence: engineResult.resultSequence,
      executionTimeMs: engineResult.executionTimeMs,
      peakMemoryKb: engineResult.peakMemoryKb,
      matchScore: scoring.matchScore,
      mismatchPenalty: scoring.mismatchPenalty,
      gapPenalty: scoring.gapPenalty,
    };

    let saved = null;
    if (persist) {
      saved = await comparisonRepository.insert(record);
    }

    logger.info('Algorithm executed', {
      algorithm,
      sequenceType,
      lengthA: sequenceA.length,
      lengthB: sequenceB.length,
      executionTimeMs: engineResult.executionTimeMs,
    });

    return saved
      ? this._toApiShape(saved)
      : this._toApiShape({ ...record, id: null, created_at: new Date().toISOString() });
  }

  /**
   * Runs all four algorithms against the same pair of sequences. Used by the
   * "compare" endpoint to give a holistic view across LCS, Edit Distance,
   * Needleman-Wunsch, and Smith-Waterman in one call.
   */
  async compareAll(payload) {
    const algorithms = Object.keys(ALGORITHM_TO_MODE);
    const results = await Promise.all(
      algorithms.map((algorithm) => this.runAlgorithm(algorithm, payload))
    );

    return algorithms.reduce((acc, algorithm, idx) => {
      acc[algorithm] = results[idx];
      return acc;
    }, {});
  }

  async getHistory({ limit, offset, algorithm, sequenceType }) {
    const [rows, total] = await Promise.all([
      comparisonRepository.findRecent({ limit, offset, algorithm, sequenceType }),
      comparisonRepository.countAll(),
    ]);
    return {
      total,
      limit,
      offset,
      results: rows.map((row) => this._toApiShape(row)),
    };
  }

  async getById(id) {
    const row = await comparisonRepository.findById(id);
    return row ? this._toApiShape(row) : null;
  }

  _toApiShape(row) {
    return {
      id: row.id,
      algorithm: row.algorithm,
      sequenceType: row.sequenceType || row.sequence_type,
      sequenceA: row.sequenceA || row.sequence_a,
      sequenceB: row.sequenceB || row.sequence_b,
      sequenceALength: row.sequenceA ? row.sequenceA.length : row.sequence_a_length,
      sequenceBLength: row.sequenceB ? row.sequenceB.length : row.sequence_b_length,
      score: Number(row.score),
      alignedA: row.alignedA ?? row.aligned_a ?? null,
      alignedB: row.alignedB ?? row.aligned_b ?? null,
      resultSequence: row.resultSequence ?? row.result_sequence ?? null,
      executionTimeMs: Number(row.executionTimeMs ?? row.execution_time_ms),
      peakMemoryKb: Number(row.peakMemoryKb ?? row.peak_memory_kb),
      scoring: {
        matchScore: row.matchScore ?? row.match_score ?? null,
        mismatchPenalty: row.mismatchPenalty ?? row.mismatch_penalty ?? null,
        gapPenalty: row.gapPenalty ?? row.gap_penalty ?? null,
      },
      createdAt: row.created_at || row.createdAt,
    };
  }
}

module.exports = new ComparisonService();