#include "src/tg3spmc.h"
#include "src/tg3spmc.logger.h"
#include "delta_time.h"

/* Replace simple TWAI frame with our tg3spmc frame.
 * Frame structure must be identical. */
#define SIMPLE_TWAI_FRAME_LABEL tg3spmc_frame

/******************************************************************************
 * SIMPLE TWAI ADAPTER FOR VARIOUS CAN RELATED PROJECTS (ESP32C6)
 * Preconfigured, default TWAI 500kbps, no filtering.
 *****************************************************************************/
#include "driver/gpio.h"
#include "driver/twai.h"

struct simple_twai
{
	twai_handle_t bus;
	uint8_t id;

	gpio_num_t tx;
	gpio_num_t rx;
};

void simple_twai_init(struct simple_twai *self)
{
	esp_err_t code;

	twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
		self->tx, self->rx, TWAI_MODE_NORMAL);
	twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS  ();
	twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
	
	g_config.controller_id = self->id;
	code = twai_driver_install_v2(&g_config, &t_config, &f_config,
		&self->bus);

	if (code == ESP_OK) {
		code = twai_start_v2(self->bus);
	}

	if (code != ESP_OK) {
		printf("Twai error: (%s)\n", esp_err_to_name(code));
	}

	twai_reconfigure_alerts_v2(self->bus, TWAI_ALERT_BUS_OFF, NULL);
}

void simple_twai_kill(struct simple_twai *self)
{
	twai_driver_uninstall_v2(self->bus);
}

void simple_twai_send(struct simple_twai *self,
		      struct SIMPLE_TWAI_FRAME_LABEL *frame)
{
	int8_t i;

	twai_message_t msg;
	msg.extd = 0;
	msg.rtr  = 0;
	msg.ss   = 0; /* Do not retransmit on error (single-shot) */
	msg.self = 0; /* loopback disabled */
	msg.dlc_non_comp = 0; /* Normal 8 byte dlc */

	msg.identifier       = frame->id;
	msg.data_length_code = frame->len;

	/* Only send frames less or equal 8 */
	if (frame->len <= 8) {
		for (i = 0; i < frame->len; i++) {
			msg.data[i] = frame->data[i];
		}

		/* Queue */
		twai_transmit_v2(self->bus, &msg, 0);
	}
}

bool simple_twai_recv(struct simple_twai *self,
		      struct SIMPLE_TWAI_FRAME_LABEL *frame)
{
	bool has_message = false;

	twai_message_t msg;

	if (twai_receive_v2(self->bus, &msg, 0) == ESP_OK &&
	    msg.data_length_code <= 8 && !msg.rtr) {
		int8_t i = 0;

		frame->id = msg.identifier;
		frame->len = msg.data_length_code;

		for (i = 0; i < msg.data_length_code; i++) {
			frame->data[i] = msg.data[i];
		}

		has_message = true;
	}

	return has_message;
}

void simple_twai_update(struct simple_twai *self)
{
	uint32_t alerts = 0;

	twai_read_alerts_v2(self->bus, &alerts, 0);

	if (alerts & TWAI_ALERT_BUS_OFF) {
		printf("TWAI_ALERT_BUS_OFF\n");

		/* Reset TWAI in case of bus off */
		twai_reconfigure_alerts_v2(self->bus, 0, NULL);
		simple_twai_kill(self);
		simple_twai_init(self);
	}
}

/*****************************************************************************/
struct simple_twai stw0;
struct simple_twai stw1;

/* Single Tesla one phase module */
#define MOD1_PWRON_PIN 18u
#define MOD1_CHGEN_PIN 19u

struct tg3spmc mod1;
struct tg3spmc_config config;

/* For timing purposes */
struct delta_time dt;

/* Buffer for log */
char log_buf[1024];

void setup()
{
	delta_time_init(&dt);

	/* Peripherals */
	pinMode(MOD1_PWRON_PIN, OUTPUT);
	pinMode(MOD1_CHGEN_PIN, OUTPUT);
	Serial.begin(921600);

	/* ESP32C6 has two TWAI(CAN2.0) controllers */
	stw0.id = 0;
	stw0.tx = GPIO_NUM_13; /* Conflicts with USB */
	stw0.rx = GPIO_NUM_12; /* Conflicts with USB */

	stw1.id = 1;
	stw1.tx = GPIO_NUM_14;
	stw1.rx = GPIO_NUM_15;

	/* simple_twai_init(&stw0); */
	simple_twai_init(&stw1);

	/* Tesla module config */
	config.rated_voltage_ac_V = 240.0f;
	config.voltage_dc_V       = 380;
	config.current_ac_A       = 0.0f;

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
	uint32_t delta_time_ms = delta_time_update_ms(&dt, millis());

	/* TWAI */
	/* simple_twai_update(&stw0); */
	simple_twai_update(&stw1);

	/* TESLA */
	ev = tg3spmc_step(&mod1, delta_time_ms);

	if (tg3spmc_get_tx_frame(&mod1, &f)) {
		simple_twai_send(&stw1, &f);
	}

	if (simple_twai_recv(&stw1, &f)) {
		tg3spmc_put_rx_frame(&mod1, &f);
	}

	digitalWrite(MOD1_PWRON_PIN, tg3spmc_get_pwron_pin_state(&mod1));
	digitalWrite(MOD1_CHGEN_PIN, tg3spmc_get_chgen_pin_state(&mod1));

	/* Log some stuff every 1000ms, + log events */
	if (ev != 0) {
		printf("TG3SPMC_EVENT_%s\n", tg3spmc_get_event_name(ev));
	}

	log_timer_ms += delta_time_ms;
	if (log_timer_ms >= 1000u) {
		log_timer_ms -= 1000u;

		/* TODO: read of private member MUST be avoided
		 * possible solutions:
		 * 	check value of tg3spmc_read_vars or update API */
		if (mod1._io.rx.has_frames) {
			tg3spmc_log(&mod1, log_buf, 1024);
			printf("%s\n\n", log_buf);
		}
	}
}
