/****************************************************************************

    File: Frame.cpp
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

#include <common/Util.hpp>

#include <ksg/Frame.hpp>
#include <ksg/TextArea.hpp>
#include <ksg/FocusWidget.hpp>

#include <SFML/Graphics/RenderTarget.hpp>

#include <stdexcept>
#include <iostream>
#include <cassert>

namespace {

using VectorF = ksg::Frame::VectorF;

} // end of <anonymous> namespace

namespace ksg {

WidgetAdder::WidgetAdder
    (Frame * frame_, const StyleMap * styles_, detail::LineSeperator * sep_):
    m_the_line_sep(sep_),
    m_styles(styles_),
    m_parent(frame_)
{
    static constexpr const char * k_null_parent =
        "WidgetAdder::WidgetAdder: [library error] Parent must not be null, "
        "and line seperator must refer to something.";
    if (!m_parent || !m_the_line_sep) {
        throw std::invalid_argument(k_null_parent);
    }
}

WidgetAdder::WidgetAdder(WidgetAdder && rhs)
    { swap(rhs); }

WidgetAdder::~WidgetAdder() noexcept(false) {
    if (std::uncaught_exceptions() > 0) return;
    if (!m_parent) return;
    m_parent->finalize_widgets(
        std::move(m_widgets), std::move(m_horz_spacers),
        m_the_line_sep, m_styles);
}

WidgetAdder & WidgetAdder::operator = (WidgetAdder && rhs) {
    swap(rhs);
    return *this;
}

WidgetAdder & WidgetAdder::add(Widget & widget) {
    // we check later whether or not this frame is trying to include itself
    // see Frame::finalize_widgets(
    // std::vector<Widget *> &&, std::vector<detail::HorizontalSpacer> &&,
    // detail::LineSeperator *, const StyleMap *)
    m_widgets.push_back(&widget);
    return *this;
}

WidgetAdder & WidgetAdder::add_horizontal_spacer() {
    // A little word on the C++ standard:
    // According to: http://stackoverflow.com/questions/5410035/when-does-a-stdvector-reallocate-its-memory-array
    // From C++ standard 23.2.4.2
    // <quote>
    // It is guaranteed that no reallocation takes place during insertions that
    // happen after a call to reserve() until the time when an insertion would
    // make the size of the vector greater than the size specified in the most
    // recent call to reserve().
    // </quote>

    // update and revalidate all pointers to spacers as necessary
    using namespace detail;
    if (   m_horz_spacers.size() == m_horz_spacers.capacity()
        && !m_horz_spacers.empty())
    {
        std::vector<HorizontalSpacer> new_spacer_cont;
        // reserve and copy
        new_spacer_cont.reserve(1 + m_horz_spacers.capacity()*2);
        (void)std::copy(m_horz_spacers.begin(), m_horz_spacers.end(),
                        std::back_inserter(new_spacer_cont));
        // revalidate pointers in the widget container
        HorizontalSpacer * new_itr = &new_spacer_cont[0];
        const HorizontalSpacer * old_itr = &m_horz_spacers[0];
        for (Widget *& widget_ptr : m_widgets) {
            if (widget_ptr == old_itr) {
                widget_ptr = new_itr++;
                ++old_itr;
            }
        }
        // safely get the end() pointer
        assert(new_itr == (new_spacer_cont.data() + new_spacer_cont.size()));
        m_horz_spacers.swap(new_spacer_cont);
    }
    m_horz_spacers.push_back(HorizontalSpacer());
    m_widgets.push_back(&m_horz_spacers.back());

    return *this;
}

WidgetAdder & WidgetAdder::add_line_seperator() {
    m_widgets.push_back(m_the_line_sep);
    return *this;
}

void WidgetAdder::swap(WidgetAdder & rhs) {
    m_widgets.swap(rhs.m_widgets);
    m_horz_spacers.swap(rhs.m_horz_spacers);
    std::swap(m_the_line_sep, rhs.m_the_line_sep);
    std::swap(m_styles, rhs.m_styles);
    std::swap(m_parent, rhs.m_parent);
}

// ----------------------------------------------------------------------------

/* static */ constexpr const char * const Frame::k_background_color ;
/* static */ constexpr const char * const Frame::k_title_bar_color  ;
/* static */ constexpr const char * const Frame::k_title_size       ;
/* static */ constexpr const char * const Frame::k_title_color      ;
/* static */ constexpr const char * const Frame::k_widget_body_color;
/* static */ constexpr const char * const Frame::k_border_size      ;

