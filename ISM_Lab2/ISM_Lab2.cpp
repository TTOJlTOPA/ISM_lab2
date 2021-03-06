#include "pch.h"
#include <iostream>
#include <fstream>
#include "MultiplicativePRNG.h"
#include "PoissonModel.h"
#include "NegativeBinomialModel.h"
#include <algorithm>

#define GNUPLOT             "gnuplot"
#define GNUPLOT_WIN_WIDTH   1280
#define GNUPLOT_WIN_HEIGHT  720

using namespace std;

FILE* outputFile;


const char* printBool(bool value)
{
	return (value) ? "true" : "false";
}


double calcEstimationMean(const int* source, int num)
{
	int sum = 0;

	for (int i = 0; i < num; i++)
	{
		sum += source[i];
	}

	return (double)sum / num;
}

double calcEstimationVariance(const int* source, int num, double mean)
{
	double sum = 0.0;

	for (int i = 0; i < num; i++)
	{
		sum += pow(source[i] - mean, 2.0);
	}

	return (double)sum / (num - 1);
}

double calcPoissonMean(double lambda)
{
	return lambda;
}

double calcPoissonVariance(double lambda)
{
	return lambda;
}

double calcNegativeBinomialMean(double probability, int failNum)
{
	return probability * failNum / (1.0 - probability);
}

double calcNegativeBinomialVariance(double probability, int failNum)
{
	return probability * failNum / pow(1.0 - probability, 2.0);
}


int* calcFrequenciesEmperic(int* sequence, int num, int cellNum)
{
	int* result = new int[cellNum];
	int resultIndex = sequence[0];
	int prev = 0;

	for (int i = 0; i < cellNum; i++)
	{
		result[i] = 0;
	}

	for (int i = 0; i < num; i++)
	{
		if (sequence[i] != prev)
		{
			resultIndex = sequence[i];
			prev = sequence[i];
		}
		result[resultIndex]++;
	}

	return result;
}

bool checkPearsonTestPoisson(const double* quantiles, int* sequence, int num, double lambda)
{
	int* empericFreq;
	int cellNum;
	double chi = 0.0;
	double expectedCount = num * exp(-lambda);

	sort(sequence, &sequence[num]);
	cellNum = sequence[num - 1] + 1;
	empericFreq = calcFrequenciesEmperic(sequence, num, cellNum);

	chi += pow(empericFreq[0] - expectedCount, 2.0) / expectedCount;
	for(int i = 1; i < cellNum; i++)
	{
		expectedCount *= lambda / i;
		chi += pow(empericFreq[i] - expectedCount, 2.0) / expectedCount;
	}

	delete[] empericFreq;

	printf("Chi: %f\n", chi);
	fprintf(outputFile, "Chi: %f\n", chi);
	printf("Quantile: %f\n", quantiles[cellNum - 2]);
	fprintf(outputFile, "Quantile: %f\n", quantiles[cellNum - 2]);

	return (chi < quantiles[cellNum - 2]);
}

bool checkPearsonTestNegativeBinomial(const double* quantiles, int* sequence, int num, int failNum, double probability)
{
	int* empericFreq;
	int cellNum;
	double chi = 0.0;
	double expectedCount = num * pow(1.0 - probability, failNum);

	sort(sequence, &sequence[num]);
	cellNum = sequence[num - 1] + 1;
	empericFreq = calcFrequenciesEmperic(sequence, num, cellNum);

	chi += pow(empericFreq[0] - expectedCount, 2.0) / expectedCount;
	for(int i = 1; i < cellNum; i++)
	{
		expectedCount *= (i + failNum - 1) * probability / i;
		chi += pow(empericFreq[i] - expectedCount, 2.0) / expectedCount;
	}

	delete[] empericFreq;

	printf("Chi: %f\n", chi);
	fprintf(outputFile, "Chi: %f\n", chi);
	printf("Quantile: %f\n", quantiles[cellNum - 2]);
	fprintf(outputFile, "Quantile: %f\n", quantiles[cellNum - 2]);

	return (chi < quantiles[cellNum - 2]);
}


