#ifndef tuner_h
#define tuner_h

#include <string>

#include "thread.h"
#include "evaluate.h"
			
#define NTERMS 481 + 2 + 2 + 1 + 10	+ 1 + 1 + 1	
#define KPRECISION 10

enum { NORMAL, SAFETY };
enum { MG, EG };

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


typedef struct TGradientData {
    double wsafetymg, bsafetymg;
    double wsafetyeg, bsafetyeg;
} TGradientData;


typedef int TArray[NTERMS];

typedef double TVector[2][NTERMS];

void startTuner();

double sigmoid(double score);
double sigmoid(double k, double e);

double averageEvaluationError();

void loadWeights(TVector params, TVector cparams);
void displayWeights(TVector params, TVector cparams);

void initCoefficients();

void optimise(TVector params, TVector cparams);

void computeGradient(TVector gradient, TVector weights, std::vector<Data> data_batch,  double K);
void computeGradient(TVector gradient, TVector weights, std::vector<Data> data_batch,  double K, int batch);

void updateSingleGradient(Data data, TVector gradient, TVector weights, double K);

double linearEvaluation(TVector weights, Data data, TGradientData *gradientData);

double tunedEvaluationErrors(TVector weights, double K);
double staticEvaluationErrors(double K);

double computeOptimalK();

void writeToFile(TVector params, TVector cparams);

void writeEvalToFile();
void getEval();

#endif