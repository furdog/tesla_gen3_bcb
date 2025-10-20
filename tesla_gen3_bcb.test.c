#include "tg3spmc.h"
#include "tg3spmc.logger.h"

/* Random message from logs: */
/*
00000207,false,Rx,1,8,00,00,00,00,C8,00,04,00,
00000217,false,Rx,1,8,00,00,01,FC,9C,02,00,00,
00000227,false,Rx,1,8,00,00,1C,7F,03,00,1F,C5,
00000237,false,Rx,1,8,3C,41,00,00,00,00,00,00,
00000247,false,Rx,1,8,44,7D,08,02,00,00,20,00,

69992088,00000217,false,Rx,1,8,02,00,01,FC,9C,02,00,04,
70082412,00000207,false,Rx,1,8,00,00,00,00,C8,00,04,00,
70082660,00000227,false,Rx,1,8,00,00,00,00,00,00,1F,C5,
70082907,00000237,false,Rx,1,8,3C,41,00,00,00,00,00,00,
70083140,00000247,false,Rx,1,8,44,7D,6E,03,00,00,20,00,
*/

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

char buf[1024];
struct tg3spmc mod;

int main()
{
	struct tg3spmc_config config;

	config.rated_voltage_ac_V = 240.0f;
	config.voltage_dc_V       = 380;
	config.current_ac_A       = 0.0f;

	/* Init module 0 */
	tg3spmc_init(&mod, 0u);

	/* Shoud say that the config is invalid before proper config */
	assert(tg3spmc_get_pwron_pin_state(&mod) == false);
	assert(tg3spmc_get_chgen_pin_state(&mod) == false);
	assert(tg3spmc_step(&mod, 0) == TG3SPMC_EVENT_CONFIG_INVALID);

	/* Shoud boot after correct config was given,
	 * only pwron pin must be enabled */
	tg3spmc_set_config(&mod, config);
	assert(tg3spmc_step(&mod, 0) == TG3SPMC_EVENT_POWER_ON);
	assert(tg3spmc_get_pwron_pin_state(&mod) == true);
	assert(tg3spmc_get_chgen_pin_state(&mod) == false);

	/* Should do nothing during CAN_TX_PERIOD */
	assert(tg3spmc_step(&mod, TG3SPMC_CAN_TX_PERIOD_MS - 1u) ==
		TG3SPMC_EVENT_NONE);
	/* Must enable all control pins after that */
	assert(tg3spmc_step(&mod, 1) == TG3SPMC_EVENT_CHARGE_ENABLED);
	assert(tg3spmc_get_pwron_pin_state(&mod) == true);
	assert(tg3spmc_get_chgen_pin_state(&mod) == true);

	/* Tx must be ready after this */
	assert(mod._io.tx.count == 0u);
	assert(tg3spmc_step(&mod, 0) == TG3SPMC_EVENT_NONE);
	assert(mod._io.tx.count == 3u);

	tg3spmc_log(&mod, buf, 1024);
	printf("%s\n\n", buf);

	_tg3spmc_decode_frame(&mod, &test_frames[0]);
	_tg3spmc_decode_frame(&mod, &test_frames[1]);
	_tg3spmc_decode_frame(&mod, &test_frames[2]);
	_tg3spmc_decode_frame(&mod, &test_frames[3]);
	_tg3spmc_decode_frame(&mod, &test_frames[4]);
	tg3spmc_log(&mod, buf, 1024);
	printf("%s\n\n", buf);

	_tg3spmc_decode_frame(&mod, &test_frames[5]);
	_tg3spmc_decode_frame(&mod, &test_frames[6]);
	_tg3spmc_decode_frame(&mod, &test_frames[7]);
	_tg3spmc_decode_frame(&mod, &test_frames[8]);
	_tg3spmc_decode_frame(&mod, &test_frames[9]);
	tg3spmc_log(&mod, buf, 1024);
	printf("%s\n", buf);

	return 0;
}