/* static */ constexpr const float Frame::k_default_padding;

/* protected */ Frame::Frame()
    { check_invarients(); }

/* protected */ Frame::Frame(const Frame & lhs):
    m_padding(lhs.m_padding),
    m_border (lhs.m_border )
{}

/* protected */ Frame::Frame(Frame && lhs) { swap(lhs); }

Frame & Frame::operator = (const Frame & lhs) {
    if (this != &lhs) {
        Frame temp(lhs);
        swap(temp);
    }
    return *this;
}

Frame & Frame::operator = (Frame && lhs) {
    if (this != &lhs) swap(lhs);
    return *this;
}

void Frame::set_location(float x, float y) {
    m_border.set_location(x, y);
    check_invarients();
}

void Frame::process_event(const sf::Event & event) {
    auto gv = m_border.process_event(event);
    if (!gv.skip_other_events) {
        for (Widget * widget_ptr : m_widgets) {
            if (widget_ptr->is_visible())
                widget_ptr->process_event(event);
        }
        // perhaps I should process focus requests after the fact to give
        // widgets the opportunity to make a request after an event
        m_focus_handler.process_event(event);
    }
    if (gv.should_update_geometry) {
        finalize_widgets();
    }

    check_invarients();
}

void Frame::set_style(const StyleMap & smap) {
    m_border.set_style(smap);
    if (!styles::set_if_found(smap, styles::k_global_padding, m_padding)) {
        m_padding = k_default_padding;
    }

    for (Widget * widget_ptr : m_widgets)
        widget_ptr->set_style(smap);
    check_invarients();
}

VectorF Frame::location() const { return m_border.location(); }

float Frame::width() const { return m_border.width(); }

float Frame::height() const { return m_border.height(); }

void Frame::set_size(float w, float h) {
    m_border.set_size(w, h);
    check_invarients();
}

WidgetAdder Frame::begin_adding_widgets(const StyleMap & styles) {
    return WidgetAdder(this, &styles, &m_the_line_seperator);
}

WidgetAdder Frame::begin_adding_widgets() {
    return WidgetAdder(this, nullptr, &m_the_line_seperator);
}

void Frame::finalize_widgets(std::vector<Widget *> && widgets,
    std::vector<detail::HorizontalSpacer> && spacers,
    detail::LineSeperator * the_line_sep, const StyleMap * styles)
{
    static constexpr const char * k_must_know_line_sep =
        "Frame::finalize_widgets: caller must know the line seperator to call "
        "this function. This is meant to be called by a Widget Adder only.";
    if (the_line_sep != &m_the_line_seperator) {
        throw std::invalid_argument(k_must_know_line_sep);

    }

    static constexpr const char * k_cannot_contain_this =
        "Frame::finalize_widgets: This frame may not contain itself.";
    for (auto * widget : widgets) {
        if (auto * frame_widget = dynamic_cast<Frame *>(widget)) {
            if (frame_widget->contains(this)) {
                throw std::invalid_argument(k_cannot_contain_this);
            }
        }
    }

    m_widgets     .swap(widgets);
    m_horz_spacers.swap(spacers);

    if (styles) {
        set_style(*styles);
        // styles must be provided in order to finalize widgets
        finalize_widgets();
    }
    check_invarients();
}

void Frame::set_padding(float pixels)
    { m_padding = pixels; }

void Frame::set_frame_border_size(float pixels)
    { m_border.set_border_size(pixels); }

/* protected */ void Frame::draw(sf::RenderTarget & target, sf::RenderStates) const {
    if (!is_visible()) return;

    target.draw(m_border);
    for (Widget * widget_ptr : m_widgets) {
        if (widget_ptr->is_visible())
            target.draw(*widget_ptr);
    }
}

