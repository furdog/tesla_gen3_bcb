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
 * 
 * @file tg3spmc.h
 * @brief Tesla(t) GEN3(g3) Single(s) phase(p) module(m) controller(c): tg3spmc
 *
 * This file implements logic to control single phase module within Tesla GEN3
 * Battery Controller Board (BCB). The implementation is completely hardware
 * agnostic and requires external wraping layer to interract with a hardware.
 */
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

/** Period of CAN message transmission. (milliseconds) */
#define TG3SPMC_CONST_CAN_TX_PERIOD_MS 90u

/** How long should we wait before setting RX timeout? (milliseconds) */
#define TG3SPMC_CONST_CAN_RX_TIMEOUT_MS 1000u

/** Fault recovery time (milliseconds) */
#define TG3SPMC_CONST_FAULT_RECOVERY_TIME_MS 1000u

/** Boot time (milliseconds).
 * Longer boot values allows module to initialize correctly after fault.
 * (Proven experimentally) */
#define TG3SPMC_CONST_BOOT_TIME_MS 1000u

/** Minimum alloved DC voltage in volts */
#define TG3SPMC_CONST_MIN_DC_VOLTAGE_V 250.0f


/******************************************************************************
 * TG3SPMC GENERIC
 *****************************************************************************/
/**
 * @brief CAN 2.0 Data Frame structure.
 */
struct tg3spmc_frame {
	uint32_t id;	  /**< Frame identifier. */
	uint8_t  len;	  /**< Data length code (0-8). */
	uint8_t  data[8]; /**< Frame data payload. */
};

/******************************************************************************
 * TG3SPMC DEBUG
 *****************************************************************************/
/**
 * @brief Fault cause code after module gets into fault.
 */
enum tg3spmc_fault_cause {
	TG3SPMC_FAULT_CAUSE_NONE,       /**< No fault */
	TG3SPMC_FAULT_CAUSE_RX_TIMEOUT, /**< Fault caused by RX timeout */
	TG3SPMC_FAULT_CAUSE_FAULT_FLAG  /**< Fault caused by module flag */
};

/******************************************************************************
 * TG3SPM PRIVATE
 *****************************************************************************/
/**
 * @brief Single phase module base ID enum
 */
enum _tg3spm_frame_base_id {
	_TG3SPM_FRAME_BASE_ID_AC_PARAMS = 0x207u,
	_TG3SPM_FRAME_BASE_ID_DC_PARAMS = 0x217u,
	_TG3SPM_FRAME_BASE_ID_STATUS    = 0x227u,
	_TG3SPM_FRAME_BASE_ID_SENSORS   = 0x237u,
	_TG3SPM_FRAME_BASE_ID_LIMITS    = 0x247u
};

/** Enum for 6bit flags inside ac_params */
enum _tg3spm_field_ac_params_flags0 {
	_TG3SPM_FIELD_AC_PARAMS_FLAGS0_UNKNOWN1          = 1u,
	_TG3SPM_FIELD_AC_PARAMS_FLAGS0_SOFTSTART_ALLOWED = 2u,
	_TG3SPM_FIELD_AC_PARAMS_FLAGS0_FAULT             = 4u,
	_TG3SPM_FIELD_AC_PARAMS_FLAGS0_CUR_OUT           = 8u,
	_TG3SPM_FIELD_AC_PARAMS_FLAGS0_UNKNOWN5          = 16u,
	_TG3SPM_FIELD_AC_PARAMS_FLAGS0_UNKNOWN6          = 32u
};

/** Enum for 2bit flags inside ac_params */
enum _tg3spm_field_ac_params_flags1 {
	_TG3SPM_FIELD_AC_PARAMS_FLAGS1_CHARGE_DISALLOWED = 1u,
	_TG3SPM_FIELD_AC_PARAMS_FLAGS1_UNKNOWN8          = 2u
};

