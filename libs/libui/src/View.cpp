/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <libfoundation/EventLoop.h>
#include <libg/Color.h>
#include <libui/Context.h>
#include <libui/View.h>

namespace UI {

View::View(View* superview, const LG::Rect& frame)
    : m_frame(frame)
    , m_superview(superview)
    , m_window(superview ? superview->window() : nullptr)
    , m_bounds(0, 0, frame.width(), frame.height())
    , m_layer()
{
}

View::View(View* superview, Window* window, const LG::Rect& frame)
    : m_frame(frame)
    , m_superview(superview)
    , m_window(window)
    , m_bounds(0, 0, frame.width(), frame.height())
    , m_layer()
{
}

View::~View()
{
    for (auto* v : subviews()) {
        delete v;
    }
}

void View::remove_view(View* child)
{
    if (!child) {
        return;
    }

    auto it = std::find(m_subviews.begin(), m_subviews.end(), child);
    if (it == m_subviews.end()) {
        return;
    }

    m_subviews.erase(it);
}

void View::remove_from_superview()
{
    if (!superview()) {
        return;
    }
    superview()->remove_view(this);
}

std::optional<View*> View::subview_at(const LG::Point<int>& point) const
{
    for (auto it = subviews().rbegin(); it != subviews().rend(); it++) {
        View* view = *it;
        if (view->frame().contains(point)) {
            return view;
        }
    }
    return {};
}

View& View::hit_test(const LG::Point<int>& point)
{
    auto subview = subview_at(point);
    if (subview.has_value()) {
        return subview.value()->hit_test(point - subview_location(*subview.value()));
    }
    return *this;
}

LG::Rect View::frame_in_window()
{
    View* view = this;
    auto rect = frame();
    while (view->has_superview()) {
        view = view->superview();
        rect.offset_by(view->frame().origin());
    }
    return rect;
}

void View::layout_subviews()
{
    // TODO: Apply topsort to find the right order.
    for (const auto& constraint : m_constrints) {
        constraint_interpreter(constraint);
    }
}

LG::Point<int> View::subview_location(const View& subview) const
{
    return subview.frame().origin();
}

void View::set_needs_display(const LG::Rect& rect)
{
    auto display_rect = rect;
    display_rect.intersect(bounds());
    if (has_superview()) {
        auto location = superview()->subview_location(*this);
        display_rect.offset_by(location);
        superview()->set_needs_display(display_rect);
    } else {
        send_display_message_to_self(*window(), display_rect);
    }
}

void View::display(const LG::Rect& rect)
{
    LG::Context ctx = graphics_current_context();
    ctx.add_clip(rect);
    ctx.set_fill_color(background_color());
    ctx.fill_rounded(bounds(), layer().corner_mask());
}

void View::did_display(const LG::Rect& rect)
{
}

void View::mouse_moved(const LG::Point<int>& location)
{
    m_gesture_manager.mouse_moved(location);
}

void View::mouse_entered(const LG::Point<int>& location)
{
    set_hovered(true);
}

void View::mouse_exited()
{
    set_hovered(false);
    set_active(false);
    m_gesture_manager.mouse_up();
}

void View::mouse_down(const LG::Point<int>& location)
{
    set_active(true);
    m_gesture_manager.mouse_down(location);
}

void View::mouse_up()
{
    set_active(false);
    m_gesture_manager.mouse_up();
}

View::WheelEventResponse View::mouse_wheel_event(int wheel_data)
{
    return View::WheelEventResponse::Skipped;
}

void View::receive_mouse_move_event(MouseEvent& event)
{
    auto location = LG::Point<int>(event.x(), event.y());
    if (!is_hovered()) {
        mouse_entered(location);
    }

    foreach_subview([&](View& subview) -> bool {
        bool event_hits_subview = subview.frame().contains(event.x(), event.y());
        if (subview.is_hovered() && !event_hits_subview) {
            LG::Point<int> point(event.x(), event.y());
            point.offset_by(-subview.frame().origin());
            MouseLeaveEvent mle(point.x(), point.y());
            subview.receive_mouse_leave_event(mle);
        } else if (event_hits_subview) {
            LG::Point<int> point(event.x(), event.y());
            point.offset_by(-subview.frame().origin());
            MouseEvent me(point.x(), point.y());
            subview.receive_mouse_move_event(me);
        }
        return true;
    });

    mouse_moved(location);
    Responder::receive_mouse_move_event(event);
}

void View::receive_mouse_action_event(MouseActionEvent& event)
{
    if (event.type() == MouseActionType::LeftMouseButtonPressed) {
        mouse_down(LG::Point<int>(event.x(), event.y()));
    } else if (event.type() == MouseActionType::LeftMouseButtonReleased) {
        mouse_up();
    }

    Responder::receive_mouse_action_event(event);
}

void View::receive_mouse_leave_event(MouseLeaveEvent& event)
{
    if (!is_hovered()) {
        return;
    }

    foreach_subview([&](View& subview) -> bool {
        if (subview.is_hovered()) {
            LG::Point<int> point(event.x(), event.y());
            point.offset_by(-subview.frame().origin());
            MouseLeaveEvent mle(point.x(), point.y());
            subview.receive_mouse_leave_event(mle);
        }
        return true;
    });

    mouse_exited();
    Responder::receive_mouse_leave_event(event);
}

bool View::receive_mouse_wheel_event(MouseWheelEvent& event)
{
    bool found_target = false;
    foreach_subview([&](View& subview) -> bool {
        if (subview.is_hovered()) {
            LG::Point<int> point(event.x(), event.y());
            point.offset_by(-subview.frame().origin());
            MouseWheelEvent mwe(point.x(), point.y(), event.wheel_data());
            found_target |= subview.receive_mouse_wheel_event(mwe);
        }
        return true;
    });

    // If not child can process wheel/scroll event, we try to do this.
    if (!found_target) {
        WheelEventResponse resp = mouse_wheel_event(event.wheel_data());
        if (resp == WheelEventResponse::Handled) {
            found_target = true;
        }
    }
    return found_target;
}

void View::receive_keyup_event(KeyUpEvent&)
{
}

void View::receive_keydown_event(KeyDownEvent&)
{
}

void View::receive_display_event(DisplayEvent& event)
{
    event.bounds().intersect(bounds());
    display(event.bounds());
    foreach_subview([&](View& subview) -> bool {
        auto bounds = event.bounds();
        if (bounds.intersects(subview.frame())) {
            subview.layer().display(bounds, subview.frame());
            graphics_push_context(Context(subview, Context::RelativeToCurrentContext::Yes));
            bounds.offset_by(-subview.frame().origin());
            DisplayEvent own_event(bounds);
            subview.receive_display_event(own_event);
            graphics_pop_context();
        }
        return true;
    });
    did_display(event.bounds());

    if (!has_superview()) {
        // Only superview sends invalidate_message to server.
        bool success = send_invalidate_message_to_server(event.bounds());
    }

    Responder::receive_display_event(event);
}

bool View::receive_layout_event(const LayoutEvent& event, bool force_layout_if_not_target)
{
    bool need_to_layout = (this == event.target()) | force_layout_if_not_target;
    if (need_to_layout) {
        layout_subviews();
    }

    foreach_subview([&](View& subview) -> bool {
        bool found_target = subview.receive_layout_event(event, need_to_layout);
        return need_to_layout || !found_target;
    });

    return need_to_layout;
}

} // namespace UI