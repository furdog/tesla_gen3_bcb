#include <stdint.h>
#include <stdio.h>

/*
 * @brief Logs the contents of the tg3spmc structure into a buffer with
 * 	visual alignment.
 *
 * This function is designed to be MISRA-C compliant by using snprintf
 * 	for safe buffer
 * handling and explicit casts for variadic arguments.
 *
 * @param self Pointer to the tg3spmc structure containing module data.
 * @param buf Pointer to the destination buffer (must be writable).
 * @param len The size of the destination buffer.
 * @return bool True if logging was successful,
 * 	false if buffer is insufficient or input is NULL.
 */
bool tg3spmc_log(struct tg3spmc *self, char *buf, size_t len)
{
	struct _tg3spmc_io       *i = &self->_io;
	struct  tg3spmc_vars     *v = &self->_vars;

	bool result = true;
	int32_t required_len;

	/* --- 1. Preparation: Convert boolean types to aligned
	 * 	strings/characters --- */
	const char *pwron_str = (true == i->pwron_out) ? "ON " : "OFF";
	const char *chgen_str = (true == i->chgen_out) ? "EN " : "DIS";

	const char ac_pres_char = (true == v->ac_present) ? 'Y' : 'N';
	const char en_pres_char = (true == v->en_present) ? 'Y' : 'N';
	const char fault_char   = (true == v->fault) ? 'Y' : 'N';

	/* --- 2. Formatting: Use snprintf for safe, aligned string generation 
	 * Fixed widths and alignment are used for readability:
	 * %-3s: Left-justified string, width 3 (e.g., "ON ", "DIS")
	 * %5.1f: Float, total width 5, 1 decimal point (e.g., " 350.5")
	 * %4u: Unsigned integer, width 4 (e.g., " 230")
	 * %+5d: Signed integer, width 5, forced sign (e.g., "+35")
	 * 0x%02X: Hexadecimal, minimum 2 digits with leading zero
	 * 	(e.g., "0x0A")
	 */

	required_len = snprintf(
		buf,
		len,
		/* Line 1: Basic Module Info & Controls */
		"|ID:%u       |Pwr:%-3s  |Chg:%-3s    |State:0x%02X |\n"
		/* Line 2: Voltage/Current DC & AC */
		"|V-DC:%5.1fV|V-AC:%3uV|I-DC:%5.1fA|I-AC:%4uA |\n"
		/* Line 3: Temperature Sensors and Limits */
		"|T1:%+5dC  |T2:%+5dC|Tgt:%+5dC |Lim:%5.1fA |\n"
		/* Line 4: Flags and Status (No newline on last line) */
		"|AC:%c       |EN:%c     |FLT:%c      |Status:0x%02X|",
		/* --- Arguments for snprintf (with explicit MISRA-C casts) */
		/* Line 1 */
		(unsigned int)self->_id,
		pwron_str,
		chgen_str,
		(unsigned int)self->_state,

		/* Line 2 */
		(double)v->voltage_dc_V,
		(unsigned int)v->voltage_ac_V,
		(double)v->current_dc_A,
		(unsigned int)v->current_ac_A,

		/* Line 3 */
		(int)v->temp1_C,
		(int)v->temp2_C,
		(int)v->inlet_target_temp_C,
		(double)v->current_limit_due_temp_A,

		/* Line 4 */
		ac_pres_char,
		en_pres_char,
		fault_char,
		(unsigned int)v->status
	);

	/* --- 3. Error Check: Verify buffer size sufficiency --- */

	if (required_len < 0)
	{
		/* Encoding error during snprintf (highly unlikely) */
		result = false;
	} else {
		/* Check if the required length
		 * (plus 1 for the null terminator) exceeds the buffer size. */
		/* MISRA-C Rule 10.3: Explicit cast of int32_t to size_t
		 *  for comparison. */
		if (((size_t)required_len + 1U) > len) {
			result = false; /* Buffer insufficient */
		}
	}

	return result;
}
