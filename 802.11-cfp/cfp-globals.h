#ifndef CFPGLOBALS_H
#define CFPGLOBALS_H

// List of all global variables to be used in CFP Period

double SIFS = 16;
double slotTime = 9;
double PIFS = 25;
double DIFS = 34;
double decodeDelay = 1;
double preambleDetection = 4;

double pollTxTime = 96; // magic number for OFDM 18 Mbps and pollSize = 83
double NullTxTime = 40;
double ACKTxTime = 40;

double pollSize = 83;
double ACKSize = 20;

#endif