/** Enum for 8bit flags inside status */
enum _tg3spm_field_status_flags {
	_TG3SPM_FIELD_STATUS_FLAGS_CHGEN_PIN_ON    = 1u,
	_TG3SPM_FIELD_STATUS_FLAGS_UNKNOWN2        = 2u,
	_TG3SPM_FIELD_STATUS_FLAGS_UNKNOWN3        = 4u,
	_TG3SPM_FIELD_STATUS_FLAGS_UNKNOWN4        = 8u,
	_TG3SPM_FIELD_STATUS_FLAGS_UNKNOWN5        = 16u,
	_TG3SPM_FIELD_STATUS_FLAGS_AC_PRECHARGE_EN = 32u,
	_TG3SPM_FIELD_STATUS_FLAGS_DC_PRECHARGE_EN = 64u,
	_TG3SPM_FIELD_STATUS_FLAGS_UNKNOWN8        = 128u
};

/* TODO More defs and make use of it in code
 * TODO Decoder functions for tg3spm messages must go here.
 * Ensure every function provides raw and calculated values */

/******************************************************************************
 * TG3SPMC PRIVATE WRITER
 *****************************************************************************/
/**
 * @brief Structure for writing CAN frames to the single phase module.
 */
struct _tg3spmc_writer
{
	/** Array to hold frames to be sent (max 3 at a time). */
	struct tg3spmc_frame frames[3];

	/** The number of valid frames currently in the array. */
	uint8_t count;

	/**
	 * @brief Controls whether this instance sends broadcast
	 * messages (0x45C).
	 *
	 * Only one instance in a multi-module system should
	 * have this enabled.
	 */
	bool enable_broadcast;

	/** Timer used for frame transmission scheduling. */
	uint32_t timer_ms;
};

/**
 * @brief Initializes the CAN writer structure.
 * @param self Pointer to the tg3spmc_writer instance.
 */
void _tg3spmc_writer_init(struct _tg3spmc_writer *self)
{
	self->count = 0u;

	self->enable_broadcast = true;

	self->timer_ms = 0u;
}

/* TODO add more methods, more separation of concerns */

/******************************************************************************
 * TG3SPMC PRIVATE READER CLASS
 *****************************************************************************/
/**
 * @brief Structure for handling received CAN frames from the module.
 */
struct _tg3spmc_reader
{
	/** Timer used for monitoring frame reception timeouts. */
	uint32_t timer_ms;

	/** Received frames (bits flagged) */
	uint8_t recv_flags;

	/** Flag indicating if new frames have been received in the step. */
	bool has_frames;
};

/**
 * @brief Initializes the CAN reader structure.
 * @param self Pointer to the tg3spmc_reader instance.
 */
void _tg3spmc_reader_init(struct _tg3spmc_reader *self)
{
	/* TODO implement properly */
	self->timer_ms = 0u;

	self->recv_flags = 0u;

	self->has_frames = false;
}

/* TODO add more methods, more separation of concerns */

/******************************************************************************
 * TG3SPMC CLASS
 *****************************************************************************/
/**
 * @brief Events emitted by the module to indicate a change or occurrence.
 */
enum tg3spmc_event {
	TG3SPMC_EVENT_NONE,           /**< No event. */
	TG3SPMC_EVENT_CONFIG_INVALID, /**< Configuration validation failed. */
	TG3SPMC_EVENT_POWER_ON,       /**< Module is being powered. */
	TG3SPMC_EVENT_CHARGE_ENABLED, /**< Charging mode is enabled. */
	TG3SPMC_EVENT_FAULT,          /**< Something went horribly wrong. */
	TG3SPMC_EVENT_RECOVERY        /**< Recovery from error. */
};

/**
 * @brief Internal states of the module controller, driven by the library.
 */
enum _tg3spmc_state {
	_TG3SPMC_STATE_CONFIG,  /**< Awaiting valid configuration settings. */
	_TG3SPMC_STATE_BOOT,    /**< Powering and initializing the module. */
	_TG3SPMC_STATE_RUNNING, /**< Module is fully operational. */
	_TG3SPMC_STATE_FAULT    /**< Something went very wrong. */
};

