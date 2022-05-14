#pragma once
#include "FastLaunchEntity.h"
#include "HomeScreenEntity.h"
#include <libg/Font.h>
#include <libui/Label.h>
#include <libui/View.h>
#include <list>
#include <string>
#include <unistd.h>

class IconView : public UI::View {
    UI_OBJECT();

public:
    IconView(View* superview, const LG::Rect& frame);

    void display(const LG::Rect& rect) override;
    void mouse_up() override { launch(m_launch_entity.path_to_exec()); }

    void set_title(const std::string& title)
    {
        if (!m_label) {
            return;
        }
        m_label->set_text(title);
    }

    FastLaunchEntity& entity() { return m_launch_entity; }
    const FastLaunchEntity& entity() const { return m_launch_entity; }

private:
    void launch(const std::string& path_to_exec)
    {
        if (fork() == 0) {
            for (int i = 3; i < 32; i++) {
                close(i);
            }
            execlp(path_to_exec.c_str(), path_to_exec.c_str(), NULL);
            std::abort();
        }
    }

    UI::Label* m_label;
    FastLaunchEntity m_launch_entity;
};