#ifndef   CANARY_LOG_READER_H
#define   CANARY_LOG_READER_H

#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

/******************************************************************************
 * CANARY
 *****************************************************************************/
enum canary_log_reader_event {
	CANARY_LOG_READER_EVENT_NONE,
	CANARY_LOG_READER_EVENT_FRAME_READY,
	CANARY_LOG_READER_EVENT_ERROR
};

enum canary_log_reader_eflags {
	/* Size of token exceeds expected size */
	CANARY_LOG_READER_EFLAG_OVERFLOW   = 1,

	/* Size of token is lower than expected size */
	CANARY_LOG_READER_EFLAG_INCOMPLETE = 2,

	/* Unexpected newline */
	CANARY_LOG_READER_EFLAG_UNEXP_NEWL = 4
};

enum canary_log_reader_state {
	CANARY_LOG_READER_STATE_SKIP_LINE,
	CANARY_LOG_READER_STATE_PARSE_TIMESTAMP,
	CANARY_LOG_READER_STATE_PARSE_BUS_NUM,
	CANARY_LOG_READER_STATE_PARSE_ID,
	CANARY_LOG_READER_STATE_PARSE_FLAGS,
	CANARY_LOG_READER_STATE_PARSE_LEN,
	CANARY_LOG_READER_STATE_PARSE_DATA
};

struct canary_log_reader_frame {
	uint32_t timestamp_us;

	uint32_t id;
	uint8_t  len;
	uint8_t  data[8u];

	uint8_t  flags;
};

struct canary_log_reader {
	uint8_t _state;
	
	uint8_t _eflags;
	uint8_t _estate; /* Last error state. */

	uint8_t _i; /* i iterator */

	char    _tok[32u];
	uint8_t _len;

	size_t _total_frames;

	struct canary_log_reader_frame _frame;

	bool common_log;
};

void canary_log_reader_init(struct canary_log_reader *self)
{
	self->_state  = CANARY_LOG_READER_STATE_PARSE_TIMESTAMP;

	self->_eflags = 0u;
	self->_estate = 0u;

	self->_i = 0u;

	self->_tok[0u] = '\0';
	self->_len     =   0u;

	self->_total_frames = 0u;

	/* self->frame ... */
	self->_frame.timestamp_us = 0u;

	self->common_log = false;
}

/* TRY store input character into internal buffer,
 * Set eflags to OVERFLOW if max character count is exceed. */
void _canary_log_reader_consume_char(struct canary_log_reader *self,
				     const char c, uint8_t max_len)
{
	if (self->_len >= max_len) {
		self->_eflags |= CANARY_LOG_READER_EFLAG_OVERFLOW;
	} else {
		self->_tok[self->_len] = c;
		self->_len++;
	}
}

/* TRY parse number with specified `base` and terminate token, 
 * Set eflags to INCOMPLETE if less characters than expected. */
uint32_t _canary_log_reader_parse_num(struct canary_log_reader *self,
				      uint8_t base, uint8_t min_len)
{
	uint32_t result = 0u;

	/* Terminate token */
	self->_tok[self->_len] = '\0';

	if (self->_len < min_len) {
		self->_eflags |= CANARY_LOG_READER_EFLAG_INCOMPLETE;
	} else {
		errno = 0;
		result = strtol(self->_tok, NULL, base);
		if (errno != 0) {}; /* We don't care*/
	}

	self->_len = 0u;

	return result;
}

void _canary_log_reader_parse_timestamp_us(
				  struct canary_log_reader *self, const char c)
{
	/* Skip dots */
	if (c == '.') {} else if (c == '\n') {
		self->_eflags |= CANARY_LOG_READER_EFLAG_UNEXP_NEWL;
	} else if (isspace(c) > 0) {
		if (self->common_log) {
			self->_state = CANARY_LOG_READER_STATE_PARSE_BUS_NUM;
		} else {
			self->_state = CANARY_LOG_READER_STATE_PARSE_ID;
		}

		self->_frame.timestamp_us =
			_canary_log_reader_parse_num(self, 10u, 10u);
	} else {
		_canary_log_reader_consume_char(self, c, 10u);
	}
}

void _canary_log_reader_parse_bus_num(
				  struct canary_log_reader *self, const char c)
{
	if (c == '\n') {
		self->_eflags |= CANARY_LOG_READER_EFLAG_UNEXP_NEWL;
	} else if (isspace(c) > 0) {
		self->_state = CANARY_LOG_READER_STATE_PARSE_ID;

		/* TODO */
		(void)_canary_log_reader_parse_num(self, 10u, 1u);
	} else {
		_canary_log_reader_consume_char(self, c, 1u);
	}
}

