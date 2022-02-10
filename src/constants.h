
#ifndef CONSTANTS
#define CONSTANTS 

const int INF = 	50000;
const int MATE = 	25000;

const uint16_t WIN_SCORE_THRESHOLD = 9000;

const int8_t NO_STAGE = -1;
const uint8_t NO_DEPTH = 0;
const uint8_t NO_BOUND = 50;
const uint8_t AP_WINDOW = 16;

const uint16_t PV_DISPLAY_INTERVAL = 2500;
const uint16_t X_NODES = 1024;

const uint16_t BONUS_COUNTER_MOVE = 2500;

const uint16_t F_PRUNE_THRESHOLD = 		265;
const uint16_t EXT_F_PRUNE_THRESHOLD = 	515;
const uint16_t LTD_RZR_THRESHOLD = 		915;

const uint16_t R_F_PRUNE_THRESHOLD = 		265;
const uint16_t R_EXT_F_PRUNE_THRESHOLD = 	515;
const uint16_t R_LTD_RZR_THRESHOLD = 		915;

const uint8_t MAX_EXTENSION = 15;

const uint8_t LMP_BASE 		= 4;
const uint8_t LMP_MAX_DEPTH = 6; 

const uint8_t HISTORY_PRUNING_MAX_DEPTH =		1;
const int16_t HISTORY_PRUNING_THRESHOLD  =	-5000;   

const uint8_t CAPTURE_HISTORY_PRUNING_MAX_DEPTH =	1;
const int16_t CAPTURE_PRUNING_THRESHOLD = 			0;

const int16_t SEE_PRUNING_THRESHOLD = -550;

const uint16_t Q_DELTA = 200;
const int16_t Q_SEE_PRUNING_THRESHOLD = 0;
const int16_t Q_MVV_LVA_PRUNING_THRESHOLD = 0;

#endif



