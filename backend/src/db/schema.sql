-- =========================================================================
-- Multi-Sequence Analysis Engine - PostgreSQL Schema
-- =========================================================================
-- This file is idempotent: it can be re-run safely thanks to
-- "IF NOT EXISTS" guards and is also executed programmatically by
-- server/src/db/migrate.js.
-- =========================================================================

CREATE TABLE IF NOT EXISTS comparisons (
    id                  BIGSERIAL PRIMARY KEY,
    algorithm           VARCHAR(32)  NOT NULL CHECK (algorithm IN
                            ('lcs', 'edit-distance', 'needleman-wunsch', 'smith-waterman')),
    sequence_type       VARCHAR(16)  NOT NULL CHECK (sequence_type IN
                            ('text', 'source-code', 'dna', 'protein')),
    sequence_a          TEXT         NOT NULL,
    sequence_b          TEXT         NOT NULL,
    sequence_a_length   INTEGER      NOT NULL,
    sequence_b_length   INTEGER      NOT NULL,
    score               BIGINT       NOT NULL,
    aligned_a           TEXT,
    aligned_b           TEXT,
    result_sequence     TEXT,
    execution_time_ms   DOUBLE PRECISION NOT NULL,
    peak_memory_kb      BIGINT       NOT NULL,
    match_score         INTEGER,
    mismatch_penalty    INTEGER,
    gap_penalty         INTEGER,
    created_at          TIMESTAMPTZ  NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_comparisons_algorithm   ON comparisons (algorithm);
CREATE INDEX IF NOT EXISTS idx_comparisons_created_at  ON comparisons (created_at DESC);
CREATE INDEX IF NOT EXISTS idx_comparisons_seq_type    ON comparisons (sequence_type);

CREATE TABLE IF NOT EXISTS benchmarks (
    id                  BIGSERIAL PRIMARY KEY,
    algorithm           VARCHAR(32)  NOT NULL CHECK (algorithm IN
                            ('lcs', 'edit-distance', 'needleman-wunsch', 'smith-waterman')),
    sequence_type       VARCHAR(16)  NOT NULL CHECK (sequence_type IN
                            ('text', 'source-code', 'dna', 'protein')),
    input_length        INTEGER      NOT NULL,
    execution_time_ms   DOUBLE PRECISION NOT NULL,
    peak_memory_kb      BIGINT       NOT NULL,
    run_label           VARCHAR(64),
    created_at          TIMESTAMPTZ  NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_benchmarks_algorithm    ON benchmarks (algorithm);
CREATE INDEX IF NOT EXISTS idx_benchmarks_run_label    ON benchmarks (run_label);
CREATE INDEX IF NOT EXISTS idx_benchmarks_created_at   ON benchmarks (created_at DESC);

COMMENT ON TABLE comparisons IS 'Stores every individual sequence comparison request and its result.';
COMMENT ON TABLE benchmarks  IS 'Stores aggregated benchmark run data points across increasing sequence lengths.';