/****************************************************************************

    File: Button.cpp
    Author: Andrew Janke
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

#include <ksg/Button.hpp>
#include <ksg/Frame.hpp>

#include <cassert>

namespace {

using StyleMap = ksg::StyleMap;
using InvalidArg = std::invalid_argument;

bool is_in_drect(int x, int y, const DrawRectangle & drect) {
    return (x >= drect.x() && y >= drect.y()
            && x <= drect.x() + drect.width()
            && y <= drect.y() + drect.height());
}

bool is_click_in(const sf::Event::MouseButtonEvent & mouse,
              const DrawRectangle & drect)
{
    return is_in_drect(mouse.x, mouse.y, drect);
}

bool is_mouse_in(const sf::Event::MouseMoveEvent & mouse,
              const DrawRectangle & drect)
{
    return is_in_drect(mouse.x, mouse.y, drect);
}
} // end of <anonymous> namespace

namespace ksg {

void Button::process_event(const sf::Event & evnt) {
    switch (evnt.type) {
    case sf::Event::MouseButtonReleased:
        if (m_is_highlighted && is_click_in(evnt.mouseButton, m_outer)) {
            press();
        }
        break;
    case sf::Event::MouseMoved:
        if (is_mouse_in(evnt.mouseMove, m_outer))
            highlight();
        else
            deselect();
        break;
    case sf::Event::MouseLeft: case sf::Event::LostFocus:
    case sf::Event::Resized:
        deselect();
        break;
    default: break;
    }
}

void Button::set_location(float x, float y) {
    float old_x = location().x, old_y = location().y;
    m_outer.set_position(x, y);
    m_inner.set_position(x + m_padding, y + m_padding);

    on_location_changed(old_x, old_y);
}

void Button::set_style(const StyleMap & smap) {
    StyleFinder sfinder(smap);
    sfinder.set_if_found(HOVER_BACK_COLOR, &m_hover.back);
    sfinder.set_if_found(HOVER_FRONT_COLOR, &m_hover.front);
    sfinder.set_if_found(REG_BACK_COLOR, &m_reg.back);
    sfinder.set_if_found(REG_FRONT_COLOR, &m_reg.front);
    sfinder.set_if_found(Frame::GLOBAL_PADDING, &m_padding);

    m_outer.set_color(m_reg.back);
    m_inner.set_color(m_reg.front);
}

void Button::set_press_event(BlankFunctor && func) {
    m_press_functor = std::move(func);
}

void Button::press() {
    if (m_press_functor)
        m_press_functor();
}

void Button::set_size(float width_, float height_) {
    if (width_ <= 0.f || height_ <= 0.f) {
        throw InvalidArg("ksg::Button::set_size: width and height must be "
                         "positive real numbers (which excludes zero)."    );
    }

    float old_width = width(), old_height = height();

    set_button_frame_size(width_, height_);
    set_size_back(width_, height_);

    on_size_changed(old_width, old_height);
}

/* protected */ Button::Button(): m_padding(0.f), m_is_highlighted(false) {}

/* protected */ void Button::draw
    (sf::RenderTarget & target, sf::RenderStates) const
{
    target.draw(m_outer);
    target.draw(m_inner);
}

/* protected */ void Button::on_size_changed(float, float) { }

/* protected */ void Button::on_location_changed(float, float) { }

/* protected */ void Button::set_size_back(float, float) { }

/* protected */ void Button::set_button_frame_size
    (float width_, float height_)
{
    m_outer.set_size(width_, height_);
    m_inner.set_size(std::max(width_  - m_padding*2.f, 0.f),
                     std::max(height_ - m_padding*2.f, 0.f));
}

/* private */ void Button::deselect() {
    m_is_highlighted = false;
    m_outer.set_color(m_reg.back);
    m_inner.set_color(m_reg.front);
}

/* private */ void Button::highlight() {
    m_is_highlighted = true;
    m_outer.set_color(m_hover.back);
    m_inner.set_color(m_hover.front);
}

} // end of ksg namespace