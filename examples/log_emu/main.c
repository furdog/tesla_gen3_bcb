#include "canary_log_reader.h"
#include "delta_time.h"
#include "tg3spmc.h"
#include "tg3spmc.logger.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>

#define SYS_TIMESTAMP_US (clock() / (CLOCKS_PER_SEC / 1000000.0f))
#define SYS_TIMESTAMP_MS (clock() / (CLOCKS_PER_SEC / 1000.0f))

void canary_print_header()
{
	printf(";CANARY V2.3\n;TIME_us.d  ID       FL L DATA\n");
}

void canary_print_frame(struct canary_log_reader *self)
{
	int i;
			
	printf("%011u %08X %02X %i",
	       self->_frame.timestamp_us, self->_frame.id,
	       self->_frame.flags, self->_frame.len);
	       
	for (i = 0; i < self->_frame.len; i++)
		printf(" %02X", self->_frame.data[i]);
	printf("\n");
}

/*****************************************************************************/
struct tg3spmc mod1;
struct tg3spmc_config config;

/* For timing purposes */
struct delta_time dt;

/* Buffer for log */
char log_buf[1024];

void setup()
{
	delta_time_init(&dt);

	/* Tesla module config */
	config.rated_voltage_ac_V = 240.0f;
	config.voltage_dc_V       = 390;
	config.current_ac_A       = 4.0f;

	/* Init module 1 (0, *1, 2) */
	tg3spmc_init(&mod1, 1u);
	tg3spmc_set_config(&mod1, config);
}

void loop()
{
	/* Log timer */
	static uint32_t log_timer_ms = 0;

	/* Module event */
	enum tg3spmc_event ev;

	/* General purpose CAN frame */
	struct tg3spmc_frame f;

	/* Measure delta time from past loop cycle (milliseconds) */
	uint32_t delta_time_ms = delta_time_update_ms(&dt, SYS_TIMESTAMP_MS);

	ev = tg3spmc_step(&mod1, delta_time_ms);

	if (tg3spmc_get_tx_frame(&mod1, &f)) {
		/*simple_twai_send(&stw1, &f);*/
	}

	/*if (simple_twai_recv(&stw1, &f)) {
		tg3spmc_put_rx_frame(&mod1, &f);
	}*/

	/*digitalWrite(MOD1_PWRON_PIN, tg3spmc_get_pwron_pin_state(&mod1));
	digitalWrite(MOD1_CHGEN_PIN, tg3spmc_get_chgen_pin_state(&mod1));*/

	/* Log some stuff every 1000ms, + log events */
	if (ev != 0) {
		printf("TG3SPMC_EVENT_%s", tg3spmc_get_event_name(ev));
	}

	if (ev == TG3SPMC_EVENT_FAULT) {
		printf(", CAUSE: %u", mod1.fault_cause);
	}

	if (ev != 0) {
		printf("\n");
	}

	log_timer_ms += delta_time_ms;
	if (log_timer_ms >= 500u) {
		log_timer_ms -= 500u;

		/* TODO: read of private member MUST be avoided
		 * possible solutions:
		 * 	check value of tg3spmc_read_vars or update API */
		if (mod1._io.rx.has_frames) {
			tg3spmc_log(&mod1, log_buf, 1024);
			printf("%s\n\n", log_buf);
		}
	}
}

int main()
{
	int c;
	struct canary_log_reader c_inst;
	uint32_t last_msg_time_us = 0u;

	FILE *file = fopen("../../research/common_20251029_154131_tesla_bcb"
			   "_start_and_230_ac_387_DC_working_4A"
			   "_but_unstable_as_hell.txt", "r");

	assert(file);
	
	canary_log_reader_init(&c_inst);
	c_inst.common_log = true;

	canary_print_header();

	/* Setup tesla module */
	setup();
	
	c = getc(file);
	while (!feof(file)) {
		/* Loop tesla module */
		loop();

		if (SYS_TIMESTAMP_US <= last_msg_time_us) {
			continue;
		}

		enum canary_log_reader_event ev =
					    canary_log_reader_putc(&c_inst, c);
		if (ev ==
		    CANARY_LOG_READER_EVENT_FRAME_READY) {
			struct tg3spmc_frame f;

			/* canary_print_frame(&c_inst); */

			f.id = c_inst._frame.id;
			f.len = c_inst._frame.len;
			memcpy(f.data, c_inst._frame.data, f.len);

			tg3spmc_put_rx_frame(&mod1, &f);

			last_msg_time_us = c_inst._frame.timestamp_us;
		}

		if (ev == CANARY_LOG_READER_EVENT_ERROR) {
			printf("err, state: %i, flags: %i\n",
				c_inst._estate, c_inst._eflags); fflush(0);
		}

		c = getc(file);
	}

	printf("FINISHED, TOTAL_FRAMES: %u\n", (unsigned)c_inst._total_frames);

	return 0;
}
