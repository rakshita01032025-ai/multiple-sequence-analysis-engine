'use strict';

const express = require('express');
const cors = require('cors');

const comparisonRoutes = require('./src/routes/comparisonRoutes');
const benchmarkRoutes = require('./src/routes/benchmarkRoutes');
const historyRoutes = require('./src/routes/historyRoutes');
const logger = require('./src/utils/logger');

const app = express();

app.use(cors());
app.use(express.json({ limit: '2mb' }));

app.get('/health', (req, res) => {
  res.json({
    status: 'ok',
    service: 'multi-sequence-analysis-engine',
    timestamp: new Date().toISOString()
  });
});

app.use('/api/compare', comparisonRoutes);
app.use('/api/benchmark', benchmarkRoutes);
app.use('/api/history', historyRoutes);

app.use((req, res) => {
  res.status(404).json({
    error: 'Route not found'
  });
});

app.use((err, req, res, next) => {
  logger.error('Unhandled error', {
    message: err.message,
    stack: err.stack
  });

  res.status(err.statusCode || 500).json({
    error: err.message || 'Internal server error'
  });
});

module.exports = app;