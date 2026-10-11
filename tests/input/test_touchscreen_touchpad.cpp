// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <limits>
#include <gtest/gtest.h>
#include "input/touchscreen_touchpad.h"

using Pad = Input::TouchscreenTouchpad;
using Action = Pad::Action;

TEST(TouchscreenTouchpad, TwoFingerClickDoesNotCancelAManualClick) {
    Input::TouchpadClickState click;
    EXPECT_FALSE(click.Pressed(2)); // Contact-only by default.
    click.two_finger_click = true;
    EXPECT_FALSE(click.Pressed(1));
    EXPECT_TRUE(click.Pressed(2));
    click.manual_down = true;
    EXPECT_TRUE(click.Pressed(1)); // Lifting a screen finger must not release a bound button.
    click.manual_down = false;
    EXPECT_TRUE(click.Pressed(2)); // Releasing a bound button must not cancel the screen click.
    EXPECT_FALSE(click.Pressed(0));
    click.two_finger_click = false;
    EXPECT_FALSE(click.Pressed(2));
}

TEST(TouchscreenTouchpad, ContinuousSwipeAndRelease) {
    Pad pad;
    auto start = pad.Handle(Action::Down, 1, 42, 0.2f, 0.8f);
    ASSERT_TRUE(start);
    EXPECT_EQ(start->slot, 0);
    EXPECT_TRUE(start->down);
    auto move = pad.Handle(Action::Motion, 1, 42, 0.8f, 0.2f);
    ASSERT_TRUE(move);
    EXPECT_EQ(move->slot, 0);
    EXPECT_FLOAT_EQ(move->x, 0.8f);
    EXPECT_FLOAT_EQ(move->y, 0.2f);
    auto end = pad.Handle(Action::Up, 1, 42, 0.9f, 0.1f);
    ASSERT_TRUE(end);
    EXPECT_FALSE(end->down);
    EXPECT_FALSE(pad.Handle(Action::Motion, 1, 42, 0.5f, 0.5f));
}

TEST(TouchscreenTouchpad, SecondFingerJoinsGameGestureAfterMouseCaptureChanges) {
    Pad pad;
    EXPECT_FALSE(pad.CanStartContact(true, true)); // First finger belongs to UI.
    EXPECT_TRUE(pad.CanStartContact(false, true));
    pad.Handle(Action::Down, 1, 1, 0.2f, 0.8f);
    EXPECT_EQ(pad.ContactCount(), 1);
    EXPECT_TRUE(pad.CanStartContact(true, true)); // Game already owns this gesture.
    pad.Handle(Action::Down, 1, 2, 0.8f, 0.8f);
    EXPECT_EQ(pad.ContactCount(), 2);
    EXPECT_FALSE(pad.CanStartContact(false, false)); // Focus loss always wins.
    pad.ReleaseAll();
    EXPECT_EQ(pad.ContactCount(), 0);
    EXPECT_FALSE(pad.CanStartContact(true, true)); // New gesture checks UI again.
}

TEST(TouchscreenTouchpad, BothSidesStayHeldAndSwipeUpTogether) {
    Pad pad;
    auto left = pad.Handle(Action::Down, 1, 10, 0.2f, 0.8f);
    auto right = pad.Handle(Action::Down, 1, 20, 0.8f, 0.8f);
    ASSERT_TRUE(left);
    ASSERT_TRUE(right);
    EXPECT_NE(left->slot, right->slot);
    for (int step = 0; step < 10; ++step) {
        const float y = 0.8f - 0.06f * step;
        left = pad.Handle(Action::Motion, 1, 10, 0.2f, y);
        right = pad.Handle(Action::Motion, 1, 20, 0.8f, y);
        ASSERT_TRUE(left);
        ASSERT_TRUE(right);
        EXPECT_TRUE(left->down);
        EXPECT_TRUE(right->down);
        EXPECT_EQ(pad.ContactCount(), 2);
        EXPECT_FLOAT_EQ(left->y, right->y);
    }
    pad.Handle(Action::Up, 1, 10, 0.2f, 0.2f);
    EXPECT_EQ(pad.ContactCount(), 1);
    right = pad.Handle(Action::Motion, 1, 20, 0.8f, 0.1f);
    ASSERT_TRUE(right);
    EXPECT_TRUE(right->down);
    EXPECT_EQ(right->slot, 1);
    pad.Handle(Action::Up, 1, 20, 0.8f, 0.1f);
    EXPECT_EQ(pad.ContactCount(), 0);
}

TEST(TouchscreenTouchpad, StableSlotsAndDeviceIdentity) {
    Pad pad;
    EXPECT_EQ(pad.Handle(Action::Down, 1, 0, 0.1f, 0.2f)->slot, 0);
    EXPECT_EQ(pad.Handle(Action::Down, 2, 0, 0.3f, 0.4f)->slot, 1);
    pad.Handle(Action::Up, 1, 0, 0.1f, 0.2f);
    auto second = pad.Handle(Action::Motion, 2, 0, 0.5f, 0.6f);
    ASSERT_TRUE(second);
    EXPECT_EQ(second->slot, 1);
    EXPECT_EQ(pad.Handle(Action::Down, 1, 1, 0.1f, 0.2f)->slot, 0);
}

TEST(TouchscreenTouchpad, ExtraFingerDoesNotJoinMidGesture) {
    Pad pad;
    pad.Handle(Action::Down, 1, 1, 0.1f, 0.2f);
    pad.Handle(Action::Down, 1, 2, 0.3f, 0.4f);
    EXPECT_FALSE(pad.Handle(Action::Down, 1, 3, 0.5f, 0.6f));
    pad.Handle(Action::Up, 1, 1, 0.1f, 0.2f);
    EXPECT_FALSE(pad.Handle(Action::Motion, 1, 3, 0.7f, 0.8f));
    EXPECT_FALSE(pad.Handle(Action::Up, 1, 3, 0.7f, 0.8f));
}

TEST(TouchscreenTouchpad, ClampCoordinatesAndCancelInvalidContact) {
    Pad pad;
    const float nan = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(pad.Handle(Action::Down, 1, 1, nan, 0.0f));
    auto start = pad.Handle(Action::Down, 1, 1, -0.1f, 1.1f);
    ASSERT_TRUE(start);
    EXPECT_FLOAT_EQ(start->x, 0.0f);
    EXPECT_FLOAT_EQ(start->y, 1.0f);
    auto end = pad.Handle(Action::Up, 1, 1, nan, nan);
    ASSERT_TRUE(end);
    EXPECT_FALSE(end->down);
    EXPECT_FLOAT_EQ(end->x, 0.0f);
    EXPECT_FLOAT_EQ(end->y, 1.0f);
}

TEST(TouchscreenTouchpad, FocusLossReleasesBothAndIgnoresStaleEvents) {
    Pad pad;
    pad.Handle(Action::Down, 1, 1, 0.2f, 0.3f);
    pad.Handle(Action::Down, 1, 2, 0.4f, 0.5f);
    const auto released = pad.ReleaseAll();
    EXPECT_FALSE(released[0].down);
    EXPECT_FALSE(released[1].down);
    EXPECT_FLOAT_EQ(released[1].x, 0.4f);
    EXPECT_FALSE(pad.Handle(Action::Motion, 1, 1, 0.8f, 0.8f));
    EXPECT_FALSE(pad.Handle(Action::Up, 1, 2, 0.8f, 0.8f));
    EXPECT_EQ(pad.Handle(Action::Down, 1, 3, 0.5f, 0.5f)->slot, 0);
}