/**
 * @brief Bit flags representing the module's status,
 * decoded from CAN messages.
 */
enum tg3spmc_status_flag {
	TG3SPMC_STATUS_FLAG_EN       = 1u,  /**< chgen_out feedback flag. */
	TG3SPMC_STATUS_FLAG_UNKNOWN2 = 2u,  /**< Unknown status bit 2. */
	TG3SPMC_STATUS_FLAG_UNKNOWN3 = 4u,  /**< Unknown status bit 3. */
	TG3SPMC_STATUS_FLAG_UNKNOWN4 = 8u,  /**< Unknown status bit 4. */

	/** Probably charger ready to start AC->DC.
	 * Goes into 0 if power output has started */
	TG3SPMC_STATUS_FLAG_UNKNOWN5 = 16u,

	/** Some internal flag (Toggles 3 times when no HVDC present).
	 * Final state is 1. Always 0 if no charge mode enabled? TODO survey */
	TG3SPMC_STATUS_FLAG_UNKNOWN6 = 32u,

	TG3SPMC_STATUS_FLAG_UNKNOWN7 = 64u, /**< Unknown status bit 7. */
	TG3SPMC_STATUS_FLAG_UNKNOWN8 = 128u /**< Unknown status bit 8. */
};

/**
 * @brief Input/Output interface to be communicated with the outside world.
 * This is a logical representation; the user must map it to real
 * hardware (pins, CAN buses, etc.) via the tg3spmc API.
 */
struct _tg3spmc_io { /* TODO rename to virtual_io or vio */
	/** Module power ON control output (3.3v logic). */
	bool pwron_out;

	/** Enable AC->HVDC charging control output (3.3v logic). */
	bool chgen_out;

	/** CAN transmission interface. */
	struct _tg3spmc_writer tx;

	/** CAN reception interface. */
	struct _tg3spmc_reader rx;
};

/**
 * @brief Configuration settings for the module controller.
 *
 * Must be set after initialization. Valid settings trigger transition
 * from CONFIG state.
 */
struct tg3spmc_config {
	/** Target DC output voltage for the charger (V). */
	float voltage_dc_V;

	/** Target AC input current for the charger (A). */
	float current_ac_A;

	/** Rated AC input voltage (e.g., 240VAC for EU/UK, 110VAC for US). */
	float rated_voltage_ac_V;
};

/**
 * @brief Read-only variables representing the module's current status,
 * measurements, and health.
 */
struct tg3spmc_vars {
	float   voltage_dc_V; /**< Measured DC output voltage (V). */
	uint8_t voltage_ac_V; /**< Measured AC input voltage (V). */
	float   current_dc_A; /**< Measured DC output current (A). */
	float   current_ac_A; /**< Measured AC input current (A). */

	/** Target inlet coolant temperature (C). */
	int16_t inlet_target_temp_C;

	/** Current limit imposed due to temperature (A). */
	float   current_limit_due_temp_A;

	/** Temperature sensor 1 reading (C). */
	int16_t temp1_C;
	/** Temperature sensor 2 reading (C). */
	int16_t temp2_C;

	/** Flag: true if AC voltage is present. */
	bool ac_present;
	/** Flag: true if module reports it's enabled. */
	bool en_present;
	/** Flag: true if the module reports a fault. */
	bool fault;

	/** Raw status byte containing various status flags. */
	uint8_t status;
};

/**
 * @brief Main structure for the single phase module logical representation.
 *
 * A single charger may contain up to three such modules.
 */
struct tg3spmc
{
	/** Module ID: 0, 1, or 2 are allowed. */
	uint8_t _id;

	/** Internal state of the module (enum tg3spmc_state). */
	uint8_t _state;

	/** General purpose timer for state transitions and timeouts (ms). */
	uint32_t _timer_ms;

	/** Stores fault cause in case of fault event */
	uint8_t fault_cause;

	/** Hold charger start when in RUNNING state.
	 *  Necessary to pass initial setup to the charger */
	bool _hold_start;

