#pragma once

#include <stdint.h>

/******************************************************************************
 * CLASS
 *****************************************************************************/
struct delta_time {
	uint32_t _timestamp_prev;
};

/******************************************************************************
 * PUBLIC
 *****************************************************************************/
void delta_time_init(struct delta_time *self)
{
	self->_timestamp_prev = 0;
}

uint32_t delta_time_update_ms(struct delta_time *self, uint32_t timestamp)
{
	uint32_t delta_time_ms = 0;

	delta_time_ms         = timestamp - self->_timestamp_prev;
	self->_timestamp_prev = timestamp;

	return delta_time_ms;
}
