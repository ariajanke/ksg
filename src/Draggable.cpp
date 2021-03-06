/****************************************************************************

    File: Draggable.hpp
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

#include <ksg/Draggable.hpp>

#include <cmath>

namespace {

using InvalidArg = std::invalid_argument;

}

namespace ksg {

Draggable::~Draggable() {}

void Draggable::mouse_move(int x, int y) {
    if (!m_dragged) return;
    x -= m_drag_offset.x;
    y -= m_drag_offset.y;
    if (has_position_contraints()) {
        const auto & contr = m_position_contraints;
        x = std::min(x, contr.left + contr.width);
        x = std::max(x, contr.left);
        y = std::min(y, contr.top + contr.height);
        y = std::max(y, contr.top);
    }
    update_drag_position(x, y);
}

bool Draggable::mouse_click
    (int x, int y, const DrawRectangle & drect)
{
    if (!m_watch_drag_events) return false;
    if (is_in_rect(x, y, drect)) {
        m_dragged = true;
        m_drag_offset = sf::Vector2i
            (int(std::round(x - drect.x())), int(std::round(y - drect.y())));
        return true;
    }
    return false;
}

void Draggable::set_drag_contraints(sf::IntRect area) {
    if (area.width == 0 || area.height == 0) {
        throw InvalidArg("Draggable::set_position_contraints: Position "
                         "constraint area may not have a zero sized area.\n"
                         "Consider disabling this drag feature instead (if "
                         "possible).");
    }
    m_position_contraints = area;
}

void Draggable::remove_drag_contraints() {
    m_position_contraints = sf::IntRect();
}

/* private */ bool Draggable::has_position_contraints() const {
    return m_position_contraints.width  != 0 &&
           m_position_contraints.height != 0;
}

/* static */ bool Draggable::is_in_rect
    (int x, int y, const DrawRectangle & drect)
{
    return (x >= drect.x() && x <= (drect.x() + drect.width() ) &&
            y >= drect.y() && y <= (drect.y() + drect.height())   );
}

} // end of ksg namespace
