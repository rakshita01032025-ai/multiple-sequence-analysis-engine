-- Optional sample data to populate the dashboard on a fresh install.
-- Safe to run multiple times; relies on application logic for true history,
-- this is purely illustrative seed data.

INSERT INTO comparisons
    (algorithm, sequence_type, sequence_a, sequence_b, sequence_a_length, sequence_b_length,
     score, aligned_a, aligned_b, result_sequence, execution_time_ms, peak_memory_kb,
     match_score, mismatch_penalty, gap_penalty)
VALUES
    ('lcs', 'text', 'the quick brown fox', 'the slow brown ox', 19, 17,
     15, NULL, NULL, 'the quick brown ox', 0.045, 2100, NULL, NULL, NULL),

    ('edit-distance', 'source-code', 'function add(a,b){return a+b;}', 'function add(a, b) { return a + b; }', 31, 37,
     7, NULL, NULL, NULL, 0.061, 2150, NULL, NULL, NULL),

    ('needleman-wunsch', 'dna', 'GATTACA', 'GCATGCU', 7, 7,
     0, 'GAT-TACA', 'G-CATGCU', NULL, 0.012, 1980, 1, -1, -2),

    ('smith-waterman', 'protein', 'HEAGAWGHEE', 'PAWHEAE', 10, 7,
     11, 'AWGHE', 'AW-HE', NULL, 0.020, 2010, 2, -1, -2)
ON CONFLICT DO NOTHING;

INSERT INTO benchmarks (algorithm, sequence_type, input_length, execution_time_ms, peak_memory_kb, run_label)
VALUES
    ('lcs', 'dna', 500, 1.2, 2200, 'seed-run'),
    ('lcs', 'dna', 1000, 4.8, 2600, 'seed-run'),
    ('lcs', 'dna', 2500, 28.4, 4100, 'seed-run'),
    ('edit-distance', 'dna', 500, 1.1, 2150, 'seed-run'),
    ('edit-distance', 'dna', 1000, 4.5, 2550, 'seed-run'),
    ('edit-distance', 'dna', 2500, 27.9, 4050, 'seed-run'),
    ('needleman-wunsch', 'protein', 500, 1.6, 2300, 'seed-run'),
    ('needleman-wunsch', 'protein', 1000, 6.1, 2700, 'seed-run'),
    ('needleman-wunsch', 'protein', 2500, 35.2, 4300, 'seed-run'),
    ('smith-waterman', 'protein', 500, 1.7, 2320, 'seed-run'),
    ('smith-waterman', 'protein', 1000, 6.4, 2750, 'seed-run'),
    ('smith-waterman', 'protein', 2500, 36.0, 4400, 'seed-run')
ON CONFLICT DO NOTHING;