int main()
{
	FILE* pipePoisson;
	FILE* pipeNegBin;

#ifdef WIN32
    pipePoisson = _popen(GNUPLOT, "w");
	pipeNegBin = _popen(GNUPLOT, "w");
#else
    pipePoisson = popen(GNUPLOT, "w");
	pipeNegBin = popen(GNUPLOT, "w");
#endif

	const double quantiles[20] = {3.8415, 5.9915, 7.8147, 9.4877, 11.07, 12.592, 14.067, 15.507, 16.919, 18.307, 
								  19.675, 21.026, 22.362, 23.685, 24.996, 26.296, 27.587, 28.869, 30.144, 31.41};
    const int num = 1000;
	const int failNumNegBin = 6;
	const double lambdaPoisson = 3.0;
	const double probabilityNegBin = 0.25;

	PRNG* prng = new MultiplicativePRNG(pow(2, 31), 262147, 262147);
	PoissonModel* poisson = new PoissonModel(prng, lambdaPoisson);
	NegativeBinomialModel* negativeBinomial = new NegativeBinomialModel(prng, failNumNegBin, probabilityNegBin);

	fopen_s(&outputFile, "output.txt", "w");

	int* poissonResult = new int[num];
	int* negativeBinomialResult = new int[num];

	double mean;
	double variance;
	bool testResult;

	delete prng;

	for (int i = 0; i < num; i++)
	{
		poissonResult[i] = poisson->next();
		negativeBinomialResult[i] = negativeBinomial->next();
	}

	printf("Poisson model:\n");
	fprintf(outputFile, "Poisson model:\n");
	for (int i = 0; i < num; i++)
	{
		fprintf(outputFile, "%d\n", poissonResult[i]);
	}
	mean = calcEstimationMean(poissonResult, num);
	variance = calcEstimationVariance(poissonResult, num, mean);
	printf("Mean: %f\n", mean);
	fprintf(outputFile, "Mean: %f\n", mean);
	printf("Variance: %f\n", variance);
	fprintf(outputFile, "Variance: %f\n", variance);
	mean = calcPoissonMean(lambdaPoisson);
	variance = calcPoissonVariance(lambdaPoisson);
	printf("Real mean: %f\n", mean);
	fprintf(outputFile, "Real mean: %f\n", mean);
	printf("Real variance: %f\n", variance);
	fprintf(outputFile, "Real variance: %f\n", variance);
	testResult = checkPearsonTestPoisson(quantiles, poissonResult, num, lambdaPoisson);
	printf("Pearson test: %s\n", printBool(testResult));
	fprintf(outputFile, "Pearson test: %s\n", printBool(testResult));

	printf("\nNegative binomial model:\n");
	fprintf(outputFile, "\nNegative binomial model:\n");
	for (int i = 0; i < num; i++)
	{
		fprintf(outputFile, "%d\n", negativeBinomialResult[i]);
	}
	mean = calcEstimationMean(negativeBinomialResult, num);
	variance = calcEstimationVariance(negativeBinomialResult, num, mean);
	printf("Mean: %f\n", mean);
	fprintf(outputFile, "Mean: %f\n", mean);
	printf("Variance: %f\n", variance);
	fprintf(outputFile, "Variance: %f\n", variance);
	mean = calcNegativeBinomialMean(probabilityNegBin, failNumNegBin);
	variance = calcNegativeBinomialVariance(probabilityNegBin, failNumNegBin);
	printf("Real mean: %f\n", mean);
	fprintf(outputFile, "Real mean: %f\n", mean);
	printf("Real variance: %f\n", variance);
	fprintf(outputFile, "Real variance: %f\n", variance);
	testResult = checkPearsonTestNegativeBinomial(quantiles, negativeBinomialResult, num, failNumNegBin, probabilityNegBin);
	printf("Pearson test: %s\n", printBool(testResult));
	fprintf(outputFile, "Pearson test: %s\n", printBool(testResult));

	fclose(outputFile);


	if (pipePoisson != nullptr && pipeNegBin != nullptr)
    {
		int cellNum = 20;

		fprintf(pipePoisson, "n=%d\n", cellNum);
		fprintf(pipePoisson, "min=%f\n", 0.0);
		fprintf(pipePoisson, "max=%f\n", 10.0);
		fprintf(pipePoisson, "width=(max-min)/n\n");
		fprintf(pipePoisson, "hist(x,width)=width*floor(x/width)+width/2.0\n");
		fprintf(pipePoisson, "set xrange [min:max]\n");
		fprintf(pipePoisson, "set yrange [0:]\n");
		fprintf(pipePoisson, "set offset graph 0.05,0.05,0.05,0.0\n");
		fprintf(pipePoisson, "set xtics min,(max-min)/5,max\n");
		fprintf(pipePoisson, "set boxwidth width*0.9\n");
		fprintf(pipePoisson, "set style fill solid 0.5\n");
		fprintf(pipePoisson, "set tics out nomirror\n");

        fprintf(pipePoisson, "set term wxt size %d, %d\n", GNUPLOT_WIN_WIDTH, GNUPLOT_WIN_HEIGHT);
        fprintf(pipePoisson, "set title \"Poisson (n=%d)\"\n", num);
        fprintf(pipePoisson, "set xlabel \"Values\"\n");
        fprintf(pipePoisson, "set ylabel \"Frequency\"\n");
        fprintf(pipePoisson, "plot '-' u (hist($1,width)):(1.0) smooth freq w boxes lc rgb\"green\" notitle\n");

		for (int i = 0; i < num; i++)
		{
			fprintf(pipePoisson, "%d\n", poissonResult[i]);
		}

		fprintf(pipePoisson, "%s\n", "e");
        fflush(pipePoisson);


		fprintf(pipeNegBin, "n=%d\n", cellNum);
		fprintf(pipeNegBin, "min=%f\n", 0.0);
		fprintf(pipeNegBin, "max=%f\n", 10.0);
		fprintf(pipeNegBin, "width=(max-min)/n\n");
		fprintf(pipeNegBin, "hist(x,width)=width*floor(x/width)+width/2.0\n");
		fprintf(pipeNegBin, "set xrange [min:max]\n");
		fprintf(pipeNegBin, "set yrange [0:]\n");
		fprintf(pipeNegBin, "set offset graph 0.05,0.05,0.05,0.0\n");
		fprintf(pipeNegBin, "set xtics min,(max-min)/5,max\n");
		fprintf(pipeNegBin, "set boxwidth width*0.9\n");
		fprintf(pipeNegBin, "set style fill solid 0.5\n");
		fprintf(pipeNegBin, "set tics out nomirror\n");

        fprintf(pipeNegBin, "set term wxt size %d, %d\n", GNUPLOT_WIN_WIDTH, GNUPLOT_WIN_HEIGHT);
        fprintf(pipeNegBin, "set title \"Negative binomial (n=%d)\"\n", num);
        fprintf(pipeNegBin, "set xlabel \"Values\"\n");
        fprintf(pipeNegBin, "set ylabel \"Frequency\"\n");
        fprintf(pipeNegBin, "plot '-' u (hist($1,width)):(1.0) smooth freq w boxes lc rgb\"green\" notitle\n");

		for (int i = 0; i < num; i++)
		{
			fprintf(pipeNegBin, "%d\n", negativeBinomialResult[i]);
		}

		fprintf(pipeNegBin, "%s\n", "e");
        fflush(pipeNegBin);

		cin.clear();
        cin.get();

#ifdef WIN32
        _pclose(pipePoisson);
        _pclose(pipeNegBin);
#else
        pclose(pipePoisson);
        pclose(pipeNegBin);
#endif
	}


	delete poisson;
	delete negativeBinomial;
	delete[] poissonResult;
	delete[] negativeBinomialResult;
}
