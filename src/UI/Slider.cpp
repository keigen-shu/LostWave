#include <cassert>
#include "Slider.hpp"

namespace UI {

constexpr int   kPadding = 4;

// a <= x <= b
inline bool is_in_range(int const &value, int const &a, int const &b) {
    return (b < a) ? is_in_range(value, b, a) : (value >= a && value <= b);
}

inline float get_normalized_value(int const &value, int const &a, int const &b) {
    return (b < a) ? get_normalized_value(value, b, a) : (static_cast<float>(value - a) / static_cast<float>(b - a));
}

Slider::Slider(clan::GUIComponent *parent)
    : clan::GUIComponent(parent, "Slider")
    , mDirection(Slider::Direction::RIGHT)
    , mMin(-1)
    , mMax( 1)
    , mValue ( 0)
    , mqThumbHold(false)
    , m_PointerPosition()
{
    func_render().set(this, &Slider::on_render);
    func_input().set(this, &Slider::on_input);
    // func_input already calls this.
    // func_input_pointer_moved().set(this, &Slider::on_pointer_move);
}

void Slider::on_render(clan::Canvas &canvas, recti const &clipRect) {
    point2i midpoint { get_width() / 2, get_height() / 2 };

    float pos = get_normalized_value(mValue, mMin, mMax);
    switch (mDirection) {
        case Slider::Direction::RIGHT:
            pos = pos * (get_width() - 2 * kPadding - 1);
            break;
        case Slider::Direction::LEFT:
            pos = (1.0f - pos) * (get_width() - 2 * kPadding - 1);
            break;
        case Slider::Direction::DOWN:
            pos = pos * (get_height() - 2 * kPadding - 1);
            break;
        case Slider::Direction::UP:
            pos = (1.0f - pos) * (get_height() - 2 * kPadding - 1);
            break;
    }

    pos = std::round(pos);
    assert(pos >= 0);
    pos = pos + kPadding;

    /****/ if (mDirection >= Slider::Direction::DOWN) { // Vertical slider
        canvas.draw_line(midpoint.x, kPadding, midpoint.x, get_height() - kPadding + 1, clan::Colorf::grey);
        canvas.fill_rect(
                0, pos - kPadding, get_width(), pos + kPadding + 1,
                mqThumbHold ? clan::Colorf::green : clan::Colorf::black
                );
        canvas.draw_line(0, pos + 1, get_width(), pos + 1, clan::Colorf::grey);
    } else { // Horizontal slider
        canvas.draw_line(kPadding, midpoint.y, get_width() - kPadding + 1, midpoint.y, clan::Colorf::grey);
        canvas.fill_rect(
                pos - kPadding, 0, pos + kPadding + 1, get_height(),
                mqThumbHold ? clan::Colorf::green : clan::Colorf::black
                );
        canvas.draw_line(pos + 1, 0, pos + 1, get_height(), clan::Colorf::grey);
    }

}


bool Slider::on_input(clan::InputEvent const &event) {
    if (event.device.get_type() == clan::InputDevice::Type::pointer) {
        switch (event.type) {
            case clan::InputEvent::Type::pointer_moved:
                return on_pointer_move(event);

            case clan::InputEvent::Type::pressed:
                if (event.id == clan::InputCode::mouse_left)
                    return on_pointer_lbtn_on(event);
                break;

            case clan::InputEvent::Type::released:
                if (event.id == clan::InputCode::mouse_left)
                    return on_pointer_lbtn_off(event);
                break;

            case clan::InputEvent::Type::axis_moved:
                // TODO: Implement Scroll wheel
                break;

        }
    }

    return false;
}

bool Slider::on_pointer_move(clan::InputEvent const &event) {
    assert(event.device.get_type() == clan::InputDevice::Type::pointer);
    assert(event.type == clan::InputEvent::Type::pointer_moved);

    if (mqThumbHold == true)
        return on_pointer_drag(event);

    // If pointer on top of thumb, make it pretty or something
}

bool Slider::on_pointer_drag(clan::InputEvent const &event) {
    assert(event.device.get_type() == clan::InputDevice::Type::pointer);
    assert(event.type == clan::InputEvent::Type::pointer_moved);
    assert(mqThumbHold == true);

    point2i new_pos = component_to_screen_coords(event.mouse_pos);
    clan::Console::write_line("(%1,%2)", new_pos.x, new_pos.y);
    char incr = 0;
    switch (mDirection) {
        case Direction::RIGHT:
            incr = new_pos.x - m_PointerPosition.x;
            break;
        case Direction::LEFT:
            incr = m_PointerPosition.x - new_pos.x;
            break;
        case Direction::DOWN:
            incr = new_pos.y - m_PointerPosition.y;
            break;
        case Direction::UP:
            incr = m_PointerPosition.y - new_pos.y;
            break;
    }

    incr = incr / std::abs(incr); // Strip magnitude

    if (incr == 0) {
        return true;
    } else {
        mValue // Take new value if it is inside the slider range.
                = is_in_range(mValue + incr, mMin, mMax)
                ? mValue + incr
                // or take the limit that is closer to the value.
                : mValue
                ;
    }

    m_PointerPosition = new_pos;

    // Invoke user callback
    if (mfValueChanged.is_null() == false)
        mfValueChanged.invoke();

    // Show tooltip.
    // Repaint component
    request_repaint();
}

bool Slider::on_pointer_lbtn_on(clan::InputEvent const &event) {
    assert(event.device.get_type() == clan::InputDevice::Type::pointer);
    assert(event.id == clan::InputCode::mouse_left);
    assert(event.type == clan::InputEvent::Type::pressed);

    if (mqThumbHold == false) {
        mqThumbHold = true;
        m_PointerPosition = event.device.get_position();
        capture_mouse(true);
        request_repaint();
    }

    return true;
}

bool Slider::on_pointer_lbtn_off(clan::InputEvent const &event) {
    assert(event.device.get_type() == clan::InputDevice::Type::pointer);
    assert(event.id == clan::InputCode::mouse_left);
    assert(event.type == clan::InputEvent::Type::released);

    if (mqThumbHold == true) {
        mqThumbHold = false;
        m_PointerPosition = event.device.get_position();
        capture_mouse(false);
        request_repaint();
    }

    return true;
}


}
