/****************************************************************************

    File: TextArea.cpp
    Author: Aria Janke
    License: GPLv3

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*****************************************************************************/

#include <ksg/TextArea.hpp>
#include <ksg/TextButton.hpp>

#include <SFML/Graphics/RenderTarget.hpp>

#include <array>

#include <cmath>
#include <cassert>

using VectorF = ksg::Widget::VectorF;

namespace {

bool is_unassigned(float x)
    { return std::equal_to<float>()(x, ksg::TextArea::k_unassigned_size); }

float verify_valid_size(float, const char * caller, const char * name);

} // end of <anonymous> namespace

namespace ksg {

/* free fn */ void set_if_present
    (Text & text, const StyleMap & smap, const char * font_field,
     const char * char_size_field, const char * text_color)
{
    using namespace styles;
    text.assign_font(smap, font_field);

    if (auto * color = find<sf::Color>(smap, text_color))
        text.set_color(*color);
    else
        text.set_color(sf::Color::White);

    if (auto * char_size = find<float>(smap, char_size_field)) {
        if (get_unset_value<int>() == text.character_size())
            text.set_character_size(int(std::round(*char_size)));
    }
}

// ----------------------------------------------------------------------------

/* static */ constexpr const char * const TextArea::k_text_color;
/* static */ constexpr const char * const TextArea::k_text_size ;
/* static */ constexpr const float TextArea::k_unassigned_size;

TextArea::TextArea() {}

void TextArea::process_event(const sf::Event &) {}

void TextArea::set_location(float x, float y) {
    m_bounds.left = x;
    m_bounds.top  = y;
    recompute_geometry();
}

VectorF TextArea::location() const
    { return m_draw_text.location(); }

float TextArea::width() const {
    if (is_unassigned(m_bounds.width))
        return m_draw_text.width();
    return m_bounds.width;
}

float TextArea::height() const {
    if (is_unassigned(m_bounds.height))
        return m_draw_text.height();
    return m_bounds.height;
}

void TextArea::set_style(const StyleMap & smap) {
    using namespace styles;
    set_if_present(m_draw_text, smap, k_global_font, k_text_size, k_text_color);
    recompute_geometry();
}

void TextArea::issue_auto_resize() {
    recompute_geometry();
}

void TextArea::set_text(const UString & str) {
    m_draw_text.set_string(str);
    recompute_geometry();
}

void TextArea::set_string(const UString & str) {
    m_draw_text.set_string(str);
    recompute_geometry();
}

void TextArea::set_character_size(int size_) {
    m_draw_text.set_character_size(size_);
    recompute_geometry();
}

void TextArea::set_width(float w) {
    set_size(w, m_bounds.height);
}

void TextArea::set_height(float h) {
    set_size(m_bounds.width, h);
}

void TextArea::set_max_width(float w) {
    set_max_width_no_update(w);
    recompute_geometry();
}

void TextArea::set_max_height(float h) {
    set_max_height_no_update(h);
    recompute_geometry();
}

void TextArea::set_size(float w, float h) {
    m_bounds.width = verify_valid_size(w, "TextArea::set_size", "width");
    set_max_width_no_update(w);

    m_bounds.height = verify_valid_size(h, "TextArea::set_size", "height");
    set_max_height_no_update(h);

    recompute_geometry();
}

void TextArea::assign_font(const sf::Font & font) {
    m_draw_text.assign_font(&font);
    recompute_geometry();
}

/* protected */ void TextArea::draw
    (sf::RenderTarget & target, sf::RenderStates) const
{
    target.draw(m_draw_text);
}

/* private */ void TextArea::recompute_geometry() {
    VectorF text_loc;
    if (is_unassigned(m_bounds.width)) {
        text_loc.x = m_bounds.left;
    } else {
        text_loc.x = m_bounds.left + (m_bounds.width - m_draw_text.width()) / 2;
    }
    if (is_unassigned(m_bounds.height)) {
        text_loc.y = m_bounds.top;
    } else {
        text_loc.y = m_bounds.top + (m_bounds.height - m_draw_text.height()) / 2;
    }
    m_draw_text.set_location(text_loc);
}

/* private */ void TextArea::set_max_width_no_update(float w) {
    verify_valid_size(w, "TextArea::set_max_width_no_update", "width");
    if (is_unassigned(w)) {
        m_draw_text.relieve_width_limit();
    } else {
        m_draw_text.set_limiting_width(w);
    }
}

/* private */ void TextArea::set_max_height_no_update(float h) {
    verify_valid_size(h, "TextArea::set_max_height_no_update", "height");
    if (is_unassigned(h)) {
        m_draw_text.relieve_height_limit();
    } else {
        m_draw_text.set_limiting_height(h);
    }
}

} // end of ksg namespace

namespace {

float verify_valid_size(float x, const char * caller, const char * name) {
    if (is_unassigned(x)) return x;
    if (x < 0.f) {
        throw std::invalid_argument(
            std::string(caller) + ": " + name + " must be a non-negative real "
            "number or the k_unassigned_size sentinel.");
    }
    return x;
}

} // end of <anonymous> namespace
