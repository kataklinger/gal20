# TODO List

List of task to implement or improve

## General
 - [ ] improve tagging system
 - [ ] individual grouping
 - [x] use std::invoke for calling operations

## Basic Operations
 - [ ] disable improving-only mutation when fitness is not totally ordered
 - [ ] initializator to produce tags
 - [ ] verify duplicates parents for parent replacement
 - [ ] roulette wheele selection when minimizing fintess
 - [ ] replacement with forbidden regions
 - [x] insert-only replacement
 - [x] elitism parameter for random replacement
 - [x] constraints on fitness types for scaling
 - [x] verify population size for selection/replacement operations
 - [x] case where scaling does not change the ordering
 - [x] preparation step for global scaling (e.g. sorting for ranking based scaling)

## Statistics Tracking
 - [ ] refreshing statistics (e.g. before/after global scaling operation)
 - [x] move statistical values tags from statistics header
 - [x] statistic that counts operation effects and timings (mutations/crossovers/replacements)
 - [x] optimize summation stats (simple summing for integers)
 - [x] split average and sum statistics

# Fitness
 - [x] fitness that can be used for defining uniform random distribution
