# TODO List

List of task to implement or improve

## General
 - [x] use std::invoke for calling operations

## Basic Operations
 - [ ] initializator to produce tags
 - [ ] verify population size for selection/replacement operations
 - [ ] constraints on fitness types for scaling
 - [x] case where scaling does not change the ordering
 - [x] preparation step for global scaling (e.g. sorting for ranking based scaling)

## Statistics Tracking
 - [ ] refreshing statistics (e.g. before/after global scaling operation)
 - [ ] move statistical values tags from statistics header
 - [x] statistic that counts operation effects and timings (mutations/crossovers/replacements)
 - [x] optimize summation stats (simple summing for integers)
 - [x] split average and sum statistics

# Fitness
 - [x] fitness that can be used for defining uniform random distribution
