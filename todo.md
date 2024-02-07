# TODO List

List of task to implement or improve

## General
 - [ ] do not share distribution object (crossover operations)
 - [x] handle NaNs for minimization floating point three way comparison
 - [x] individual grouping
 - [x] improve tagging system
 - [x] use std::invoke for calling operations

## Basic Operations
 - [ ] disable improving-only mutation when fitness is not totally ordered
 - [ ] initializator to produce tags
 - [ ] verify duplicates parents for parent replacement
 - [ ] roulette wheele selection when minimizing fitness
 - [ ] replacement with forbidden regions
 - [ ] optimize linkage clustering to recalculate only merged clusters
 - [ ] add pruning operation that removes the worst parents and offsprings (paes)
 - [ ] substractive translate projection
 - [ ] optimize field coupling by creating slot for each parent (avoid sorting)
 - [x] tournament selection (random, roulette wheel)
 - [x] selection of hypercells rather than individuals
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

# Pareto
 - [x] skip comparing dominant solution from the previous gen. between themselves
 - [x] propagate the number of solutions that are required to be ranked by later operations
 - [x] preserve pareto front to which the solution belongs
