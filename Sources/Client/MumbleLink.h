#pragma once

#include <memory>

#include "Player.h"

namespace spades {
    struct MumbleLinkedMemory;
    struct MumbleLinkPrivate;

    class MumbleLink {
        const float metre_per_block;
        MumbleLinkedMemory *mumbleLinkedMemory;
        std::unique_ptr<MumbleLinkPrivate> priv;

        void set_mumble_vector3(float mumble_vec[3], const spades::Vector3 &vec);

    public:
        MumbleLink();
        ~MumbleLink();

        bool init();
        void setContext(const std::string &context);
        void setIdentity(const std::string &identity);
        void update(spades::client::Player *player);
    };
}
