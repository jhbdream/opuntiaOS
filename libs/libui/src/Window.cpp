/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <libfoundation/Memory.h>
#include <libui/App.h>
#include <libui/Connection.h>
#include <libui/Context.h>
#include <libui/Screen.h>
#include <libui/Window.h>

namespace UI {

Window::Window(const std::string& title, const LG::Size& size, WindowType type)
{
    init_window(title, size, "", StatusBarStyle(), type);
}

Window::Window(const std::string& title, const LG::Size& size, const std::string& icon_path)
{
    init_window(title, size, icon_path, StatusBarStyle(), WindowType::Standard);
}

Window::Window(const std::string& title, const LG::Size& size, const std::string& icon_path, const StatusBarStyle& style)
{
    init_window(title, size, icon_path, style, WindowType::Standard);
}

void Window::init_window(const std::string& title, const LG::Size& size, const std::string& icon_path, const StatusBarStyle& style, WindowType type)
{
    m_scale = UI::Screen::main().scale();
    m_bounds = LG::Rect(0, 0, size.width(), size.height());
    m_native_bounds = LG::Rect(0, 0, size.width() * m_scale, size.height() * m_scale);

    m_buffer = LFoundation::SharedBuffer<LG::Color>(size_t(native_bounds().width() * native_bounds().height()));
    m_title = title;
    m_icon_path = icon_path;
    m_status_bar_style = style;
    m_type = type;

    m_id = Connection::the().new_window(*this);
    m_menubar.set_host_window_id(m_id);
    m_popup.set_host_window_id(m_id);
    m_bitmap = LG::PixelBitmap(m_buffer.data(), native_bounds().width(), native_bounds().height());
    App::the().set_window(this);
}

bool Window::set_title(const std::string& title)
{
    m_title = title;
    SetTitleMessage msg(Connection::the().key(), id(), title);
    return App::the().connection().send_async_message(msg);
}

bool Window::set_status_bar_style(StatusBarStyle style)
{
    m_status_bar_style = style;
    SetBarStyleMessage msg(Connection::the().key(), id(), style.color().u32(), style.flags());
    return App::the().connection().send_async_message(msg);
}

bool Window::did_buffer_change()
{
    m_bitmap.set_data(buffer().data());
    m_bitmap.set_size({ bounds().width(), bounds().height() });

    // If we have a superview, we also have a context for the superview.
    // This context should be updated, since the superview is resized.
    if (m_superview) {
        graphics_pop_context();
        graphics_push_context(Context(*m_superview));
    }

    SetBufferMessage msg(Connection::the().key(), id(), buffer().id(), bitmap().format(), bounds());
    return App::the().connection().send_async_message(msg);
}

bool Window::did_format_change()
{
    if (bitmap().format() == LG::PixelBitmapFormat::RGBA) {
        // Set full bitmap as opaque, to mix colors correctly.
        fill_with_opaque(bounds());
    }

    return did_buffer_change();
}

void Window::receive_event(std::unique_ptr<LFoundation::Event> event)
{
    switch (event->type()) {
    case Event::Type::MouseEvent:
        if (m_superview) {
            MouseEvent& own_event = *(MouseEvent*)event.get();
            m_superview->receive_mouse_move_event(own_event);
        }
        break;

    case Event::Type::MouseActionEvent:
        if (m_superview) {
            MouseActionEvent& own_event = *(MouseActionEvent*)event.get();
            auto& view = m_superview->hit_test({ (int)own_event.x(), (int)own_event.y() });
            view.receive_mouse_action_event(own_event);
        }
        break;

    case Event::Type::MouseLeaveEvent:
        if (m_superview) {
            MouseLeaveEvent& own_event = *(MouseLeaveEvent*)event.get();
            m_superview->receive_mouse_leave_event(own_event);
        }
        break;

    case Event::Type::MouseWheelEvent:
        if (m_superview) {
            MouseWheelEvent& own_event = *(MouseWheelEvent*)event.get();
            m_superview->receive_mouse_wheel_event(own_event);
        }
        break;

    case Event::Type::KeyUpEvent:
        if (m_focused_view) {
            KeyUpEvent& own_event = *(KeyUpEvent*)event.get();
            m_focused_view->receive_keyup_event(own_event);
        }
        break;

    case Event::Type::KeyDownEvent:
        if (m_focused_view) {
            KeyDownEvent& own_event = *(KeyDownEvent*)event.get();
            m_focused_view->receive_keydown_event(own_event);
        }
        break;

    case Event::Type::DisplayEvent:
        if (m_superview) {
            DisplayEvent& own_event = *(DisplayEvent*)event.get();

            // If the window is in RGBA mode, we have to fill this rect
            // with opaque color before superview will mix it's color on
            // top of bitmap.
            if (bitmap().format() == LG::PixelBitmapFormat::RGBA) {
                fill_with_opaque(own_event.bounds());
            }

            m_superview->receive_display_event(own_event);
        }
        break;

    case Event::Type::LayoutEvent:
        if (m_superview) {
            LayoutEvent& own_event = *(LayoutEvent*)event.get();
            m_superview->receive_layout_event(own_event);
        }
        break;

    case Event::Type::MenuBarActionEvent: {
        MenuBarActionEvent& own_event = *(MenuBarActionEvent*)event.get();
        for (Menu& menu : menubar().menus()) {
            if (menu.menu_id() == own_event.menu_id()) {
                if (own_event.item_id() < menu.items().size()) [[likely]] {
                    menu.items()[own_event.item_id()].invoke();
                }
            }
        }
        break;
    }

    case Event::Type::PopupActionEvent: {
        PopupActionEvent& own_event = *(PopupActionEvent*)event.get();
        auto& menu = popup_manager().menu();
        if (own_event.item_id() < menu.items().size()) [[likely]] {
            menu.items()[own_event.item_id()].invoke();
        }
        break;
    }

    case Event::Type::ResizeEvent: {
        ResizeEvent& own_event = *(ResizeEvent*)event.get();
        resize(own_event);
        break;
    }
    }
}

void Window::resize(ResizeEvent& resize_event)
{
    m_bounds = resize_event.bounds();

    if (m_superview) [[likely]] {
        m_superview->frame() = resize_event.bounds();
        m_superview->bounds() = resize_event.bounds();
        m_superview->set_needs_layout();
    }

    size_t new_size = resize_event.bounds().width() * resize_event.bounds().height();
    if (m_buffer.size() != new_size) [[likely]] {
        m_buffer.resize(resize_event.bounds().width() * resize_event.bounds().height());
        did_buffer_change();
    }
}

void Window::setup_superview()
{
    graphics_push_context(Context(*m_superview));
}

void Window::fill_with_opaque(const LG::Rect& rect)
{
    const auto color = LG::Color(0, 0, 0, 0).u32();
    auto draw_bounds = rect;
    draw_bounds.intersect(bounds());
    if (draw_bounds.empty()) {
        return;
    }

    int min_x = draw_bounds.min_x();
    int min_y = draw_bounds.min_y();
    int max_x = draw_bounds.max_x();
    int max_y = draw_bounds.max_y();
    int len_x = max_x - min_x + 1;
    for (int y = min_y; y <= max_y; y++) {
        LFoundation::fast_set((uint32_t*)&m_bitmap[y][min_x], color, len_x);
    }
}

} // namespace UI