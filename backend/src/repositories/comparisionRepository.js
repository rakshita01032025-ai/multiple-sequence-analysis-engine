'use strict';

const { query } = require('./pool');
const { DatabaseError } = require('../utils/errors');

// Repository pattern: this is the only module that knows SQL for the
// "comparisons" table. Services depend on this interface, not on raw SQL,
// which keeps persistence concerns isolated and swappable.
class ComparisonRepository {
  async insert(record) {
    try {
      const result = await query(
        `INSERT INTO comparisons
          (algorithm, sequence_type, sequence_a, sequence_b, sequence_a_length, sequence_b_length,
           score, aligned_a, aligned_b, result_sequence, execution_time_ms, peak_memory_kb,
           match_score, mismatch_penalty, gap_penalty)
         VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15)
         RETURNING *`,
        [
          record.algorithm,
          record.sequenceType,
          record.sequenceA,
          record.sequenceB,
          record.sequenceA.length,
          record.sequenceB.length,
          record.score,
          record.alignedA || null,
          record.alignedB || null,
          record.resultSequence || null,
          record.executionTimeMs,
          record.peakMemoryKb,
          record.matchScore ?? null,
          record.mismatchPenalty ?? null,
          record.gapPenalty ?? null,
        ]
      );
      return result.rows[0];
    } catch (err) {
      throw new DatabaseError(`Failed to insert comparison: ${err.message}`);
    }
  }

  async findRecent({ limit = 50, offset = 0, algorithm = null, sequenceType = null }) {
    try {
      const conditions = [];
      const params = [];
      let idx = 1;

      if (algorithm) {
        conditions.push(`algorithm = $${idx++}`);
        params.push(algorithm);
      }
      if (sequenceType) {
        conditions.push(`sequence_type = $${idx++}`);
        params.push(sequenceType);
      }

      const whereClause = conditions.length ? `WHERE ${conditions.join(' AND ')}` : '';
      params.push(limit, offset);

      const result = await query(
        `SELECT * FROM comparisons
         ${whereClause}
         ORDER BY created_at DESC
         LIMIT $${idx++} OFFSET $${idx++}`,
        params
      );
      return result.rows;
    } catch (err) {
      throw new DatabaseError(`Failed to fetch comparison history: ${err.message}`);
    }
  }

  async countAll() {
    try {
      const result = await query('SELECT COUNT(*)::int AS count FROM comparisons');
      return result.rows[0].count;
    } catch (err) {
      throw new DatabaseError(`Failed to count comparisons: ${err.message}`);
    }
  }

  async findById(id) {
    try {
      const result = await query('SELECT * FROM comparisons WHERE id = $1', [id]);
      return result.rows[0] || null;
    } catch (err) {
      throw new DatabaseError(`Failed to fetch comparison ${id}: ${err.message}`);
    }
  }
}

module.exports = new ComparisonRepository();