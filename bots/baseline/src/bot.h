#pragma once
class Bot {
    unsigned health_shards = 0;
    unsigned damage_shards = 0;
    bool recovering = false;
public:
    void play_turn();
};
