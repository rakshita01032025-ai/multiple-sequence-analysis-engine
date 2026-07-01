'use strict';

const { query } = require('./pool');
const { DatabaseError } = require('../utils/errors');

class BenchmarkRepository {
  async insert(record) {
    try {
      const result = await query(
        `INSERT INTO benchmarks
          (algorithm, sequence_type, input_length, execution_time_ms, peak_memory_kb, run_label)
         VALUES ($1,$2,$3,$4,$5,$6)
         RETURNING *`,
        [
          record.algorithm,
          record.sequenceType,
          record.inputLength,
          record.executionTimeMs,
          record.peakMemoryKb,
          record.runLabel || null,
        ]
      );
      return result.rows[0];
    } catch (err) {
      throw new DatabaseError(`Failed to insert benchmark point: ${err.message}`);
    }
  }

  async insertMany(records) {
    // Inserted sequentially inside a single connection/transaction so a
    // benchmark run is recorded atomically; volume here is small (a handful
    // of length data points per algorithm) so this is simple and sufficient.
    const { pool } = require('./pool');
    const client = await pool.connect();
    try {
      await client.query('BEGIN');
      const inserted = [];
      for (const record of records) {
        const result = await client.query(
          `INSERT INTO benchmarks
            (algorithm, sequence_type, input_length, execution_time_ms, peak_memory_kb, run_label)
           VALUES ($1,$2,$3,$4,$5,$6)
           RETURNING *`,
          [
            record.algorithm,
            record.sequenceType,
            record.inputLength,
            record.executionTimeMs,
            record.peakMemoryKb,
            record.runLabel || null,
          ]
        );
        inserted.push(result.rows[0]);
      }
      await client.query('COMMIT');
      return inserted;
    } catch (err) {
      await client.query('ROLLBACK');
      throw new DatabaseError(`Failed to insert benchmark run: ${err.message}`);
    } finally {
      client.release();
    }
  }

  async findAll({ algorithm = null, runLabel = null, limit = 200 }) {
    try {
      const conditions = [];
      const params = [];
      let idx = 1;

      if (algorithm) {
        conditions.push(`algorithm = $${idx++}`);
        params.push(algorithm);
      }
      if (runLabel) {
        conditions.push(`run_label = $${idx++}`);
        params.push(runLabel);
      }

      const whereClause = conditions.length ? `WHERE ${conditions.join(' AND ')}` : '';
      params.push(limit);

      const result = await query(
        `SELECT * FROM benchmarks
         ${whereClause}
         ORDER BY algorithm, input_length ASC
         LIMIT $${idx++}`,
        params
      );
      return result.rows;
    } catch (err) {
      throw new DatabaseError(`Failed to fetch benchmarks: ${err.message}`);
    }
  }
}

module.exports = new BenchmarkRepository();