/* private */ void Frame::finalize_widgets() {
    // auto sizing
    issue_auto_resize();

    // must come before horizontal spacer updates
    m_border.update_geometry();

    // update horizontal spacer sizes
    update_horizontal_spacers();

    const float start_x = m_border.widget_start().x + m_padding;
    float x = start_x;
    float y = m_border.widget_start().y + m_padding;

    float line_height = 0.f;
    float pad_fix     = 0.f;
    auto advance_locals_to_next_line = [&]() {
        y += line_height + m_padding;
        x = start_x;
        line_height = 0.f;
        pad_fix = 0.f;
    };
    const float k_right_limit = location().x + width();
    for (Widget * widget_ptr : m_widgets) {
        assert(widget_ptr);
        // horizontal overflow
        if (is_line_seperator(widget_ptr)) {
            advance_locals_to_next_line();
            continue;
        }
        if (x + get_widget_advance(widget_ptr) > k_right_limit) {
            advance_locals_to_next_line();
            // this widget_ptr is placed as the first element of the line
        }
        if (is_horizontal_spacer(widget_ptr))
            x += pad_fix;

        widget_ptr->set_location(x, y);

        line_height = std::max(widget_ptr->height(), line_height);

        // horizontal advance
        x += get_widget_advance(widget_ptr);
        pad_fix = -m_padding;
    }

    for (Widget * widget_ptr : m_widgets) {
        auto * frame_ptr = dynamic_cast<Frame *>(widget_ptr);
        if (frame_ptr)
            frame_ptr->finalize_widgets();
    }

    // note: there is no consideration given to "vertical overflow"
    // not considering if additional widgets overflow the frame's
    // height
    std::vector<FocusWidget *> focus_widgets;
    iterate_children_f([&focus_widgets](Widget & widget) {
        if (auto * frame = dynamic_cast<Frame *>(&widget)) {
            frame->m_focus_handler.clear_focus_widgets();
        }
        if (auto * focwid = dynamic_cast<FocusWidget *>(&widget)) {
            focus_widgets.push_back(focwid);
        }
    });
    m_focus_handler.take_widgets_from(focus_widgets);

    check_invarients();
}

void Frame::swap(Frame & lhs) {
    std::swap(m_padding, lhs.m_padding);
    std::swap(m_border , lhs.m_border );
}

/* private */ VectorF Frame::compute_size_to_fit() const {
    float total_width  = 0.f;
    float line_width   = 0.f;
    float total_height = 0.f;
    float line_height  = 0.f;
    float pad_fix      = 0.f;

    for (Widget * widget_ptr : m_widgets) {
        assert(widget_ptr);
        if (is_horizontal_spacer(widget_ptr)) {
            pad_fix = -m_padding;
            continue;
        }
        if (is_line_seperator(widget_ptr)) {
            total_width   = std::max(line_width, total_width);
            assert(is_real(total_width));
            line_width    = 0.f;
            total_height += line_height + m_padding;
            line_height   = 0.f;
            pad_fix       = 0.f;
            continue;
        }
        float width  = widget_ptr->width ();
        float height = widget_ptr->height();
        Frame * widget_as_frame = nullptr;
        // should I issue auto-resize here?
        if (width == 0.f && height == 0.f
            && (widget_as_frame = dynamic_cast<Frame *>(widget_ptr)))
        {
            auto gv = widget_as_frame->compute_size_to_fit();
            width  = gv.x;
            height = gv.y;
        }
        line_width  += get_widget_advance(widget_ptr) + pad_fix;
        line_height  = std::max(line_height, height);
        pad_fix = 0.f;
        if (!is_real(line_width)) {
            get_widget_advance(widget_ptr);
        }
        assert(is_real(line_width) && is_real(line_height));
    }

    if (line_width != 0.f) {
        total_width   = std::max(total_width, line_width);
        total_height += line_height + m_padding;
        assert(is_real(total_width));
    }
    // we want to fit for the title's width and height also
    // accommodate for the title
    total_height += (m_border.widget_start() - m_border.location()).y;
    total_width = std::max(total_width, m_border.title_width_accommodation() + m_padding*2.f);
    assert(is_real(total_width));

    if (!m_widgets.empty()) {
        // padding for borders + padding on end
        // during normal iteration we include only one
        total_width  += m_padding*3.f;
        total_height += m_padding*3.f;
    }
    return VectorF(total_width, total_height);
}

/* private */ bool Frame::is_horizontal_spacer
    (const Widget * widget) const
{
    if (m_horz_spacers.empty()) return false;
    return !(widget < &m_horz_spacers.front() || widget > &m_horz_spacers.back());
}

/* private */ bool Frame::is_line_seperator(const Widget * widget) const
    { return widget == &m_the_line_seperator; }

