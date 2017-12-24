/****************************************************************************

    File: ProgressBar.cpp
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

#include <ksg/ProgressBar.hpp>
#include <ksg/Visitor.hpp>

#include <SFML/Graphics/RenderTarget.hpp>

namespace ksg {

namespace {

static const char * const FILL_OUT_OF_RANGE_MSG =
    "ProgressBar::set_fill_amount: fill amount is not in range: [0 1].";

} // end of <anonymous> namespace

using VectorF = ksg::Widget::VectorF;

ProgressBar::ProgressBar(): m_fill_amount(0.f), m_padding(0.f) {}

void ProgressBar::process_event(const sf::Event &) {}

void ProgressBar::set_location(float x, float y) {
    m_outer.set_position(x, y);
    update_positions_using_outer();
}

VectorF ProgressBar::location() const
    { return VectorF(m_outer.x(), m_outer.y()); }

void ProgressBar::set_size(float w, float h) {
    m_outer.set_size(w, h);
    update_sizes_using_outer();
}

float ProgressBar::width() const
    { return m_outer.width(); }

float ProgressBar::height() const
    { return m_outer.height(); }

void ProgressBar::set_style(const StyleMap & smap) {
    using std::bind;
    using namespace std::placeholders;
    StyleFinder sfinder(smap);
    sfinder.set_if_found(PADDING, &m_padding);
    sfinder.call_if_found<sf::Color>
        (OUTER_COLOR, bind(&DrawRectangle::set_color, &m_outer, _1));
    sfinder.call_if_found<sf::Color>
        (INNER_FRONT_COLOR, bind(&DrawRectangle::set_color, &m_inner_front, _1));
    sfinder.call_if_found<sf::Color>
        (INNER_BACK_COLOR, bind(&DrawRectangle::set_color, &m_inner_back, _1));
    update_positions_using_outer();
    update_sizes_using_outer();
}

void ProgressBar::accept(Visitor & visitor)
    { visitor.visit(*this); }

void ProgressBar::accept(const Visitor & visitor) const
    { visitor.visit(*this); }

void ProgressBar::set_outer_color(sf::Color color_)
    { m_outer.set_color(color_); }

void ProgressBar::set_inner_front_color(sf::Color color_)
    { m_inner_front.set_color(color_); }

void ProgressBar::set_inner_back_color(sf::Color color_)
    { m_inner_back.set_color(color_); }

void ProgressBar::set_fill_amount(float fill_amount) {
    if (fill_amount < 0.f or fill_amount > 1.f)
        throw std::out_of_range(FILL_OUT_OF_RANGE_MSG);
    m_fill_amount = fill_amount;
    set_size(width(), height());
}

float ProgressBar::fill_amount() const
    { return m_fill_amount; }

void ProgressBar::set_padding(float p) {
    m_padding = p;
    update_sizes_using_outer();
}

/* protected */ void ProgressBar::draw
    (sf::RenderTarget & target, sf::RenderStates) const
{
    target.draw(m_outer      );
    target.draw(m_inner_back );
    target.draw(m_inner_front);
}

/* private */ float ProgressBar::active_padding() const {
    float padding;
    if (width() < m_padding or height() < m_padding)
        padding = 0.f;
    else
        padding = m_padding;
    return padding;
}

/* private */ void ProgressBar::update_positions_using_outer() {
    auto x = m_outer.x();
    auto y = m_outer.y();
    auto padding = active_padding();
    m_inner_back .set_position(x + padding, y + padding);
    m_inner_front.set_position(x + padding, y + padding);
}

/* private */ void ProgressBar::update_sizes_using_outer() {
    auto width  = m_outer.width ();
    auto height = m_outer.height();
    auto pad = active_padding();
    m_inner_back .set_size(width - pad*2.f, height - pad*2.f);
    m_inner_front.set_size((width - pad*2.f)*m_fill_amount,
                           height - pad*2.f                );
}

} // end of namespace ksg
