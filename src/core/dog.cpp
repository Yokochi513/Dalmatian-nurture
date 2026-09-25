#include "core/dog.hpp"

#include <utility>

namespace dal::core {

DogState new_dog(std::string name)
{
    DogState state;
    state.name = std::move(name);
    state.needs = Needs{
        .hunger = 60.0,
        .exercise = 50.0,
        .boredom = 60.0,
        .loneliness = 60.0,
        .sleepiness = 20.0,
    };
    state.affection = 20.0;
    return state;
}

} // namespace dal::core
