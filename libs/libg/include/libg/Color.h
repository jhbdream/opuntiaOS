/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <cstdint>
#include <libfoundation/Logger.h>

namespace LG {

// We keep opacity as an opposite to alpha.
// This is used for capability with BGA driver
// and probably should be fixed.
class Color final {
public:
    enum Colors {
        Red,
        Blue,
        Green,
        White,
        Black,
        LightSystemText,
        LightSystemAccentText,
        LightSystemBackground,
        LightSystemButton,
        LightSystemAccentButton,
        LightSystemBlue,
        LightSystemOpaque,
        LightSystemOpaque128,
        DarkSystemBackground,
        DarkSystemText,
        Opaque,
    };

    Color() = default;
    Color(Colors);
    Color(const Color& c) = default;

    Color(uint32_t color)
        : m_opacity((color & 0xFF000000) >> 24)
        , m_r((color & 0x00FF0000) >> 16)
        , m_g((color & 0x0000FF00) >> 8)
        , m_b((color & 0x000000FF) >> 0)
    {
    }

    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t alpha = 255)
        : m_opacity(255 - alpha)
        , m_b(b)
        , m_g(g)
        , m_r(r)
    {
    }

    ~Color() = default;

    Color& operator=(const Color& c) = default;

    Color& operator=(Color&& c)
    {
        m_opacity = c.m_opacity;
        m_r = c.m_r;
        m_g = c.m_g;
        m_b = c.m_b;
        return *this;
    }

    inline uint8_t alpha() const { return 255 - m_opacity; }
    inline bool is_opaque() const { return m_opacity == 255; }

    inline uint8_t red() const { return m_r; }
    inline uint8_t green() const { return m_g; }
    inline uint8_t blue() const { return m_b; }
    inline void set_alpha(uint8_t alpha) { m_opacity = 255 - alpha; }

    inline uint32_t u32() const
    {
        uint32_t clr = (m_opacity << 24)
            | (m_b << 0)
            | (m_g << 8)
            | (m_r << 16);
        return clr;
    }

    [[gnu::always_inline]] inline void mix_with(const Color& clr)
    {
        if (clr.is_opaque()) {
            return;
        }

        if (clr.alpha() == 255 || is_opaque()) {
            *this = clr;
            return;
        }

        size_t alpha_c = 255 * (alpha() + clr.alpha()) - alpha() * clr.alpha();
        size_t alpha_of_me = alpha() * (255 - clr.alpha());
        size_t alpha_of_it = 255 * clr.alpha();

        m_r = (red() * alpha_of_me + clr.red() * alpha_of_it) / alpha_c;
        m_g = (green() * alpha_of_me + clr.green() * alpha_of_it) / alpha_c;
        m_b = (blue() * alpha_of_me + clr.blue() * alpha_of_it) / alpha_c;
        m_opacity = 255 - (alpha_c / 255);
    }

    inline LG::Color darken(int percents) const
    {
        double multiplier = 1.0 - (double(percents) / 100.0);
        int r = int(red() * multiplier);
        int g = int(green() * multiplier);
        int b = int(blue() * multiplier);
        return LG::Color(r, g, b);
    }

private:
    uint8_t m_b { 0 };
    uint8_t m_g { 0 };
    uint8_t m_r { 0 };
    uint8_t m_opacity { 0 };
};

// Should that color size is 4 bytes, as window_server requires this.
static_assert(sizeof(Color) == 4);

} // namespace LG