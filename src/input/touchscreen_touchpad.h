// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>

namespace Input {

struct TouchpadClickState {
    bool manual_down{};
    bool mouse_down{};
    bool two_finger_click{};

    bool Pressed(int screen_contacts) const {
        return manual_down || (mouse_down && screen_contacts == 0) ||
               (two_finger_click && screen_contacts == 2);
    }
};

// SDL-independent contact tracking. Only DOWN allocates a slot, so a third finger
// cannot become a new contact halfway through its gesture when another lifts.
class TouchscreenTouchpad {
public:
    enum class Action { Down, Motion, Up };
    struct Update {
        int slot;
        bool down;
        float x;
        float y;
    };

    int ContactCount() const {
        return static_cast<int>(contacts[0].down) + static_cast<int>(contacts[1].down);
    }

    bool CanStartContact(bool captured, bool focused) const {
        // UI capture is decided for the gesture's first contact. A synthetic
        // mouse-down from that contact must not steal its second finger.
        return focused && (!captured || ContactCount() != 0);
    }

    std::optional<Update> Handle(Action action, std::uint64_t device, std::uint64_t finger,
                                 float x, float y) {
        Contact* contact = nullptr;
        for (auto& entry : contacts) {
            if (entry.down && entry.device == device && entry.finger == finger) {
                contact = &entry;
                break;
            }
        }
        if (!contact && action == Action::Down && std::isfinite(x) && std::isfinite(y)) {
            for (auto& entry : contacts) {
                if (!entry.down) {
                    entry = {.device = device, .finger = finger, .down = true};
                    contact = &entry;
                    break;
                }
            }
        }
        if (!contact) {
            return std::nullopt;
        }
        // Cancellation still releases a contact even if the OS supplies invalid coordinates.
        if (std::isfinite(x) && std::isfinite(y)) {
            contact->x = std::clamp(x, 0.0f, 1.0f);
            contact->y = std::clamp(y, 0.0f, 1.0f);
        }
        contact->down = action != Action::Up;
        return Update{static_cast<int>(contact - contacts.data()), contact->down, contact->x,
                      contact->y};
    }

    std::array<Update, 2> ReleaseAll() {
        std::array<Update, 2> updates{};
        for (int i = 0; i < 2; ++i) {
            updates[i] = {i, false, contacts[i].x, contacts[i].y};
            contacts[i].down = false;
        }
        return updates;
    }

private:
    struct Contact {
        std::uint64_t device{};
        std::uint64_t finger{};
        bool down{};
        float x{};
        float y{};
    };
    std::array<Contact, 2> contacts{};
};

} // namespace Input