	/** Input/Output hardware interface structure. */
	struct _tg3spmc_io     _io;
	/** Configuration settings structure. */
	struct  tg3spmc_config _config;
	/** Read-only module variables and measurements. */
	struct  tg3spmc_vars   _vars;
};

/******************************************************************************
 * TG3SPMC PRIVATE METHODS
 *****************************************************************************/
/**
 * @brief Decodes a single CAN frame received from the module.
 *
 * It uses the module's ID to calculate the message base ID.
 * @param self Pointer to the tg3spmc instance.
 * @param f Pointer to the received CAN frame.
 */
void _tg3spmc_decode_frame(struct tg3spmc *self, struct tg3spmc_frame *f)
{
	/* TODO move this into _tg3spmc_reader section, do not decode.
	 * move decoding into _tg3spm section and put into separate methods.
	 * Provide decoding only when required explicitly (lazy decoding) */
	struct  tg3spmc_vars *v = &self->_vars;
	struct _tg3spmc_io   *i = &self->_io;

	/* Tells us if we received a valid frame after all */
	bool valid_frame = true;

	uint32_t base_id;

	/* Use current module ID to calculate base ID */
	base_id = f->id - (self->_id * 2u);

	/* Generic for all three modules. */
	switch (base_id) {
	case 0x207u: /* AC_vars */
		/* SG_ voltage_V : 8|8@1+ (1,0) [0|1] "" Vector__XXX */
		v->voltage_ac_V = f->data[1];
		v->ac_present = (v->voltage_ac_V > 70u)      ? true : false;

		/* SG_ peak_current_A : 41|9@1+ (0.1,0) [0|1] "" Vector__XXX */
		/* (peak_current_A * 10) */
		v->current_ac_A = 0.070710678118f * /* 0.1/sqrt(2) */
			((((f->data[6] & 0x0003u) << 8u) | f->data[5]) >> 1u);

   		/* TODO rename */
		/* SG_ precharge_en : 17|1@1+ (1,0) [0|1] "" Vector__XXX */
   		v->en_present = ((f->data[2] & 0x02u) != 0u) ? true : false;

		/* SG_ fault_flag : 18|1@1+ (1,0) [0|1] "" Vector__XXX */
		v->fault = ((f->data[2] & 0x04u) != 0u) ? true : false;

		i->rx.recv_flags |= (1u << 0u);
		break;
	case 0x217u: /* Status */
		/* Status Message: Raw status byte. */
		v->status = f->data[0];

		i->rx.recv_flags |= (1u << 1u);
		break;
	case 0x227u: /* DC_vars */
		/* I highly doubt that they transmit actual ADC data,
		 * But these scalars seems to be close to real measurements. */
		v->voltage_dc_V =
			((f->data[3] << 8u) | f->data[2]) * 700.0f/0xFFFF;
		/*mul = 0.01068131532768749523155565728237*/

		v->current_dc_A =
			((f->data[5] << 8u) | f->data[4]) * 50.0f/0xFFFF;
		/*mul = 0.000762951094834821087968261234455*/

		i->rx.recv_flags |= (1u << 2u);
		break;
	case 0x237u:
		/* Temp Msg 1: Temp sensor readings and target temp. */
		v->temp1_C = (int16_t)f->data[0] - 40;
		v->temp2_C = (int16_t)f->data[1] - 40;
		v->inlet_target_temp_C = (int16_t)f->data[5] - 40;

		i->rx.recv_flags |= (1u << 3u);
		break;
	case 0x247u:
		/* 15/64, close to 1/4 */
		/* Temp Msg 2: Current limit due to temperature. */
		v->current_limit_due_temp_A = f->data[0] * 0.234375;

		i->rx.recv_flags |= (1u << 4u);
		break;

	case 0x347u:
		/* 1000ms period
		 * byte[2] goes 0x80 shortly when there's
		 * some distruption in AC supply
		 * (lose or bad connection to AC socket (sparks)) */
		break;

	case 0x467u:
		/* 100ms period
		 * byte[0], byte[1] goes 7E 09 (24300 decimal in little endian)
		 * It slowly increases to that value after start, approx 5s */
		break;

	case 0x537u:
		/* 900ms period
		 * Probably fragmented CAN message
		 * byte[0] is an index of fragment
		 * Range: 0x0A - 0x14
		 * Observed sequence: 0A 0B 0D 0E 0F 10 11 12 13 14
		 * */
		break;

	case 0x717u:
		/* 100ms period
		 * Probably fragmented CAN message
		 * byte[0] is an index of fragment (range: 0x01 - 0x1C)
		 * Observed sequence: 01 02 04 05 06 07 08 09 0A 0B 0C 0E 0F
		 * 		      10 11 12 13 14 16 17 18 19 1A 1B 1C
		 */
		break;

	default:
		valid_frame = false;
		break;
	}

	if (valid_frame && (i->rx.recv_flags == (1u << 5u)) - 1u) {
		i->rx.has_frames = true;
		i->rx.timer_ms   = 0u;
	}
}