void _canary_log_reader_parse_id(struct canary_log_reader *self, const char c)
{
	if (c == '\n') {
		self->_eflags |= CANARY_LOG_READER_EFLAG_UNEXP_NEWL;
	} else if (isspace(c) > 0) {
		self->_state = CANARY_LOG_READER_STATE_PARSE_FLAGS;

		self->_frame.id = _canary_log_reader_parse_num(self, 16u, 8u);
	} else {
		_canary_log_reader_consume_char(self, c, 8u);
	}
}

void _canary_log_reader_parse_flags(
				  struct canary_log_reader *self, const char c)
{
	if (c == '\n') {
		self->_eflags |= CANARY_LOG_READER_EFLAG_UNEXP_NEWL;
	} else if (isspace(c) > 0) {
		self->_state = CANARY_LOG_READER_STATE_PARSE_LEN;

		self->_frame.flags =
			_canary_log_reader_parse_num(self, 16u, 2u);
	} else {
		_canary_log_reader_consume_char(self, c, 2u);
	}
}

void _canary_log_reader_parse_len(struct canary_log_reader *self, const char c)
{
	if (c == '\n') {
		self->_eflags |= CANARY_LOG_READER_EFLAG_UNEXP_NEWL;
	} else if (isspace(c) > 0) {
		self->_state = CANARY_LOG_READER_STATE_PARSE_DATA;

		self->_frame.len = _canary_log_reader_parse_num(self, 10u, 1u);

		if (self->_frame.len > 8u) {
			self->_eflags |=
				CANARY_LOG_READER_EFLAG_OVERFLOW;
		}

		/* Set data element index to 0 */
		self->_i = 0u;
	} else {
		_canary_log_reader_consume_char(self, c, 1u);
	}
}

enum canary_log_reader_event _canary_log_reader_parse_data(
				  struct canary_log_reader *self, const char c)
{
	enum canary_log_reader_event ev = CANARY_LOG_READER_EVENT_NONE;

	if (isspace(c) > 0) {
		self->_frame.data[self->_i] =
			_canary_log_reader_parse_num(self, 16u, 2u);

		self->_i++;

		if (self->_i >= self->_frame.len) {
			self->_total_frames++;
			ev = CANARY_LOG_READER_EVENT_FRAME_READY;
			self->_state = CANARY_LOG_READER_STATE_PARSE_TIMESTAMP;
		}
	} else {
		_canary_log_reader_consume_char(self, c, 2u);
	}

	return ev;
}

enum canary_log_reader_event canary_log_reader_putc(
				  struct canary_log_reader *self, const char c)
{
	enum canary_log_reader_event ev = CANARY_LOG_READER_EVENT_NONE;

	/* Save state in case of error */
	self->_estate = self->_state;
	self->_eflags = 0u;
	
	/* Skip comments */
	if (c == ';') {
		self->_state = CANARY_LOG_READER_STATE_SKIP_LINE;
	}

	switch (self->_state) {
	case CANARY_LOG_READER_STATE_SKIP_LINE:
		if (c == '\n') {
			self->_len  = 0u;
			self->_state = CANARY_LOG_READER_STATE_PARSE_TIMESTAMP;
		}

		break;

	case CANARY_LOG_READER_STATE_PARSE_TIMESTAMP:
		_canary_log_reader_parse_timestamp_us(self, c);
		break;

	case CANARY_LOG_READER_STATE_PARSE_BUS_NUM:
		_canary_log_reader_parse_bus_num(self, c);
		break;

	case CANARY_LOG_READER_STATE_PARSE_ID:
		_canary_log_reader_parse_id(self, c);
		break;
		
	case CANARY_LOG_READER_STATE_PARSE_FLAGS:
		_canary_log_reader_parse_flags(self, c);
		break;
			
	case CANARY_LOG_READER_STATE_PARSE_LEN:
		_canary_log_reader_parse_len(self, c);
		break;

	case CANARY_LOG_READER_STATE_PARSE_DATA:
		ev = _canary_log_reader_parse_data(self, c);
		break;

	default:
		assert(0);
		break;
	}

	if (self->_eflags > 0u) {
		if ((self->_eflags &
		     (uint8_t)CANARY_LOG_READER_EFLAG_UNEXP_NEWL) > 0u) {
			self->_state = CANARY_LOG_READER_STATE_PARSE_TIMESTAMP;
		} else {
			self->_state = CANARY_LOG_READER_STATE_SKIP_LINE;
		}

		ev = CANARY_LOG_READER_EVENT_ERROR;
	}

	return ev;
}

#endif /* CAN_LOG_READER_H */
