# About
This directory contains various files for Savvy CAN software
(commonly used for reverse engineering of CAN data)

Specifically included:
- Preconfigured graphs for every message
- DBC file for discovered messages
- Logs that have been used for discovery

This Directory will be updated upon research

# Discoveries
See .dbc file for more description.


AC params (AC Side) 0x207(519), 0x209(521), 0x20B(523):
- 06.11.2025: Discovered `precharge_en` signal. Deduced based on
measured current spike during that. (maybe use `softstart_en` instead?)
- 06.11.2025: Discovered `powergate_en` signal. Didn't know how to name
that, but current flow starts right after this one. This is flag probably
means that PWM output activates
- 06.11.2025: Discovered `unknown_val2`. Have no idea what's that, but it's only
there if VAC present, it has form of almost pure sine with 237 amplitude (signed int)
and period of approx ~14.5seconds (+- 2 second deviations).
Amplitude looks very close to grid or rated VAC.
It affects `voltage_V` just a little on its peaks (both negative and positive).
it also affects `unknown_val3` on its peaks.
- 06.11.2025: Discovered `unknown_val3`. Some ADC reading, unknown purpose.
It has a spike on start and end. Closely related with `unknown_flg5`.
Only appears when charging is enabled.
- 06.11.2025: Discovered `unknown_flg5`. Probably triggers or indicates `unknown_val3` presence.
- 06.11.2025: Discovered `peak_current_limit_A`.
- 06.11.2025: Discovered `unknown_flg7`. Goes from high to low at the very beginning of charge.

Status 0x217(535), 0x219(537), 0x21B(539):
- 06.11.2025: `flag_chen_pin` when chgen pin enabled, it becomes true
- 06.11.2025: `flag_unk6` goes high during precharge_en, maybe this is actual precharge signal?
- 06.11.2025: `flag_unk7` goes high 5sec after precharge_en, causing very small current spike.
It's probably HVDC contactor status (on)

DC params (DC Side) 0x227(551), 0x229(553), 0x22B(555):
- 06.11.2025: neither DC voltage, nor DC current are affected by AC side `precharge_en`,
nor by status flags in general. I assume that second status message is
responsible for soft start and DC measurements are made before DC precharge circuit.
