# TODO List

List of task to implement or improve

## General
 - [x] use std::invoke for calling operations

## Basic Operations
 - [ ] initializator to produce tags
 - [ ] case where scaling does not change the ordering
 - [x] preparation step for global scaling (e.g. sorting for ranking based scaling)

## Statistics Tracking
 - [ ] refreshing statistics (e.g. before/after global scaling operation)
 - [ ] optimize summation stats (simple summing for integers)
 - [ ] statistic that counts operation effects and timings (mutations/crossovers/replacements)
 - [x] split average and sum statistics

# Fitness
 - [ ] real number vs integer fitness
 - [ ] fitness that can be used for defining uniform random distribution