/**
 * @brief Encodes the 0x45C (Broadcast) CAN frame.
 * It contains the target DC voltage setting.
 *
 * @warning This message is non-local and should only be sent by one instance.
 * @param self Pointer to the tg3spmc instance.
 * @param f Pointer to the CAN frame to be populated.
 */
void _tg3spmc_encode_frame_h45C(struct tg3spmc *self,
				struct tg3spmc_frame *f)
{
	/* TODO return frame instead of writing by reference */
	struct tg3spmc_config *s = &self->_config;

	uint16_t raw_set_voltage_dc_V = s->voltage_dc_V * 100.0f;

	f->id  = 0x45C;
	f->len = 8;

	f->data[0] = (raw_set_voltage_dc_V & 0x00FFu) >> 0u;
	f->data[1] = (raw_set_voltage_dc_V & 0xFF00u) >> 8u;

	if (self->_state == (uint8_t)_TG3SPMC_STATE_RUNNING &&
	    !self->_hold_start) {
		f->data[3] = 0x2E; /* State-dependent control byte */
	} else {
		f->data[3] = 0x0E; /* State-dependent control byte */
	}

	/* Unknown, static data */
	f->data[2] = 0x14;
	f->data[4] = 0x00;
	f->data[5] = 0x00;
	f->data[6] = 0x90;
	f->data[7] = 0x8C;
}

/**
 * @brief Encodes the 0x42C (+ID) CAN frame for module-specific control.
 *
 * This message contains the target AC current setting.
 * @param self Pointer to the tg3spmc instance.
 * @param f Pointer to the CAN frame to be populated.
 */
void _tg3spmc_encode_frame_h42C(struct tg3spmc *self,
				struct tg3spmc_frame *f)
{
	/* TODO return frame instead of writing by reference */
	struct tg3spmc_config *s = &self->_config;

	uint16_t raw_set_current_ac_A = s->current_ac_A * 1500.0f;

	/* Use specific module ID */
	f->id  = 0x42Cu + (self->_id * 0x10u);
	f->len = 8;

	f->data[2] = (raw_set_current_ac_A & 0x00FFu) >> 0u;
	f->data[3] = (raw_set_current_ac_A & 0xFF00u) >> 8u;

	if (self->_state == (uint8_t)_TG3SPMC_STATE_RUNNING &&
	    !self->_hold_start) {
		f->data[1] = 0xBB; /* State-dependent control byte */
		/* FE - normal operation. FF - clear faults. */
		f->data[4] = 0xFE;
	} else {
		f->data[1] = s->rated_voltage_ac_V / 1.2f;
		f->data[4] = 0x64; /* State-dependent control byte */
	}

	/* Unknown, static data */
	f->data[0] = 0x42;
	f->data[5] = 0x00;
	f->data[6] = 0x00;
	f->data[7] = 0x00;
}

/**
 * @brief Encodes the 0x368 (Static Broadcast) CAN frame.
 *
 * This frame contains static data and is sent periodically.
 * @param self Pointer to the tg3spmc instance (unused).
 * @param f Pointer to the CAN frame to be populated.
 */
