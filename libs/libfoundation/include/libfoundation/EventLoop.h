/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once
#include <functional>
#include <libfoundation/Event.h>
#include <libfoundation/EventReceiver.h>
#include <libfoundation/Receivers.h>
#include <list>
#include <memory>
#include <vector>

namespace LFoundation {

class QueuedEvent {
public:
    friend class EventLoop;
    QueuedEvent(EventReceiver& rec, Event* ptr)
        : event(ptr)
        , receiver(rec)
    {
    }

    QueuedEvent(QueuedEvent&& qe)
        : event(std::move(qe.event))
        , receiver(qe.receiver)
    {
    }

    QueuedEvent& operator=(QueuedEvent&& qe)
    {
        event = std::move(qe.event);
        receiver = qe.receiver;
        return *this;
    }

    ~QueuedEvent() = default;

    EventReceiver& receiver;
    std::unique_ptr<Event> event { nullptr };
};

class EventLoop {
public:
    inline static EventLoop& the()
    {
        extern EventLoop* s_LFoundation_EventLoop_the;
        return *s_LFoundation_EventLoop_the;
    }

    EventLoop();

    inline void add(int fd, std::function<void(void)> on_read, std::function<void(void)> on_write)
    {
        m_waiting_fds.push_back(FDWaiter(fd, on_read, on_write));
    }

    inline void add(const Timer& timer)
    {
        m_timers.push_back(timer);
    }

    inline void add(Timer&& timer)
    {
        m_timers.push_back(std::move(timer));
    }

    inline void add(EventReceiver& rec, Event* ptr)
    {
        m_event_queue.push_back(QueuedEvent(rec, ptr));
    }

    inline void stop(int exit_code) { m_exit_code = exit_code, m_stop_flag = true; }
    int run();

private:
    void pump();
    void cleanup_timers();
    void check_fds();
    void check_timers();

    bool m_stop_flag { false };
    int m_exit_code { 0 };
    std::vector<FDWaiter> m_waiting_fds;
    std::list<Timer> m_timers;
    std::vector<QueuedEvent> m_event_queue;
};
} // namespace LFoundation