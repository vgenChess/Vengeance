#ifndef CONSTANTS_H
#define CONSTANTS_H

const uint8_t VALUI8_NO_BOUND = 50;
const uint8_t VALUI8_AP_WINDOW = 20;
const uint8_t VALUI8_MAX_EXTENSION = 10;
const uint8_t VALUI8_LMP_BASE = 4;
const uint8_t VALUI8_LMP_DEPTH = 8; 
const uint8_t VALUI8_HISTORY_PRUNING_DEPTH = 1;
const uint8_t VALUI8_CAPTURE_HISTORY_PRUNING_DEPTH = 1;

const uint16_t VALUI16_WIN_SCORE = 9000;
const uint16_t VALUI16_PV_DISPLAY_INTERVAL = 2500;
const uint16_t VALUI16_CHECK_NODES = 1024;
const uint16_t VALUI16_COUNTER_MOVE_BONUS = 2500;
const uint16_t VALUI16_FPRUNE = 265;
const uint16_t VALUI16_EXT_FPRUNE = 515;
const uint16_t VALUI16_LTD_RAZOR = 915;
const uint16_t VALUI16_RVRFPRUNE = 265;
const uint16_t VALUI16_EXT_RVRFPRUNE = 515;
const uint16_t VALUI16_LTD_RVRRAZOR = 915;
const uint16_t VALUI16_Q_DELTA = 200;

const int16_t VALI16_NO_DEPTH = -127;
const int16_t VALI16_HISTORY_PRUNING = -5000;   
const int16_t VALI16_CAPTURE_PRUNING = 0;
const int16_t VALI16_SEE_PRUNING = -550;
const int16_t VALI16_QSEE_PRUNING = 0;

const int32_t VALI32_INF = 50000;
const int32_t VALI32_UNKNOWN = 30000;
const int32_t VALI32_MATE = 25000;

const float VALF_ONE_PLY = 1.25;
const float VALF_CHECK_EXT = 0.75;
const float VALF_MATE_THREAT_EXT = 0.5;
const float VALF_RECAPTURE_EXT = 0.5;
const float VALF_PRANK_EXT = 1;
const float VALF_PROMOTION_EXT = 0.75;

#endif



