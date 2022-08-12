# TODO List

List of task to implement or improve

## General
 - [x] use std::invoke for calling operations

## Basic Operations
 - [ ] pass population and statistics to operations as context (operation factories)
 - [ ] initializator to produce tags
 - [x] case where scaling does not change the ordering
 - [x] preparation step for global scaling (e.g. sorting for ranking based scaling)

## Statistics Tracking
 - [ ] refreshing statistics (e.g. before/after global scaling operation)
 - [x] statistic that counts operation effects and timings (mutations/crossovers/replacements)
 - [x] optimize summation stats (simple summing for integers)
 - [x] split average and sum statistics

# Fitness
 - [x] fitness that can be used for defining uniform random distribution
