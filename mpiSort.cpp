#include "mpi.h"
#include<iostream>
#include <stdio.h>
#include <cstdlib>
#include <ctime>

using namespace std;
void initArray(int* arr, int n) {
	int i;
	for (i = 0; i < n; ++i)
	{
		arr[i] = rand() % n;
	}
}

void printArray(int* arr, int n) {
	int i;
	for (i = 0; i < n; ++i)
	{
		cout << arr[i] << "\t";
	}
}

void swap(int* arr, int i, int j) {
	int temp = arr[i];
	arr[i] = arr[j];
	arr[j] = temp;
}

void quicksortNormal(int* arr, int lo, int hi) {
	if (lo < hi) {
		int x = arr[lo];
		int pivot = lo;
		for (int i = lo + 1; i < hi; ++i)
		{
			if (arr[i] <= x) {
				pivot++;
				swap(arr, pivot, i);
			}
		}
		swap(arr, lo, pivot);

		quicksortNormal(arr, lo, pivot);
		quicksortNormal(arr, pivot + 1, hi);
	}
}

void quicksortMPI(int* arr, int lo, int hi, int rank, int numtasks, int rank_index) {
	int dest = rank + (1 << rank_index);

	if (dest >= numtasks) {
		quicksortNormal(arr, lo, hi);
	}
	else if (lo < hi) {
		int x = arr[lo];
		int pivot = lo;

		for (int i = lo + 1; i < hi; ++i)
		{
			if (arr[i] <= x) {
				pivot++;
				swap(arr, pivot, i);
			}
		}
		swap(arr, lo, pivot);

		if (pivot - lo > hi - pivot - 1) {
			MPI_Send(&arr[pivot + 1], hi - pivot - 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
			quicksortMPI(arr, lo, pivot, rank, numtasks, rank_index + 1);
			MPI_Recv(&arr[pivot + 1], hi - pivot - 1, MPI_INT, dest, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		else {
			MPI_Send(&arr[lo], pivot - lo, MPI_INT, dest, 1, MPI_COMM_WORLD);
			quicksortMPI(arr, pivot + 1, hi, rank, numtasks, rank_index + 1);
			MPI_Recv(&arr[lo], pivot - lo, MPI_INT, dest, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
	}
}

int main(int argc, char** argv) {
	srand(time(0));
	int numtasks, rank;
	int* tempArr;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Status msg;

	if (rank == 0) {
		const int n = 100000;
		int arr[n];
		initArray(arr, n);

		double start = MPI_Wtime();
		quicksortMPI(arr, 0, n, rank, numtasks, 0);
		double finish = MPI_Wtime();

		printArray(arr, n);
		cout << "N: " << n << "\n";
		cout << "Time: " << finish - start << " seconds\n";
	}
	else {
		int rc;
		int arrSize;
		int source;
		int index_count = 0;
		while (1 << index_count <= rank)
			index_count++;

		rc = MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &msg);
		rc = MPI_Get_count(&msg, MPI_INT, &arrSize);
		source = msg.MPI_SOURCE;

		tempArr = (int*)malloc(sizeof(int) * arrSize);
		rc = MPI_Recv(tempArr, arrSize, MPI_INT, source, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		quicksortMPI(tempArr, 0, arrSize, rank, numtasks, index_count);
		rc = MPI_Send(tempArr, arrSize, MPI_INT, source, 1, MPI_COMM_WORLD);
		free(tempArr);
	}
	MPI_Finalize();
	return 0;
}
