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
measured current spike during that.
- 06.11.2025: Discovered `powergate_en` signal. Didn't know how to name
that, but current flow starts right after this one.
- 06.11.2025: Discovered `unknown_val2`. Have no idea what's that, but it's only
there if VAC present, it has form of pure sine with 237 amplitude (signed int)
and period of approx 14.5. Amplitude looks very close to grid or rated VAC.
It affects `voltage_V` just a little on its peaks (both negative and positive).
it also affects `unknown_val3` on its peaks.
- 06.11.2025: Discovered `unknown_val3`. Some ADC reading, unknown purpose.
It has a spike on start and end. Closely related with `unknown_flg5`.
Only appears when charging is enabled.
- 06.11.2025: Discovered `unknown_flg5`. Probably triggers or indicates `unknown_val3` presence.
- 06.11.2025: Discovered `peak_current_limit_A`.
- 06.11.2025: Discovered `unknown_flg7`. Goes from high to low at the very beginning of charge.
