#ifndef tuner_h
#define tuner_h

#include <string>

#include "thread.h"
#include "evaluate.h"
			
#define NTERMS 20480 + 130	  
#define KPRECISION 10

enum { NORMAL, SAFETY };
enum { MG, EG };

typedef int TArray[NTERMS];

typedef double TVector[2][NTERMS];

typedef struct TGradientData {
    double wsafetymg, bsafetymg;
    double wsafetyeg, bsafetyeg;
} TGradientData;

struct CoefficientsInfo {
	
	uint16_t index;

    int8_t wcoeff;
    int8_t bcoeff;

    int8_t type;
};

struct Data {
	
	float result;

	std::vector<CoefficientsInfo> coefficientsInfoList;
	float pfactors[2];

	int phase;
	int eval;
	int sEval;
	int safety[2];
};

double sigmoid(double K, double E);

void startTuner();

void optimise(TVector params, TVector cparams);

void computeGradient(TVector gradient, TVector weights, std::vector<Data> data_batch,  double K);

void updateSingleGradient(Data data, TVector gradient, TVector weights, double K);

double linearEvaluation(TVector weights, Data data, TGradientData *gradientData);

double tunedEvaluationErrors(TVector weights, double K);
double staticEvaluationErrors(double K);

double computeOptimalK();

void saveWeights(TVector params, TVector cparams);

#endif