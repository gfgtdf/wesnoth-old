/*
   Copyright (C) 2008 - 2017 by Mark de Wever <koraq@xs4all.nl>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#define GETTEXT_DOMAIN "wesnoth-lib"

#include "gui/widgets/slider_base.hpp"

#include "gui/core/log.hpp"
#include "gui/widgets/window.hpp" // Needed for invalidate_layout()

#include "utils/functional.hpp"

#define LOG_SCOPE_HEADER get_control_type() + " [" + id() + "] " + __func__
#define LOG_HEADER LOG_SCOPE_HEADER + ':'

namespace gui2
{

slider_base::slider_base()
	: styled_widget(COUNT)
	, state_(ENABLED)
	, item_count_(0)
	, item_position_(0)
	, pixels_per_step_(0.0)
	, drag_initial_mouse_(0, 0)
	, drag_initial_position_(0)
	, positioner_offset_(0)
	, positioner_length_(0)
{
	connect_signal<event::MOUSE_ENTER>(std::bind(
			&slider_base::signal_handler_mouse_enter, this, _2, _3, _4));
	connect_signal<event::MOUSE_MOTION>(std::bind(
			&slider_base::signal_handler_mouse_motion, this, _2, _3, _4, _5));
	connect_signal<event::MOUSE_LEAVE>(std::bind(
			&slider_base::signal_handler_mouse_leave, this, _2, _3));
	connect_signal<event::LEFT_BUTTON_DOWN>(std::bind(
			&slider_base::signal_handler_left_button_down, this, _2, _3));
	connect_signal<event::LEFT_BUTTON_UP>(std::bind(
			&slider_base::signal_handler_left_button_up, this, _2, _3));
}

void slider_base::scroll(const scroll_mode scroll)
{
	switch(scroll) {
		case BEGIN:
			set_slider_position(0);
			break;

		case ITEM_BACKWARDS:
			set_slider_position(item_position_ - 1);
			break;

		case HALF_JUMP_BACKWARDS:
			set_slider_position(item_position_  - jump_size() / 2);
			break;

		case JUMP_BACKWARDS:
			set_slider_position(item_position_ - jump_size());
			break;

		case END:
			set_slider_position(item_count_ - 1);
			break;

		case ITEM_FORWARD:
			set_slider_position(item_position_ + 1);
			break;

		case HALF_JUMP_FORWARD:
			set_slider_position(item_position_ + jump_size() / 2);
			break;

		case JUMP_FORWARD:
			set_slider_position(item_position_ + jump_size());
			break;

		default:
			assert(false);
	}

	fire(event::NOTIFY_MODIFIED, *this, nullptr);
}

void slider_base::place(const point& origin, const point& size)
{
	// Inherited.
	styled_widget::place(origin, size);

	recalculate();
}

void slider_base::set_active(const bool active)
{
	if(get_active() != active) {
		set_state(active ? ENABLED : DISABLED);
	}
}

bool slider_base::get_active() const
{
	return state_ != DISABLED;
}

unsigned slider_base::get_state() const
{
	return state_;
}

void slider_base::set_slider_position(int item_position)
{
	// Set the value always execute since we update a part of the state.
	item_position_ = utils::clamp(item_position, 0, item_count_ - 1);

	// Determine the pixel offset of the item position.
	positioner_offset_
			= static_cast<unsigned>(item_position_ * pixels_per_step_);

	update_canvas();
}

void slider_base::update_canvas()
{

	for(auto & tmp : get_canvases())
	{
		tmp.set_variable("positioner_offset", wfl::variant(positioner_offset_));
		tmp.set_variable("positioner_length", wfl::variant(positioner_length_));
	}
	set_is_dirty(true);
}

void slider_base::set_state(const state_t state)
{
	if(state != state_) {
		state_ = state;
		set_is_dirty(true);
	}
}

void slider_base::recalculate()
{
	// We can be called before the size has been set up in that case we can't do
	// the proper recalcultion so stop before we die with an assert.
	if(!get_length()) {
		return;
	}

	// Get the available size for the slider to move.
	const int available_length = get_length() - offset_before()
								 - offset_after();

	assert(available_length > 0);

	// All visible.
	if(item_count_ <= 1) {
		positioner_offset_ = offset_before();
		recalculate_positioner();
		item_position_ = 0;
		update_canvas();
		return;
	}

	const unsigned steps = item_count_ - 2;

	recalculate_positioner();

	// Make sure we can also show the last step, so add one more step.
	pixels_per_step_ = (available_length - positioner_length_)
					   / static_cast<float>(steps + 1);

	set_slider_position(item_position_);

}

void slider_base::move_positioner(int distance)
{
	int new_position = item_position_;
	int new_positioner_offset = positioner_offset_;

	if (true) {
		// 'snap' the slider to allowed positions
		int steps_diff = std::round(distance / pixels_per_step_);
		new_position = drag_initial_position_ + steps_diff;
		new_positioner_offset = offset_before() + (available_length() - positioner_length_) * item_count_ / new_position;
	}
	else {
		// continious dragging. todo: fix this case and add a bool snap_ field in [slider]
		if (distance < 0 && -distance > static_cast<int>(positioner_offset_)) {
			new_positioner_offset = 0;
		}
		else {
			new_positioner_offset += distance;
		}

		int length = get_length() - offset_before() - offset_after() - positioner_length_;

		if (new_positioner_offset > length) {
			new_positioner_offset = length;
		}

		new_position = static_cast<int>(new_positioner_offset / pixels_per_step_);

		// Note due to floating point rounding the position might be outside the
		// available positions so set it back.
		new_position = utils::clamp(new_position, 0, item_count_ - 1);
	}

	positioner_offset_ = new_positioner_offset;

	if(new_position != item_position_) {
		item_position_ = new_position;

		child_callback_positioner_moved();

		fire(event::NOTIFY_MODIFIED, *this, nullptr);
	}

	update_canvas();
}

void slider_base::load_config_extra()
{
	// These values won't change so set them here.
	for(auto & tmp : get_canvases())
	{
		tmp.set_variable("offset_before", wfl::variant(offset_before()));
		tmp.set_variable("offset_after", wfl::variant(offset_after()));
	}
}

void slider_base::signal_handler_mouse_enter(const event::ui_event event,
											 bool& handled,
											 bool& halt)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	// Send the motion under our event id to make debugging easier.
	signal_handler_mouse_motion(event, handled, halt, get_mouse_position());
}

void slider_base::signal_handler_mouse_motion(const event::ui_event event,
											  bool& handled,
											  bool& halt,
											  const point& coordinate)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << " at " << coordinate << ".\n";

	point mouse = coordinate;
	mouse.x -= get_x();
	mouse.y -= get_y();

	switch(state_) {
		case ENABLED:
			if(on_positioner(mouse)) {
				set_state(FOCUSED);
			}

			break;

		case PRESSED: {
			if(in_orthogonal_range(mouse)) {
				const int distance = get_length_difference(drag_initial_mouse_, mouse);
				move_positioner(distance);
			}

		} break;

		case FOCUSED:
			if(!on_positioner(mouse)) {
				set_state(ENABLED);
			}
			break;

		case DISABLED:
			// Shouldn't be possible, but seems to happen in the lobby
			// if a resize layout happens during dragging.
			halt = true;
			break;

		default:
			assert(false);
	}
	handled = true;
}

void slider_base::signal_handler_mouse_leave(const event::ui_event event,
											 bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	if(state_ == FOCUSED) {
		set_state(ENABLED);
	}
	handled = true;
}


void slider_base::signal_handler_left_button_down(const event::ui_event event,
												  bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	point mouse = get_mouse_position();
	mouse.x -= get_x();
	mouse.y -= get_y();

	if(on_positioner(mouse)) {
		assert(get_window());
		drag_initial_mouse_ = mouse;
		drag_initial_position_ = item_position_;

		get_window()->mouse_capture();
		set_state(PRESSED);
	}

	const int bar = on_bar(mouse);

	if(bar == -1) {
		scroll(HALF_JUMP_BACKWARDS);
		fire(event::NOTIFY_MODIFIED, *this, nullptr);
	} else if(bar == 1) {
		scroll(HALF_JUMP_FORWARD);
		fire(event::NOTIFY_MODIFIED, *this, nullptr);
	} else {
		assert(bar == 0);
	}

	handled = true;
}

void slider_base::signal_handler_left_button_up(const event::ui_event event,
												bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	point mouse = get_mouse_position();
	mouse.x -= get_x();
	mouse.y -= get_y();

	if(state_ != PRESSED) {
		return;
	}

	assert(get_window());
	get_window()->mouse_capture(false);

	if(on_positioner(mouse)) {
		set_state(FOCUSED);
	} else {
		set_state(ENABLED);
	}

	drag_initial_mouse_ = {0, 0};
	drag_initial_position_ = 0;

	handled = true;
}

} // namespace gui2
