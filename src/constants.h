#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>

const uint8_t U8_MAX_SIDES = 2;
const uint8_t U8_MAX_PIECES = 8;
const uint8_t U8_MAX_SQUARES = 64;
const uint8_t U8_HASH_AGE = 6;

const uint8_t U8_NO_BOUND = 50;
const uint8_t U8_AP_WINDOW = 20;
const uint8_t U8_LMP_BASE = 4;
const uint8_t U8_LMP_DEPTH = 8; 
const uint8_t U8_HISTORY_PRUNING_DEPTH = 2;
const uint8_t U8_SEE_PRUNING_DEPTH = 1;

const uint16_t U16_MAX_PLY = 128;
const uint16_t U16_MAX_MOVES = 256;
const uint16_t U16_WIN_SCORE = 9000;
const uint16_t U16_PV_DISPLAY_INTERVAL = 2500;
const uint16_t U16_CHECK_NODES = 1024;
const uint16_t U16_COUNTER_MOVE_BONUS = 2500;
const uint16_t U16_FPRUNE = 265;
const uint16_t U16_EXT_FPRUNE = 515;
const uint16_t U16_RAZOR_MARGIN = 250;
const uint16_t U16_RVRFPRUNE = 265;
const uint16_t U16_EXT_RVRFPRUNE = 515;
const uint16_t U16_Q_DELTA = 150;
const uint16_t U16_CURRMOVE_INTERVAL = 3000;

const int16_t I16_NO_DEPTH = -127;
const int16_t I16_HISTORY_PRUNING = -10000; 
const int16_t I16_SEE_PRUNING = -1000;
const int16_t I16_QSEE_PRUNING = 0;

const uint16_t U16_PAWN_HASH_TABLE_RECORDS = 1024 * 8;
const uint16_t U16_EVAL_HASH_TABLE_RECORDS = 1024 * 16;

const int32_t I32_INF = 50000;
const int32_t I32_UNKNOWN = 40000;
const int32_t I32_MATE = 30000;

const float F_ONE_PLY = 1.25;
const float F_CHECK_EXT = 0.5;
const float F_MATE_THREAT_EXT = 0.5;
const float F_RECAPTURE_EXT = 0.75;
const float F_PRANK_EXT = 1;
const float F_PROMOTION_EXT = 0.75;
const float F_SINGULAR_EXT = 1.25;

#endif



