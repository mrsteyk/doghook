#include <precompiled.hh>

#include "menu.hh"

#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <sdk/convar.hh>
#include <sdk/draw.hh>
#include <sdk/log.hh>

using namespace sdk;

namespace menu {
struct MenuElement {
    bool           cat_init;
    bool           uncat_enabled; // dirty
    std::string    name;
    ConvarType     type;
    ConvarWrapper *cvar;
    MenuElement(const char *cname, ConvarType ctype, bool uncat = false) {
        cat_init      = uncat;
        uncat_enabled = false;
        name          = cname;
        type          = ctype;
        //cat_name = ccat_name;
        if (!uncat) {
            name = cname + 8;
            ConvarWrapper tmp(cname);
            cvar = (ConvarWrapper *)malloc(sizeof(ConvarWrapper));
            memcpy(cvar, &tmp, sizeof(ConvarWrapper));
        } else {
            cvar = nullptr;
        }
    };
};

std::vector<std::deque<MenuElement>> to_render;
//std::deque<menu_element>              uncat_cvars;

bool can_draw = false;

void init() {
    to_render.erase(to_render.cbegin(), to_render.cend());
    std::unordered_set<std::string> cat_names;
    //std::unordered_set<ConvarWrapper> cvars;

    std::unordered_map<std::string, std::vector<MenuElement>> cat_cvars;

    for (auto c : ConvarBase::Convar_Range()) {
        //cvars.insert(ConvarWrapper(c->name));
        //char *cat_name_start = strstr(c->name, "_");
        std::string name(c->name());
        size_t      cat_name_start = name.find("_");
        if (cat_name_start != std::string::npos) {
            //const size_t cat_name_end = strstr(c->name + cat_name_start, "_") - cat_name_start;
            size_t cat_name_end = name.find("_", cat_name_start + 1);
            if (cat_name_end != std::string::npos) {
                std::string cat_name(name.c_str() + cat_name_start + 1, cat_name_end - cat_name_start - 1);
                cat_names.insert(cat_name);
                logging::msg("cat_name %s cvar_name %s", cat_name.c_str(), c->name());
                //cat_cvars[cat_name].push_back(std::make_pair(ConvarWrapper(c->name), c->name));
                cat_cvars[cat_name].push_back(MenuElement(c->name(), c->type()));
            } else {
                //uncat_cvars.push_back(std::make_pair(ConvarWrapper(c->name), c->name));
                //uncat_cvars.push_back(menu_element(c->name(), c->type()));
            }
        } else {
            logging::msg("TF!? %s", c->name());
        }
    }

    for (auto cat_name : cat_names) {
        char enabled[256];
        memset(enabled, 0, 256);
        strcpy(enabled, cat_name.c_str());
        strcat(enabled, "_enabled");

        std::deque<MenuElement> tmp;

        bool search = true;

        for (auto &menu_elem : cat_cvars[cat_name]) {
            if (search && menu_elem.name.find(enabled) != std::string::npos) {
                menu_elem.cat_init = true;
                menu_elem.name     = cat_name;
                tmp.push_front(menu_elem);
                logging::msg("%s", menu_elem.name.c_str());
                search = false;
            } else {
                tmp.push_back(menu_elem);
            }
        }

        if (search)
            tmp.push_front(MenuElement(cat_name.c_str(), ConvarType::Bool, true));

        to_render.push_back(tmp);
    }

    std::reverse(to_render.begin(), to_render.end());

    //uncat_cvars.push_front(menu_element("Misc", ConvarType::Bool, true));
    //to_render.push_back(uncat_cvars);

    can_draw = true;
}

void cursor_input(int &cur_pos) {
    static bool was_up   = false;
    static bool was_down = false;
    if (!was_up && iface::input_system->is_button_down(ButtonCode::KEY_UP)) {
        was_up = true;
        cur_pos--;
    } else if (was_up && !iface::input_system->is_button_down(ButtonCode::KEY_UP)) {
        was_up = false;
    }
    if (!was_down && iface::input_system->is_button_down(ButtonCode::KEY_DOWN)) {
        was_down = true;
        cur_pos++;
    } else if (was_down && !iface::input_system->is_button_down(ButtonCode::KEY_DOWN)) {
        was_down = false;
    }
}

void cvar_input(MenuElement &elem) {
    static bool was_left  = false;
    static bool was_right = false;
    if (!was_left && iface::input_system->is_button_down(ButtonCode::KEY_LEFT)) {
        was_left = true;
        switch (elem.type) {
        case ConvarType::Bool:
            if (elem.cvar)
                elem.cvar->set_value("0");
            else
                elem.uncat_enabled = false;
            break;
        case ConvarType::Int:
            elem.cvar->set_value(std::to_string(elem.cvar->get_int() - 1).c_str());
            break;
        case ConvarType::Float:
            elem.cvar->set_value(std::to_string(elem.cvar->get_float() - 1.0f).c_str());
            break;
        }
    } else if (was_left && !iface::input_system->is_button_down(ButtonCode::KEY_LEFT)) {
        was_left = false;
    }
    if (!was_right && iface::input_system->is_button_down(ButtonCode::KEY_RIGHT)) {
        was_right = true;
        switch (elem.type) {
        case ConvarType::Bool:
            if (elem.cvar)
                elem.cvar->set_value("1");
            else
                elem.uncat_enabled = true;
            break;
        case ConvarType::Int:
            elem.cvar->set_value(std::to_string(elem.cvar->get_int() + 1).c_str());
            break;
        case ConvarType::Float:
            elem.cvar->set_value(std::to_string(elem.cvar->get_float() + 1.0f).c_str());
            break;
        }
    } else if (was_right && !iface::input_system->is_button_down(ButtonCode::KEY_RIGHT)) {
        was_right = false;
    }
}

bool is_visible() {
    static bool insert  = false;
    static bool visible = false;
    if (!insert && iface::input_system->is_button_down(ButtonCode::KEY_INSERT)) {
        visible = !visible;
        insert  = true;
    } else if (insert && !iface::input_system->is_button_down(ButtonCode::KEY_INSERT))
        insert = false;
    return visible;
}

void draw_elem(const draw::FontHandle &font, draw::Color &color, const math::Vector &render, const MenuElement &elem) {
    switch (elem.type) {
    case ConvarType::Bool:
        draw::text(font, 15, color, render, "    %s: %s", elem.name.c_str(), elem.cvar->get_bool() ? "Enabled" : "Disabled");
        break;
    case ConvarType::Int:
        draw::text(font, 15, color, render, "    %s: %d", elem.name.c_str(), elem.cvar->get_int());
        break;
    case ConvarType::Float:
        draw::text(font, 15, color, render, "    %s: %.4f", elem.name.c_str(), elem.cvar->get_float());
        break;
    case ConvarType::String:
        draw::text(font, 15, color, render, "    %s: %s", elem.name.c_str(), elem.cvar->get_string());
        break;
    default:
        color.r = 255;
        draw::text(font, 15, color, render, "    %s: Unsupported", elem.name.c_str());
        break;
    }
}

void paint() {
    if (!can_draw)
        return;

    if (!is_visible())
        return;

    static math::Vector pos{400, 5, 0};
    static auto         font = []() {
        if constexpr (doghook_platform::windows())
            return draw::register_font("C:\\Windows\\Fonts\\Tahoma.ttf", "Arial", 16);
        else if (doghook_platform::linux()) {
            return draw::register_font("/usr/share/fonts/TTF/DejaVuSans.ttf", "DejaVuSans", 16);
        }
    }();
    static int          cur_pos = 0;
    static math::Vector end{400, 5, 0};
    //static bool inited = false;

    math::Vector render{pos.x, pos.y, 0};
    int          cursor = 0;

    cursor_input(cur_pos);

    float max_len = 0;

    if (cur_pos < 0)
        cur_pos = 0;

    draw::filled_rect({0, 0, 0, 255 - 8}, pos, end);

    for (auto &cat : to_render) {
        bool cat_enabled = false;
        for (auto &elem : cat) {
#define selected cursor == cur_pos
            if (elem.cat_init) {
                if (selected)
                    cvar_input(elem);
                if (elem.cvar) {
                    cat_enabled = elem.cvar->get_bool();
                } else {
                    cat_enabled = elem.uncat_enabled;
                }
                draw::text(font, 15, cat_enabled ? draw::Color(128, 0, selected ? 255 : 0, 255) : draw::Color(0, 128, selected ? 255 : 0, 255), render, ">%s", elem.name.c_str());
#if 0
                char tmp[256];
				sprintf(tmp, ">%s", elem.name.c_str());
				int wide, tall;
				IFace<draw::Surface>()->text_size(font, tmp, wide, tall);
				max_len = std::max(max_len, (float)wide);
#endif
                cursor++;
                render.y += 16;
            } else if (cat_enabled) {
                if (selected)
                    cvar_input(elem);
                draw::Color color(128, 0, selected ? 255 : 128, 255);
                draw_elem(font, color, render, elem);
                cursor++;
                render.y += 16;
            }
        }
    }
#undef selected

    if (cur_pos >= cursor)
        cur_pos = cursor - 1;

    end = render;
    end.x += 200.0f;
}
} // namespace menu
