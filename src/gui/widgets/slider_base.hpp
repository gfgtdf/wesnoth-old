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

#pragma once

#include "gui/core/notifier.hpp"
#include "gui/widgets/styled_widget.hpp"

#include "utils/functional.hpp"

namespace gui2
{

/**
 * Base class for a scroll bar.
 *
 * class will be subclassed for the horizontal and vertical scroll bar.
 * It might be subclassed for a slider class.
 *
 * To make this class generic we talk a lot about offset and length and use
 * pure virtual functions. The classes implementing us can use the heights or
 * widths, whichever is applicable.
 *
 * The NOTIFY_MODIFIED event is send when the position of slider is changed.
 *
 * Common signal handlers:
 * - connect_signal_notify_modified
 */
class slider_base : public styled_widget
{
	/** @todo Abstract the code so this friend is no longer needed. */
	friend class slider;

public:
	slider_base();

	/**
	 * scroll 'step size'.
	 *
	 * When scrolling we always scroll a 'fixed' amount, these are the
	 * parameters for these amounts.
	 */
	enum scroll_mode {
		BEGIN,				 /**< Go to begin position. */
		ITEM_BACKWARDS,		 /**< Go one item towards the begin. */
		HALF_JUMP_BACKWARDS, /**< Go half the visible items towards the begin.
								*/
		JUMP_BACKWARDS,	/**< Go the visibile items towards the begin. */
		END,			   /**< Go to the end position. */
		ITEM_FORWARD,	  /**< Go one item towards the end. */
		HALF_JUMP_FORWARD, /**< Go half the visible items towards the end. */
		JUMP_FORWARD
	}; /**< Go the visible items towards the end. */

	/**
	 * Sets the item position.
	 *
	 * We scroll a predefined step.
	 *
	 * @param scroll              'step size' to scroll.
	 */
	void scroll(const scroll_mode scroll);
protected:
	/** Is the positioner at the beginning of the slider? */
	bool at_begin() const
	{
		return item_position_ == 0;
	}

	/**
	 * Is the positioner at the and of the slider?
	 *
	 * Note both begin and end might be true at the same time.
	 */
	bool at_end() const
	{
		return item_position_ + 1 >= item_count_;
	}
public:

	/***** ***** ***** ***** layout functions ***** ***** ***** *****/

	/** See @ref widget::place. */
	virtual void place(const point& origin, const point& size) override;

	/***** ***** ***** ***** Inherited ***** ***** ***** *****/

	/** See @ref styled_widget::set_active. */
	virtual void set_active(const bool active) override;

	/** See @ref styled_widget::get_active. */
	virtual bool get_active() const override;

	/** See @ref styled_widget::get_state. */
	virtual unsigned get_state() const override;

    /**
	 * Possible states of the widget.
	 *
	 * Note the order of the states must be the same as defined in settings.hpp.
	 */
	enum state_t {
		ENABLED,
		DISABLED,
		PRESSED,
		FOCUSED,
		COUNT
	};

protected:
	/***** ***** ***** setters / getters for members ***** ****** *****/

	void slider_set_item_count(const unsigned item_count)
	{
		item_count_ = item_count;
		recalculate();
	}
	unsigned slider_get_item_count() const
	{
		return item_count_;
	}

	/**
	 * Note the position isn't guaranteed to be the wanted position
	 * the step size is honored. The value will be rouded down.
	 */
	void set_slider_position(int item_position);

	unsigned get_slider_position() const
	{
		return item_position_;
	}

	float get_pixels_per_step() const
	{
		return pixels_per_step_;
	}

	unsigned get_positioner_offset() const
	{
		return positioner_offset_;
	}

	unsigned get_positioner_length() const
	{
		return positioner_length_;
	}

	/**
	 * See @ref styled_widget::update_canvas.
	 *
	 * After a recalculation the canvasses also need to be updated.
	 */
	virtual void update_canvas() override;

	/**
	 * Callback for subclasses to get notified about positioner movement.
	 *
	 * @todo This is a kind of hack due to the fact there's no simple
	 * callback slot mechanism. See whether we can implement a generic way to
	 * attach callback which would remove quite some extra code.
	 */
	virtual void child_callback_positioner_moved()
	{
	}

