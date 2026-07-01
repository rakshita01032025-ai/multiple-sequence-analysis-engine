'use strict';

const comparisonService = require('../services/comparisonService');

async function runComparison(req, res, next) {
    try {
        const result = await comparisonService.compare(req.body);
        res.status(200).json(result);
    } catch (err) {
        next(err);
    }
}
module.exports = {
    runComparison
};