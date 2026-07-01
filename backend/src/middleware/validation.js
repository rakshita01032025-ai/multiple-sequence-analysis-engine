'use strict';

const config = require('../config');
const { ValidationError } = require('../utils/errors');

const VALID_SEQUENCE_TYPES = ['text', 'source-code', 'dna', 'protein'];
const VALID_ALGORITHMS = ['lcs', 'edit-distance', 'needleman-wunsch', 'smith-waterman'];

const DNA_PATTERN = /^[ACGTUNacgtun\s]+$/;
const PROTEIN_PATTERN = /^[ACDEFGHIKLMNPQRSTVWYXBZJUOacdefghiklmnpqrstvwyxbzjuo\s*]+$/;

/**
 * Validates the common comparison request body shared by the LCS, edit
 * distance, Needleman-Wunsch, Smith-Waterman, and compare endpoints.
 * Throws ValidationError with a descriptive message on failure.
 */
function validateComparisonRequest(body) {
  const { sequenceA, sequenceB, sequenceType } = body;

  if (typeof sequenceA !== 'string' || typeof sequenceB !== 'string') {
    throw new ValidationError('sequenceA and sequenceB are required and must be strings');
  }

  if (sequenceA.trim().length === 0 || sequenceB.trim().length === 0) {
    throw new ValidationError('sequenceA and sequenceB must not be empty');
  }

  if (sequenceA.length > config.limits.maxSequenceLength || sequenceB.length > config.limits.maxSequenceLength) {
    throw new ValidationError(
      `Sequence length exceeds the maximum of ${config.limits.maxSequenceLength} characters`
    );
  }

  const type = sequenceType || 'text';
  if (!VALID_SEQUENCE_TYPES.includes(type)) {
    throw new ValidationError(`sequenceType must be one of: ${VALID_SEQUENCE_TYPES.join(', ')}`);
  }

  if (type === 'dna') {
    if (!DNA_PATTERN.test(sequenceA) || !DNA_PATTERN.test(sequenceB)) {
      throw new ValidationError('DNA sequences may only contain characters A, C, G, T, U, N');
    }
  }

  if (type === 'protein') {
    if (!PROTEIN_PATTERN.test(sequenceA) || !PROTEIN_PATTERN.test(sequenceB)) {
      throw new ValidationError('Protein sequences must contain valid amino acid letter codes');
    }
  }

  // Optional scoring overrides for alignment algorithms
  const scoring = {};
  for (const key of ['matchScore', 'mismatchPenalty', 'gapPenalty']) {
    if (body[key] !== undefined) {
      const value = Number(body[key]);
      if (!Number.isInteger(value)) {
        throw new ValidationError(`${key} must be an integer`);
      }
      scoring[key] = value;
    }
  }

  return { sequenceA, sequenceB, sequenceType: type, scoring };
}

function validateAlgorithm(algorithm) {
  if (!VALID_ALGORITHMS.includes(algorithm)) {
    throw new ValidationError(`algorithm must be one of: ${VALID_ALGORITHMS.join(', ')}`);
  }
}

module.exports = {
  validateComparisonRequest,
  validateAlgorithm,
  VALID_SEQUENCE_TYPES,
  VALID_ALGORITHMS,
};