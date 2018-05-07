#include <precompiled.hh>

#include "menu.hh"

#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <sdk/convar.hh>
#include <sdk/draw.hh>
#include <sdk/log.hh>

namespace menu {

struct menu_element {
    bool                cat_init;
    bool                uncat_enabled; // dirty
    std::string         name;
    sdk::ConvarType     type;
    sdk::ConvarWrapper *cvar;
    menu_element(const char *cname, sdk::ConvarType ctype, bool uncat = false) {
        cat_init      = uncat;
        uncat_enabled = false;
        name          = cname;
        type          = ctype;
        //cat_name = ccat_name;
        if (!uncat) {
            sdk::ConvarWrapper tmp(cname);
            cvar = (sdk::ConvarWrapper *)malloc(sizeof(sdk::ConvarWrapper));
            memcpy(cvar, &tmp, sizeof(sdk::ConvarWrapper));
        } else {
            cvar = nullptr;
        }
    };
};

std::vector<std::deque<menu_element>> to_render;
//std::deque<menu_element>              uncat_cvars;

bool can_draw = false;

void init() {
    to_render.erase(to_render.cbegin(), to_render.cend());
    std::unordered_set<std::string> cat_names;
    //std::unordered_set<sdk::ConvarWrapper> cvars;

    std::unordered_map<std::string, std::vector<menu_element>> cat_cvars;

    for (auto c : sdk::ConvarBase::Convar_Range()) {
        //cvars.insert(sdk::ConvarWrapper(c->name));
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
                //cat_cvars[cat_name].push_back(std::make_pair(sdk::ConvarWrapper(c->name), c->name));
                cat_cvars[cat_name].push_back(menu_element(c->name(), c->type()));
            } else {
                //uncat_cvars.push_back(std::make_pair(sdk::ConvarWrapper(c->name), c->name));
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
        strcat_s(enabled, "_enabled");

        std::deque<menu_element> tmp;

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
            tmp.push_front(menu_element(cat_name.c_str(), sdk::ConvarType::Bool, true));

        to_render.push_back(tmp);
    }

    //uncat_cvars.push_front(menu_element("Misc", sdk::ConvarType::Bool, true));
    //to_render.push_back(uncat_cvars);

    can_draw = true;
}

void cursor_input(int &cur_pos) {
    static bool was_up   = false;
    static bool was_down = false;
    if (!was_up && IFace<sdk::InputSystem>()->is_button_down(sdk::ButtonCode::KEY_UP)) {
        was_up = true;
        cur_pos--;
    } else if (was_up && !IFace<sdk::InputSystem>()->is_button_down(sdk::ButtonCode::KEY_UP)) {
        was_up = false;
    }
    if (!was_down && IFace<sdk::InputSystem>()->is_button_down(sdk::ButtonCode::KEY_DOWN)) {
        was_down = true;
        cur_pos++;
    } else if (was_down && !IFace<sdk::InputSystem>()->is_button_down(sdk::ButtonCode::KEY_DOWN)) {
        was_down = false;
    }
}

void cvar_input(menu_element &elem) {
    static bool was_left  = false;
    static bool was_right = false;
    if (!was_left && IFace<sdk::InputSystem>()->is_button_down(sdk::ButtonCode::KEY_LEFT)) {
        was_left = true;
        switch (elem.type) {
        case sdk::ConvarType::Bool:
            if (elem.cvar)
                elem.cvar->set_value("0");
            else
                elem.uncat_enabled = false;
            break;
        case sdk::ConvarType::Int:
            elem.cvar->set_value(std::to_string(elem.cvar->get_int() - 1).c_str());
            break;
        case sdk::ConvarType::Float:
            elem.cvar->set_value(std::to_string(elem.cvar->get_float() - 1.0f).c_str());
            break;
        }
    } else if (was_left && !IFace<sdk::InputSystem>()->is_button_down(sdk::ButtonCode::KEY_LEFT)) {
        was_left = false;
    }
    if (!was_right && IFace<sdk::InputSystem>()->is_button_down(sdk::ButtonCode::KEY_RIGHT)) {
        was_right = true;
        switch (elem.type) {
        case sdk::ConvarType::Bool:
            if (elem.cvar)
                elem.cvar->set_value("1");
            else
                elem.uncat_enabled = true;
            break;
        case sdk::ConvarType::Int:
            elem.cvar->set_value(std::to_string(elem.cvar->get_int() + 1).c_str());
            break;
        case sdk::ConvarType::Float:
            elem.cvar->set_value(std::to_string(elem.cvar->get_float() + 1.0f).c_str());
            break;
        }
    } else if (was_right && !IFace<sdk::InputSystem>()->is_button_down(sdk::ButtonCode::KEY_RIGHT)) {
        was_right = false;
    }
}

bool is_visible() {
    static bool insert  = false;
    static bool visible = false;
    if (!insert && IFace<sdk::InputSystem>()->is_button_down(sdk::ButtonCode::KEY_INSERT)) {
        visible = !visible;
        insert  = true;
    } else if (insert && !IFace<sdk::InputSystem>()->is_button_down(sdk::ButtonCode::KEY_INSERT))
        insert = false;
    return visible;
}

void draw_elem(const sdk::draw::FontHandle &font, sdk::draw::Color &color, const math::Vector &render, const menu_element &elem) {
    switch (elem.type) {
    case sdk::ConvarType::Bool:
        sdk::draw::text(font, color, render, "    %s: %s", elem.name.c_str(), elem.cvar->get_bool() ? "Enabled" : "Disabled");
        break;
    case sdk::ConvarType::Int:
        sdk::draw::text(font, color, render, "    %s: %d", elem.name.c_str(), elem.cvar->get_int());
        break;
    case sdk::ConvarType::Float:
        sdk::draw::text(font, color, render, "    %s: %.4f", elem.name.c_str(), elem.cvar->get_float());
        break;
    case sdk::ConvarType::String:
        sdk::draw::text(font, color, render, "    %s: %s", elem.name.c_str(), elem.cvar->get_string());
        break;
    default:
        color.r = 255;
        sdk::draw::text(font, color, render, "    %s: Unsupported", elem.name.c_str());
        break;
    }
}

void paint() {
    if (!can_draw)
        return;

    if (!is_visible())
        return;

    static math::Vector pos{400, 5, 0};
    static auto         font    = sdk::draw::register_font("Arial", 16);
    static int          cur_pos = 0;
    static math::Vector end{400, 5, 0};
    //static bool inited = false;

    math::Vector render{pos.x, pos.y, 0};
    int          cursor = 0;

    cursor_input(cur_pos);

    float max_len = 0;

    if (cur_pos < 0)
        cur_pos = 0;

	sdk::draw::filled_rect({ 0, 0, 0, 255 - 8 }, pos, end);

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
                sdk::draw::text(font, cat_enabled ? sdk::draw::Color(128, 0, selected ? 255 : 0, 255) : sdk::draw::Color(0, 128, selected ? 255 : 0, 255), render, ">%s", elem.name.c_str());
                /*char tmp[256];
				sprintf(tmp, ">%s", elem.name.c_str());
				int wide, tall;
				IFace<sdk::draw::Surface>()->text_size(font, tmp, wide, tall);
				max_len = std::max(max_len, (float)wide);*/
                cursor++;
                render.y += 16;
            } else if (cat_enabled) {
                if (selected)
                    cvar_input(elem);
                sdk::draw::Color color(128, 0, selected ? 255 : 128, 255);
                draw_elem(font, color, render, elem);
                cursor++;
                render.y += 16;
            }
        }
    }
#undef selected

    if (cur_pos >= cursor)
        cur_pos = cursor-1;

	end = render;
	end.x += 200.0f;
}
} // namespace menu