/* private */ float Frame::get_widget_advance(const Widget * widget_ptr) const {
    bool is_special_widget = is_line_seperator(widget_ptr) ||
                             is_horizontal_spacer(widget_ptr);
    return widget_ptr->width() + (is_special_widget ? 0.f : m_padding);
}

/* private */ void Frame::issue_auto_resize() {
    // ignore auto resize if the frame as a width/height already set
    for (Widget * widget_ptr : m_widgets)
        widget_ptr->issue_auto_resize();

    issue_auto_resize_for_frame();

    if (width() == 0.f || height() == 0.f) {
        auto size_ = compute_size_to_fit();
        set_size(size_.x, size_.y);
    }
}

/* private */ bool Frame::contains(const Widget * wptr) const noexcept {
    for (const auto * widget : m_widgets) {
        if (widget == wptr) return true;
        if (const auto * frame = dynamic_cast<const Frame *>(widget)) {
            if (frame->contains(wptr)) return true;
        }
    }
    return false;
}

/* private */ void Frame::iterate_children_(ChildWidgetIterator & itr) {
    for (auto * widget : m_widgets) {
        itr.on_child(*widget);
        widget->iterate_children(itr);
    }
}

/* private */ void Frame::iterate_const_children_(ChildWidgetIterator & itr) const {
    for (const auto * widget : m_widgets) {
        itr.on_child(*widget);
        widget->iterate_children(itr);
    }
}

/* private */ void Frame::check_invarients() const {
    assert(!is_nan(width ()) && width () >= 0.f);
    assert(!is_nan(height()) && height() >= 0.f);
}

/* private */ void Frame::update_horizontal_spacers() {
    const float k_horz_space = m_border.width_available_for_widgets();
    const float k_start_x    = 0.f;
    assert(k_horz_space >= 0.f);
    // horizontal spacers:
    // will have to find out how much horizontal space is available per line
    // first. Next, if there are any horizontal spacers, each of them carries
    // an equal amount of the left over space.
    float x = k_start_x;
    float pad_fix = 0.f;
    auto line_begin = m_widgets.begin();
    for (auto itr = m_widgets.begin(); itr != m_widgets.end(); ++itr) {
        Widget * widget_ptr = *itr;
        assert(widget_ptr);
        // if the widget follow another non spacer is a spacer than no
        // padding is added
        if (is_horizontal_spacer(widget_ptr)) {
            x += pad_fix;
            pad_fix = 0.f;
            continue;
        }
        pad_fix = -m_padding;
        float horz_step = get_widget_advance(widget_ptr);

        // horizontal overflow or end of widgets
        if (x + horz_step > k_horz_space || is_line_seperator(widget_ptr)) {
            // at the end of the line, set the widths for the horizontal
            // spacers
            line_begin = set_horz_spacer_widths
                (line_begin, itr, std::max(k_horz_space - x, 0.f), m_padding);

            // advance to new line
            x = k_start_x;
            pad_fix = 0.f;
        } // end of horizontal overflow handling

        // horizontal advance
        x += horz_step;
    } // looping through widgets

    if (line_begin == m_widgets.end()) return;

    set_horz_spacer_widths
        (line_begin, m_widgets.end(), std::max(k_horz_space - x, 0.f), m_padding);
}

/* private */ Frame::WidgetItr Frame::set_horz_spacer_widths
    (WidgetItr beg, WidgetItr end, float left_over_space, float padding)
{
    assert(left_over_space >= 0.f);
    int horz_spacer_count = 0;
    for (auto jtr = beg; jtr != end; ++jtr) {
        if (is_horizontal_spacer(*jtr))
            ++horz_spacer_count;
    }

    // if no horizontal spacers, space cannot be split up
    if (horz_spacer_count == 0) return end;

    // distribute left over space to spacers
    float width_per_spacer = (left_over_space / horz_spacer_count) - padding;
    width_per_spacer = std::max(0.f, width_per_spacer);
    for (auto jtr = beg; jtr != end; ++jtr) {
        if (!is_horizontal_spacer(*jtr)) continue;
        auto horz_spacer = dynamic_cast<HorizontalSpacer *>(*jtr);
        assert(horz_spacer);
        // always move on the next state
        horz_spacer->set_width(width_per_spacer);
    }

    return end;
}

// ----------------------------------------------------------------------------

// anchor vtable for clang
SimpleFrame::~SimpleFrame() {}

} // end of ksg namespace
