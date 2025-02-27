#pragma once

#include <cassert>
#include <memory>
#include <initializer_list>

// Note that the functions in this header are NOT thread-safe, unlike their counterparts in libc.

class RandomEngine;

/**
 * @return                              Currently used global random engine.
 */
RandomEngine *GlobalRandomEngine();

/**
 * @param engine                        New global random engine to use for `Random` and other functions.
 *                                      This function takes ownership of the provided random engine.
 * @returns                             Old random engine.
 */
std::unique_ptr<RandomEngine> SetGlobalRandomEngine(std::unique_ptr<RandomEngine> engine);

/**
 * Reseeds the global random engine.
 *
 * @param seed                          Seed to use.
 */
void SeedRandom(int seed);

/**
 * @param hi                            Upper bound for the result. Must be greater than zero.
 * @return                              Random number in range `[0, hi)`, generated using the global random engine.
 *                                      Note that `hi` is not included in the range.
 */
int Random(int hi);

/**
 * @param min                           Minimal result value.
 * @param max                           Maximal result value. Must be greater or equal to `min`.
 * @return                              Random number in closed interval `[min, max]`, generated using the global
 *                                      random engine. Note that both `min` and `max` are included in the interval.
 */
int RandomInSegment(int min, int max);

/**
 * @return                              Random floating point number in range `[0, 1)`, generated using the global
 *                                      random engine.
 */
float RandomFloat();

/**
 * @offset 0x00452B2E
 *
 * @param count                         Number of dice to throw.
 * @param faces                         Number of faces on each die.
 * @return                              Result of throwing the dice, generated using the global random engine.
 */
int RandomDice(int count, int faces);

// TODO(captainurist): rename to RandomSample
/**
 * @param range                         Random access range.
 * @return                              Random element from the provided range.
 */
template<class Range, class T = typename Range::value_type>
T Sample(const Range &range) {
    using std::begin;
    using std::end;

    auto b = begin(range);
    auto e = end(range);
    size_t size = e - b; // Assume random access iterators.

    assert(size > 0);

    return *(b + Random(size));
}

template<class T>
T Sample(std::initializer_list<T> range) {
    return Sample<std::initializer_list<T>, T>(range);
}
