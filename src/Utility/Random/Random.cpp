#include "Random.h"

#include <memory>
#include <utility>

#include "MersenneTwisterRandomEngine.h"

static constinit std::unique_ptr<RandomEngine> globalRandomEngine;

RandomEngine *GlobalRandomEngine() {
    if (!globalRandomEngine) {
        // We don't guarantee thread safety, so this code is perfectly OK, and it rids us of potential problems with
        // static initialization order.
        globalRandomEngine = std::make_unique<MersenneTwisterRandomEngine>();
    }

    return globalRandomEngine.get();
}

std::unique_ptr<RandomEngine> SetGlobalRandomEngine(std::unique_ptr<RandomEngine> engine) {
    assert(engine);
    std::swap(engine, globalRandomEngine);
    return std::move(engine);
}

void SeedRandom(int seed) {
    GlobalRandomEngine()->Seed(seed);
}

int Random(int hi) {
    assert(hi > 0);

    return GlobalRandomEngine()->Random(hi);
}

int RandomInSegment(int min, int max) {
    assert(max >= min);

    return min + GlobalRandomEngine()->Random(max - min + 1);
}

float RandomFloat() {
    return GlobalRandomEngine()->RandomFloat();
}

int RandomDice(int count, int faces) {
    assert(count >= 0 && faces >= 0);

    if (count == 0 || faces == 0)
        return 0;

    RandomEngine *engine = GlobalRandomEngine();

    int result = 0;
    for (int i = 0; i < count; i++)
        result += 1 + engine->Random(faces);
    return result;
}