void _tg3spmc_encode_frame_h368(struct tg3spmc *self,
				struct tg3spmc_frame *f)
{
	/* TODO return frame instead of writing by reference */
	(void)self;

	/* Unknown, static data (every ~100ms) */
	f->id = 0x368;
	f->len = 8;

	f->data[0] = 0x03;
	f->data[1] = 0x49;
	f->data[2] = 0x29;
	f->data[3] = 0x11;
	f->data[4] = 0x00;
	f->data[5] = 0x0c;
	f->data[6] = 0x40;
	f->data[7] = 0xff;
}

/**
 * @brief Queues tx messages for send.
 * User is responsible for further processing of these
 * @param self Pointer to the tg3spmc instance.
 */
void _tg3spmc_queue_tx(struct tg3spmc *self)
{
	struct _tg3spmc_io *i = &self->_io;

	/* TODO implement _tg3spmc_writer_put_frame instead */
	_tg3spmc_encode_frame_h42C(self, &i->tx.frames[0]);

	if (i->tx.enable_broadcast) {
		i->tx.count = 3u;
		_tg3spmc_encode_frame_h45C(self, &i->tx.frames[1]);
		_tg3spmc_encode_frame_h368(self, &i->tx.frames[2]);
	} else {
		i->tx.count = 1u;
	}
}

/**
 * @brief This function will try to catch common error during charging.
 * Timeouts, module errors, etc.
 *
 * It must immediately go into error recovery state, after detection.
 * 
 * @param self Pointer to the tg3spmc instance.
 */
bool _tg3spmc_detected_errors_during_charge(struct tg3spmc *self)
{
	struct _tg3spmc_io   *i = &self->_io;
	struct  tg3spmc_vars *v = &self->_vars;

	bool fault = false;

	if (i->rx.timer_ms >= TG3SPMC_CONST_CAN_RX_TIMEOUT_MS) {
		self->fault_cause = TG3SPMC_FAULT_CAUSE_RX_TIMEOUT;
		i->rx.has_frames = false;
		fault = true;
	}

	if ((i->rx.has_frames) && (v->fault == true)) {
		self->fault_cause = TG3SPMC_FAULT_CAUSE_FAULT_FLAG;
		fault = true;
	}

	return fault;
}

/******************************************************************************
 * TG3SPMC PUBLIC
 *****************************************************************************/
/**
 * @brief Initializes the single phase module controller instance.
 * @param self Pointer to the tg3spmc instance.
 * @param id The module's unique ID (0, 1, or 2).
 */
void tg3spmc_init(struct tg3spmc *self, uint8_t id)
{
	struct _tg3spmc_io     *i = &self->_io;
	struct  tg3spmc_config *s = &self->_config;
	struct  tg3spmc_vars   *v = &self->_vars;

	/* Base */
	self->_id = id;
	/* Ensures the ID is within the allowed range (0-2). */
	assert(self->_id < 3u);

	self->_state = (uint8_t)_TG3SPMC_STATE_CONFIG;

	self->_timer_ms = 0u;

	self->fault_cause = 0u;

	self->_hold_start = true;

	/* IO */
	i->pwron_out = false;
	i->chgen_out = false;
	_tg3spmc_writer_init(&i->tx);
	_tg3spmc_reader_init(&i->rx);

	/* Settings */
	s->voltage_dc_V = 0.0f;
	s->current_ac_A = 0.0f;
	s->rated_voltage_ac_V = 0.0f;

	/* Vars */
	v->voltage_dc_V = 0.0f;
	v->voltage_ac_V = 0u;
	v->current_dc_A = 0.0f;
	v->current_ac_A = 0.0f;

	v->inlet_target_temp_C      = 0;
	v->current_limit_due_temp_A = 0.0f;

	v->temp1_C = 0;
	v->temp2_C = 0;

	v->ac_present = false;
	v->en_present = false;
	v->fault = false;

	v->status = 0u;
}

