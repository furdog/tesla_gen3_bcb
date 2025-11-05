/**
 * ```LICENSE
 * Tesla GEN3 Single phase module controller
 *
 * Copyright (C) 2025 furdog
 * https://github.com/furdog/tg3spmc
 *
 * Knowledge derived from:
 * Copyright (C) 2017-2019 T de Bree, D. Maguire, and C. Kidder
 * https://github.com/damienmaguire/Tesla-charger

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * ```
 */

#include "tg3spmc.h"
#include "tg3spmc.logger.h"

struct tg3spmc_frame test_frames[] = {
	{0x207, 8, {0x00, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x04, 0x00}},
	{0x217, 8, {0x00, 0x00, 0x01, 0xFC, 0x9C, 0x02, 0x00, 0x00}},
	{0x227, 8, {0x00, 0x00, 0x1C, 0x7F, 0x03, 0x00, 0x1F, 0xC5}},
	{0x237, 8, {0x3C, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
	{0x247, 8, {0x44, 0x7D, 0x08, 0x02, 0x00, 0x00, 0x20, 0x00}},

	{0x207, 8, {0x00, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x04, 0x00}},
	{0x217, 8, {0x02, 0x00, 0x01, 0xFC, 0x9C, 0x02, 0x00, 0x04}},
	{0x227, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xC5}},
	{0x237, 8, {0x3C, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
	{0x247, 8, {0x44, 0x7D, 0x6E, 0x03, 0x00, 0x00, 0x20, 0x00}}
};

void tg3spmc_test_config_invalid(struct tg3spmc *self)
{
	/* Shoud say that the config is invalid before proper config */
	assert(tg3spmc_get_pwron_pin_state(self) == false);
	assert(tg3spmc_get_chgen_pin_state(self) == false);
	assert(tg3spmc_step(self, 0) == TG3SPMC_EVENT_CONFIG_INVALID);
}

void tg3spmc_test_config_valid(struct tg3spmc *self)
{
	/* Shoud boot after correct config was given,
	 * only pwron pin must be enabled */
	assert(tg3spmc_step(self, 0) == TG3SPMC_EVENT_POWER_ON);
	assert(tg3spmc_get_pwron_pin_state(self) == true);
	assert(tg3spmc_get_chgen_pin_state(self) == false);
}

void tg3spmc_test_boot(struct tg3spmc *self)
{
	/* Should do nothing during CAN_TX_PERIOD */
	assert(tg3spmc_step(self, TG3SPMC_CONST_BOOT_TIME_MS - 1u) ==
		TG3SPMC_EVENT_NONE);
	/* Must enable all control pins after that */
	assert(tg3spmc_step(self, 1) == TG3SPMC_EVENT_CHARGE_ENABLED);
	assert(tg3spmc_get_pwron_pin_state(self) == true);
	assert(tg3spmc_get_chgen_pin_state(self) == true);
}

void tg3spmc_test_tx_no_broadcast(struct tg3spmc *self)
{
	struct tg3spmc_frame f;

	assert(tg3spmc_get_tx_frame(self, &f) == false);
	assert(tg3spmc_step(self, 0) == TG3SPMC_EVENT_NONE);
	assert(tg3spmc_get_tx_frame(self, &f) == true);
	assert(tg3spmc_get_tx_frame(self, &f) == false);
}

/* TODO validate messages */
void tg3spmc_test_tx(struct tg3spmc *self)
{
	struct tg3spmc_frame f;

	assert(tg3spmc_get_tx_frame(self, &f) == false);
	assert(tg3spmc_step(self, 0) == TG3SPMC_EVENT_NONE);

	assert(tg3spmc_get_tx_frame(self, &f) == true);
	assert(tg3spmc_get_tx_frame(self, &f) == true);

	/* Broadcast 0x45C goes second (TODO validate ID)*/
	assert(self->_hold_start == true);
	assert(f.data[3] == 0x0E); /* Todo check for 0x2E */

	assert(tg3spmc_get_tx_frame(self, &f) == true);
	assert(tg3spmc_get_tx_frame(self, &f) == false);
}

void tg3spmc_test_read_vars(struct tg3spmc *self)
{
	struct tg3spmc_frame invalid = { 0x555u, 8u,
		{ 0xFFu, 0xFFu, 0xFFu, 0xFFu,
		  0xFFu, 0xFFu, 0xFFu, 0xFFu }
	};

	struct tg3spmc_vars v;

	/* Before RX */
	assert(tg3spmc_read_vars(self, &v) == false);

	/* After invalid RX */
	tg3spmc_put_rx_frame(self, &invalid);
	assert(tg3spmc_read_vars(self, &v) == false);

	/* After valid RX */
	tg3spmc_put_rx_frame(self, &test_frames[0]);
	assert(tg3spmc_read_vars(self, &v) == true);
}

/* Normal initial state test, should also pass after error recovery */
void tg3spmc_test_normal_init(struct tg3spmc *self)
{
	struct tg3spmc saved_state; /* Snapshot state */

	tg3spmc_test_config_valid(self);

	tg3spmc_test_boot(self);

	saved_state = *self; /* Save state */
	tg3spmc_set_broadcast(self, false);
	tg3spmc_test_tx_no_broadcast(self);

	*self = saved_state; /* Load saved state */
	tg3spmc_set_broadcast(self, true);
	tg3spmc_test_tx(self);

	tg3spmc_test_read_vars(self);
}

void tg3spmc_test_rx_timeout(struct tg3spmc *self)
{
	assert(tg3spmc_step(self, TG3SPMC_CONST_CAN_RX_TIMEOUT_MS - 1u) ==
	       TG3SPMC_EVENT_NONE);
	assert(tg3spmc_step(self, 1u) == TG3SPMC_EVENT_FAULT);
	assert(tg3spmc_step(self, TG3SPMC_CONST_FAULT_RECOVERY_TIME_MS) ==
		TG3SPMC_EVENT_RECOVERY);

	/* Must pass after error recovery */
	tg3spmc_test_normal_init(self);
}

void tg3spmc_test_mod_fault(struct tg3spmc *self)
{
	test_frames[0].data[2] = 0xFF; /* put fault artifically */

	tg3spmc_put_rx_frame(self, &test_frames[0]);

	assert(tg3spmc_step(self, 0u) == TG3SPMC_EVENT_FAULT);
	assert(tg3spmc_step(self, TG3SPMC_CONST_FAULT_RECOVERY_TIME_MS) ==
		TG3SPMC_EVENT_RECOVERY);

	test_frames[0].data[2] = 0x00; /* undo fault artifically */

	/* Must pass after error recovery */
	tg3spmc_test_normal_init(self);

	assert(tg3spmc_step(self, 0u) == TG3SPMC_EVENT_NONE);
}

/* TODO test message periods */

int main()
{
	char buf[1024];
	struct tg3spmc mod;
	struct tg3spmc_config config;

	config.rated_voltage_ac_V = 240.0f;
	config.voltage_dc_V       = 380;
	config.current_ac_A       = 0.0f;

	/* Init module 0 */
	tg3spmc_init(&mod, 0u);
	tg3spmc_test_config_invalid(&mod);
	tg3spmc_set_config(&mod, config);
	tg3spmc_test_normal_init(&mod);

	tg3spmc_test_rx_timeout(&mod);
	tg3spmc_test_mod_fault(&mod);

	tg3spmc_log(&mod, buf, 1024);
	printf("%s\n\n", buf);

	tg3spmc_put_rx_frame(&mod, &test_frames[0]);
	tg3spmc_put_rx_frame(&mod, &test_frames[1]);
	tg3spmc_put_rx_frame(&mod, &test_frames[2]);
	tg3spmc_put_rx_frame(&mod, &test_frames[3]);
	tg3spmc_put_rx_frame(&mod, &test_frames[4]);
	tg3spmc_log(&mod, buf, 1024);
	printf("%s\n\n", buf);

	tg3spmc_put_rx_frame(&mod, &test_frames[5]);
	tg3spmc_put_rx_frame(&mod, &test_frames[6]);
	tg3spmc_put_rx_frame(&mod, &test_frames[7]);
	tg3spmc_put_rx_frame(&mod, &test_frames[8]);
	tg3spmc_put_rx_frame(&mod, &test_frames[9]);
	tg3spmc_log(&mod, buf, 1024);
	printf("%s\n", buf);

	return 0;
}
