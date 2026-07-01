'use strict';

const express = require('express');
const comparisonController = require('../controllers/comparisonController');

const router = express.Router();

router.post('/lcs', comparisonController.compare);
router.post('/edit-distance', comparisonController.compare);
router.post('/needleman-wunsch', comparisonController.compare);
router.post('/smith-waterman', comparisonController.compare);

module.exports = router;