/**
 * @brief Sets the configuration parameters for the module.
 *
 * Must be called after tg3spmc_init. Triggers the CONFIG state transition.
 * @param self Pointer to the tg3spmc instance.
 * @param config The new configuration structure to apply.
 */
void tg3spmc_set_config(struct tg3spmc *self,
			struct tg3spmc_config config)
{
	struct tg3spmc_config *s = &self->_config;

	*s = config;

	/** Enforce valid values */
	if (config.voltage_dc_V < TG3SPMC_CONST_MIN_DC_VOLTAGE_V) {
		s->voltage_dc_V = TG3SPMC_CONST_MIN_DC_VOLTAGE_V;
	}
}

/**
 * @brief Gets power on pin state.

 * Must be mapped to real hardware as OUTPUT pin.
 * @param self Pointer to the tg3spmc instance.
 */
bool tg3spmc_get_pwron_pin_state(struct tg3spmc *self)
{
	struct _tg3spmc_io *i = &self->_io;

	return i->pwron_out;
}

/**
 * @brief Gets "charge enable" pin state.
 *
 * Must be mapped to real hardware as OUTPUT pin.
 * @param self Pointer to the tg3spmc instance.
 */
bool tg3spmc_get_chgen_pin_state(struct tg3spmc *self)
{
	struct _tg3spmc_io *i = &self->_io;

	return i->chgen_out;
}

/**
 * @brief Retrieves queued TX frame for immediate sending.
 *
 * If there any TX frame is queued, this method returns true
 * and copies message into a frame pointed by `f`.
 *
 * All frames should be redirected to a single phase module.
 * It's up on API user to implement valid CAN transmission.
 *
 * @param self Pointer to the tg3spmc instance.
 * @param[out] f Pointer to the frame where data will be copied.
 * @return **True** if frame was copied, **false** if no TX frames available.
 *
 * @note **Side effects:** this function will pop queued frames.
 */
bool tg3spmc_get_tx_frame(struct tg3spmc *self, struct tg3spmc_frame *f)
{
	struct _tg3spmc_io *i = &self->_io;

	bool frame_available = false;

	if (i->tx.count > 0u) {
		i->tx.count--;

		*f = i->tx.frames[i->tx.count];

		frame_available = true;
	}

	return frame_available;
}

/**
 * @brief Processes and consumes a received (RX) frame.
 *
 * All frames from single phase module should be consumed by this function.
 * It's up on API user to implement valid CAN reception.
 *
 * @param self Pointer to the tg3spmc instance.
 * @param f    A pointer to the received frame.
 * @return Returns true if frame was consumed successfully.
 */
bool tg3spmc_put_rx_frame(struct tg3spmc *self,
			  struct tg3spmc_frame *f)
{
	_tg3spmc_decode_frame(self, f);

	/* There's no internal limits. Frames will be consumed always. */
	return true;
}

/**
 * @brief Reads charger variables.
 *
 * @param self Pointer to the tg3spmc instance.
 * @param _v   A pointer to the object where read variables will be stored.
 * @return Returns true if charge variables has been read successfully.
 */
bool tg3spmc_read_vars(struct tg3spmc *self, struct tg3spmc_vars *_v)
{
	struct _tg3spmc_io   *i = &self->_io;
	struct  tg3spmc_vars *v = &self->_vars;

	bool vars_been_read = false;

	if (i->rx.has_frames) {
		*_v = *v;
		vars_been_read = true;
	}

	return vars_been_read;
}

/**
 * @brief Set broadcast (either true or false).
 *
 * @param self Pointer to the tg3spmc instance.
 * @param enabled set broadcast enabled/disabled.
 * @note Broadcast is enabled by default.
 *
 * If there are multiple instances of tg3spmc is running, it's advised to
 * have only one broadcast message enabled. This behaviour may be changed
 * or enhanced in the future.
 */
void tg3spmc_set_broadcast(struct tg3spmc *self, bool enabled)
{
	struct _tg3spmc_io *i = &self->_io;

	i->tx.enable_broadcast = enabled;
}

