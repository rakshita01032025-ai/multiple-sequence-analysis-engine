'use strict';

// Generates synthetic sequences of a requested length for each supported
// sequence type, used by the benchmark runner. Keeping this in its own
// module makes the data generation strategy easy to audit and reuse from
// tests or the sample-data script.

const DNA_ALPHABET = 'ACGT';
const PROTEIN_ALPHABET = 'ACDEFGHIKLMNPQRSTVWY';
const TEXT_WORDS = [
  'the', 'quick', 'brown', 'fox', 'jumps', 'over', 'lazy', 'dog', 'while',
  'analyzing', 'sequences', 'across', 'multiple', 'biological', 'and',
  'textual', 'domains', 'with', 'great', 'precision', 'speed',
];
const CODE_TOKENS = [
  'function', 'const', 'let', 'return', 'if', 'else', 'for', 'while',
  '(', ')', '{', '}', ';', '=>', '===', 'true', 'false', 'null',
  'array', 'index', 'value', 'result', 'compute', 'data',
];

function randomChoice(arr) {
  return arr[Math.floor(Math.random() * arr.length)];
}

function generateDna(length) {
  let s = '';
  while (s.length < length) s += randomChoice(DNA_ALPHABET);
  return s.slice(0, length);
}

function generateProtein(length) {
  let s = '';
  while (s.length < length) s += randomChoice(PROTEIN_ALPHABET);
  return s.slice(0, length);
}

function generateText(length) {
  let s = '';
  while (s.length < length) s += randomChoice(TEXT_WORDS) + ' ';
  return s.slice(0, length);
}

function generateSourceCode(length) {
  let s = '';
  while (s.length < length) s += randomChoice(CODE_TOKENS) + ' ';
  return s.slice(0, length);
}

function generateSequence(sequenceType, length) {
  switch (sequenceType) {
    case 'dna':
      return generateDna(length);
    case 'protein':
      return generateProtein(length);
    case 'source-code':
      return generateSourceCode(length);
    case 'text':
    default:
      return generateText(length);
  }
}

/**
 * Produces a mutated copy of a sequence (used to create a "sequenceB" that
 * is related to, but not identical to, sequenceA -- this is more realistic
 * for benchmarking alignment algorithms than two fully independent random
 * strings, which tend to share almost nothing in common).
 */
function mutateSequence(sequence, sequenceType, mutationRate = 0.1) {
  const alphabet =
    sequenceType === 'dna' ? DNA_ALPHABET : sequenceType === 'protein' ? PROTEIN_ALPHABET : null;

  if (!alphabet) {
    // For text/source-code, just shuffle word order slightly via swaps.
    const chars = sequence.split('');
    const swaps = Math.floor(chars.length * mutationRate);
    for (let i = 0; i < swaps; i++) {
      const a = Math.floor(Math.random() * chars.length);
      const b = Math.floor(Math.random() * chars.length);
      [chars[a], chars[b]] = [chars[b], chars[a]];
    }
    return chars.join('');
  }

  const chars = sequence.split('');
  const mutations = Math.floor(chars.length * mutationRate);
  for (let i = 0; i < mutations; i++) {
    const pos = Math.floor(Math.random() * chars.length);
    chars[pos] = randomChoice(alphabet);
  }
  return chars.join('');
}

module.exports = { generateSequence, mutateSequence };