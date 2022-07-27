#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>

const uint8_t MAX_SIDES = 2;
const uint8_t MAX_PIECES = 8;
const uint8_t MAX_SQUARES = 64;
const uint8_t HASH_AGE = 6;
const uint8_t LMP_DEPTH = 8;
const uint8_t NO_BOUND = 50;
const uint8_t AP_WINDOW = 20;
const uint8_t HISTORY_PRUNING_DEPTH = 2;
const uint8_t SEE_PRUNING_DEPTH = 1;
const uint8_t MAX_PV_LENGTH = 24;

const uint16_t MAX_PLY = 128;
const uint16_t MAX_MOVES = 256;
const uint16_t WIN_SCORE = 9000;
const uint16_t PV_DISPLAY_INTERVAL = 2500;
const uint16_t CHECK_NODES = 1024;
const uint16_t U16_COUNTER_MOVE_BONUS = 2500;
const uint16_t U16_FPRUNE = 265;
const uint16_t U16_EXT_FPRUNE = 515;
const uint16_t U16_RAZOR_MARGIN = 250;
const uint16_t U16_RVRFPRUNE = 265;
const uint16_t U16_EXT_RVRFPRUNE = 515;
const uint16_t VAL_Q_DELTA = 150;
const uint16_t CURRMOVE_INTERVAL = 3000;

const int16_t NO_DEPTH = -127;
const int16_t HISTORY_PRUNING = -10000;
const int16_t SEE_PRUNING = -1000;
const int16_t I16_QSEE_PRUNING = 0;

const uint16_t PAWN_HASH_TABLE_RECORDS = 1024 * 4;
const uint16_t EVAL_HASH_TABLE_RECORDS = 1024 * 8;

const int32_t I32_INF = 50000;
const int32_t VAL_UNKNOWN = 40000;
const int32_t MATE = 30000;

#define PLY 8

const int NN_SIZE = 128;

#endif



