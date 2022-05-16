/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once
#include <functional>
#include <libg/Size.h>
#include <libui/Constants/Layout.h>
#include <libui/EdgeInsets.h>
#include <libui/ScrollView.h>
#include <list>
#include <string>
#include <utility>

namespace UI {

// The CollectionView in UIKit is devided into sections where each section contains a number
// of elements. This behavior is not implemented, instead of CollectionViewDataSource protocol
// just a callback CollectionViewRowStreamer is used. It returns the whole line to be rendered.
typedef std::function<View*(int)> CollectionViewRowStreamer;

class CollectionView : public ScrollView {
    UI_OBJECT();

public:
    ~CollectionView() = default;

    template <class T>
    void set_data_source(T data_source) { m_data_source = data_source, prefetch_forward(); };

    void reload_data() { prefetch_forward(); }
    void invalidate_row(int id);

    virtual void display(const LG::Rect& rect) override;
    virtual void mouse_entered(const LG::Point<int>& location) override;
    virtual void mouse_exited() override;
    virtual WheelEventResponse mouse_wheel_event(int wheel_data) override
    {
        ScrollView::mouse_wheel_event(wheel_data);
        if (wheel_data > 0) {
            after_scroll_forward();
            prefetch_forward();
        } else {
            after_scroll_backward();
        }

        return WheelEventResponse::Handled;
    }

protected:
    CollectionView(View* superview, const LG::Rect&);
    CollectionView(View* superview, Window* window, const LG::Rect& frame);

private:
    enum PrefetchStatus {
        Success,
        EndOfStream,
    };
    PrefetchStatus prefetch_row_forward(int id);
    void prefetch_forward();
    void after_scroll_forward();
    void after_scroll_backward();

    // Cache of views which precede the first on-screen view.
    std::list<View*> m_preceding_views {};

    // Cache of views which follow the last on-screen view.
    std::list<View*> m_following_views {};
    std::list<View*> m_views_on_screen {};

    size_t m_first_onscreen_row_index { 0 };
    size_t m_first_offscreen_row_index { 0 };
    LG::Point<int> m_next_frame_origin { 0, 16 };

    CollectionViewRowStreamer m_data_source;
};

} // namespace UI