	virtual int jump_size() const { return 1;  };

private:
	void set_state(const state_t state);
	/**
	 * Current state of the widget.
	 *
	 * The state of the widget determines what to render and how the widget
	 * reacts to certain 'events'.
	 */
	state_t state_;

	/** The number of items the slider 'holds'. */
	int item_count_;

	/** The item the positioner is at, starts at 0. */
	int item_position_;

	/**
	 * Number of pixels per step.
	 *
	 * The number of pixels the positioner needs to move to go to the next step.
	 * Note if there is too little space it can happen 1 pixel does more than 1
	 * step.
	 */
	float pixels_per_step_;

	/**
	 * The position the mouse was when draggin the slider was started.
	 *
	 * This is used during dragging the positioner.
	 */
	point drag_initial_mouse_;
	/**
	* The position the slider was when draggin the slider was started.
	*
	* This is used during dragging the positioner.
	*/
	int drag_initial_position_;

	point mouse2_;

	/**
	 * The start offset of the positioner.
	 *
	 * This takes the offset before in consideration.
	 */
	int positioner_offset_;

	/** The current length of the positioner. */
	int positioner_length_;

	/***** ***** ***** ***** Pure virtual functions ***** ***** ***** *****/

	/** Get the length of the slider. */
	virtual unsigned get_length() const = 0;

	int available_length() const { return get_length() - offset_before() - offset_after(); }

	/**
	 * The number of pixels we can't use since they're used for borders.
	 *
	 * These are the pixels before the widget (left side if horizontal,
	 * top side if vertical).
	 */
	virtual unsigned offset_before() const = 0;

	/**
	 * The number of pixels we can't use since they're used for borders.
	 *
	 * These are the pixels after the widget (right side if horizontal,
	 * bottom side if vertical).
	 */
	virtual unsigned offset_after() const = 0;

	/**
	 * Is the coordinate on the positioner?
	 *
	 * @param coordinate          Coordinate to test whether it's on the
	 *                            positioner.
	 *
	 * @returns                   Whether the location on the positioner is.
	 */
	virtual bool on_positioner(const point& coordinate) const = 0;

	/**
	 * Is the coordinate on the bar?
	 *
	 * @param coordinate          Coordinate to test whether it's on the
	 *                            bar.
	 *
	 * @returns                   Whether the location on the bar is.
	 * @retval -1                 Coordinate is on the bar before positioner.
	 * @retval 0                  Coordinate is not on the bar.
	 * @retval 1                  Coordinate is on the bar after the positioner.
	 */
	virtual int on_bar(const point& coordinate) const = 0;

	/**
	 * Is the coordinate in the bar's orthogonal range?
	 *
	 * @param coordinate          Coordinate to test whether it's in-range.
	 *
	 * @returns                   Whether the location is in the bar's.
	 *                            orthogonal range.
	 */
	virtual bool in_orthogonal_range(const point& coordinate) const = 0;

	/**
	 * Gets the relevant difference in between the two positions.
	 *
	 * This function is used to determine how much the positioner needs to  be
	 * moved.
	 */
	virtual int get_length_difference(const point& original,
									  const point& current) const = 0;

	/***** ***** ***** ***** Private functions ***** ***** ***** *****/

	/**
	 * Updates the slider.
	 *
	 * Needs to be called when someting changes eg number of items
	 * or available size. It can only be called once we have a size
	 * otherwise we can't calulate a thing.
	 */
	void recalculate();

	/**
	 * Updates the positioner.
	 *
	 * This is a helper for recalculate().
	 */
	virtual int positioner_length() const = 0;

	void recalculate_positioner() {	positioner_length_ = positioner_length(); }

	/**
	 * Moves the positioner.
	 *
	 * @param distance           The distance moved, negative to begin, positive
	 *                           to end.
	*/
	void move_positioner(const int distance);

	/** Inherited from styled_widget. */
	void load_config_extra() override;

	/***** ***** ***** signal handlers ***** ****** *****/

	void signal_handler_mouse_enter(const event::ui_event event,
									bool& handled,
									bool& halt);

	void signal_handler_mouse_motion(const event::ui_event event,
									 bool& handled,
									 bool& halt,
									 const point& coordinate);

	void signal_handler_mouse_leave(const event::ui_event event, bool& handled);

	void signal_handler_left_button_down(const event::ui_event event,
										 bool& handled);

	void signal_handler_left_button_up(const event::ui_event event,
									   bool& handled);
};

} // namespace gui2