/**
 * @brief Performs a single step of the module controller's state machine.
 * @param self Pointer to the tg3spmc instance.
 * @param delta_time_ms Time elapsed since the last step (milliseconds).
 * @return An event (enum tg3spmc_event) indicating any state change.
 */
enum tg3spmc_event tg3spmc_step(struct tg3spmc *self,
				      uint32_t delta_time_ms)
{
	struct  tg3spmc_config *s = &self->_config;
	struct _tg3spmc_io     *i = &self->_io;

	enum tg3spmc_event ev = TG3SPMC_EVENT_NONE;

	/* TODO, make postconditions and preconditions clear enough.
	 * FSM must follow Design-By-Contract approach */
	switch (self->_state) {
	case _TG3SPMC_STATE_CONFIG:
		/* Validate config before proceed to the next state */
		if ((s->rated_voltage_ac_V <= 0.0f) ||
		    (s->voltage_dc_V < TG3SPMC_CONST_MIN_DC_VOLTAGE_V)) {
			ev = TG3SPMC_EVENT_CONFIG_INVALID;
			break;
		}

		self->_state = _TG3SPMC_STATE_BOOT;
		ev = TG3SPMC_EVENT_POWER_ON;

		/* Power on module */
		i->pwron_out = true;

		/* _TG3SPMC_STATE_BOOT init */
		self->_timer_ms = 0u;
		i->tx.timer_ms  = 0u;

		break;

	/* Keep little delay before transmission and charging start */
	case _TG3SPMC_STATE_BOOT:
		self->_timer_ms += delta_time_ms;

		/* Increment TX timer */
		i->tx.timer_ms += delta_time_ms;

		if (self->_timer_ms < TG3SPMC_CONST_BOOT_TIME_MS) {
			break;
		}

		ev = TG3SPMC_EVENT_CHARGE_ENABLED;
		self->_state = _TG3SPMC_STATE_RUNNING;

		/* Enable charge mode */
		i->chgen_out = true;

		/* _TG3SPMC_STATE_RUNNING init */
		i->rx.timer_ms    = 0u;
		i->rx.has_frames  = false;
		self->_timer_ms   = 0u;
		self->_hold_start = true;

		break;

	/* We send messages and validate charging process in this state */
	case _TG3SPMC_STATE_RUNNING:
		self->_timer_ms += delta_time_ms;

		/* Wait 1000ms before setting initial setup flag to false
		 * TODO we must somehow detect that charger mode is ready to
		 * provide output. TODO no magic numbers */
		if (self->_timer_ms > 1000u) {
			self->_hold_start = false;
		}

		i->tx.timer_ms += delta_time_ms;
		i->rx.timer_ms += delta_time_ms;

		if (i->tx.timer_ms >= TG3SPMC_CONST_CAN_TX_PERIOD_MS) {
			i->tx.timer_ms -= TG3SPMC_CONST_CAN_TX_PERIOD_MS;

			_tg3spmc_queue_tx(self);
		}

		if (_tg3spmc_detected_errors_during_charge(self)) {
			self->_state = _TG3SPMC_STATE_FAULT;
			ev = TG3SPMC_EVENT_FAULT;

			/* Disable module power and charge */
			i->pwron_out = false;
			i->chgen_out = false;

			/* TG3SPMC_EVENT_FAULT init */
			i->tx.count     = 0u;
			self->_timer_ms = 0u;
		}

		break;

	case _TG3SPMC_STATE_FAULT:
		/* Wait before recovery */
		self->_timer_ms += delta_time_ms;

		if (self->_timer_ms < TG3SPMC_CONST_FAULT_RECOVERY_TIME_MS) {
			break;
		}

		self->_state = _TG3SPMC_STATE_CONFIG;
		ev = TG3SPMC_EVENT_RECOVERY;

		/* _TG3SPMC_STATE_CONFIG init */
		i->rx.has_frames = false;
		i->rx.recv_flags = 0u;

		break;

	default:
		assert(0);
		while (1) {};
		break;
	}

	return ev;
}
