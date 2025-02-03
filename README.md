Notes on DTMFDetector
=====================

This class provides a standards-compliant DTMF tone decoder.  At the moment the sample rate is
fixed at 8 kHz.

References:
* [ETSI Standard Document for DTMF receivers](https://www.etsi.org/deliver/etsi_es/201200_201299/20123503/01.02.01_50/es_20123503v010201m.pdf) for 
more information.
* My imlpementation borrows heavily from a TI application note called [DTMF Tone Generation and Detection:
An Implementation Using the TMS320C54x](https://www.ti.com/lit/an/spra096a/spra096a.pdf).
* Another application note: https://www.infineon.com/dgdl/Infineon-AN2122_Analog_Standard_DTMF_Detector-ApplicationNotes-v08_00-EN.pdf?fileId=8ac78c8c7cdc391c017d0736b28259bd

The standard tones are (all in Hz):

* Rows: 697, 770, 852, 941
* Cols: 1209, 1336, 1477, 1633

The minimum distance between tones is about 70 Hz.  Assuming am 8 kHz samlpe rate, that implies that the 
smallest amount of DFT resolution required for compliant detection is: 8,000 / 70 = 114 bins.
This implementation provides about 10 Hz of buffer and uses 136 bins.

The AT&T standard requires that tones that are >3.5% from the standard should 
be considered invalid.  This equates to about 24 Hz of resolution on the low end of the spectrum.  We aren't 
using enough spectral resolution to make this determination.

The official ETSI receiver standard requires that a valid tone be within 1.5% + 2 Hz of the standard frequency,
so are not using quite enough resolution to support that determination.  I note that the ETSI standard
document doesn't explicitly say that >1.5% of frequency error is not valid (see section 4.2.1.2).



