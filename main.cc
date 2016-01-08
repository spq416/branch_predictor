#include <iostream>
#include <fstream>
#include <string.h>
#include <sstream>
#include <iomanip>
#include <math.h>
#include <stdlib.h>

using namespace std;

//INTITIALIZING COUNTERS AND DYNAMIC ARRAYS
int bhr;
int bi_index, gsh_index, cho_index;
int M1,M2,N,K;
int mode;
int mispredict;
int * bimodal = NULL;
int * gshare  = NULL;
int * chooser = NULL;
void Access(long,int);
void initPred(int,int,int,int);
void calcIndex(long);

//PREDICTOR INITIALIZATION

void initPred(int P_M1, int P_M2, int P_N, int P_K)
{
    bhr = 0;
    M1 = M2 = N = K = bi_index = gsh_index = 0;
    int pred_size;
    if(P_K == 0)
    {
        if (P_M2 == 0) // GSHARE
        {
            mode = 1;
            M1 = P_M1;
            N = P_N;
            pred_size = pow(2,M1);
            gshare = new int[pred_size];
            for (int i=0; i<pred_size; i++)
                gshare[i] = 2;
        }
        else        // Bi-modal
        {
            mode = 0;
            M2 = P_M2;
            pred_size = pow(2,M2);
            bimodal = new int[pred_size];
            for (int i=0; i<pred_size; i++)
                bimodal[i] = 2;
        }
    }
    else        // HYBRID
    {
            M1 = P_M1;
            N = P_N;
            pred_size = pow(2,M1);
            gshare = new int[pred_size];
            for (int i=0; i<pred_size; i++)
                gshare[i] = 2;

            M2 = P_M2;
            pred_size = pow(2,M2);
            bimodal = new int[pred_size];
            for (int i=0; i<pred_size; i++)
                bimodal[i] = 2;

            K = P_K;
            mode = 2;
            pred_size =  pow(2,K);
            chooser = new int[pred_size];
            for (int i=0; i<pred_size; i++)
                chooser[i] = 1;
    }
}

//PREDICTOR ACCESS

    void Access (long addr, int taken)
    {
        addr = addr>>2;
        int bi_counter = 0, gsh_counter = 0;
        calcIndex(addr);
        switch (mode)
        {
        case 0:         //BIMODAL
            {
            if (bimodal[bi_index] >= 2)
                bi_counter = 1;

            if(taken == 1 && bimodal[bi_index] < 3)
                bimodal[bi_index]++;
            else if (taken == 0 && bimodal[bi_index] > 0)
                bimodal[bi_index]--;

            if (bi_counter != taken)
                mispredict++;
                break;
            }
        case 1:         //GSHARE
            {
            if (gshare[gsh_index] >= 2)
                gsh_counter = 1;

            if (taken == 1 && gshare[gsh_index] < 3)
                gshare[gsh_index]++;
            else if (taken == 0 && gshare[gsh_index] > 0)
                gshare[gsh_index]--;
			
            bhr = bhr>>1;
            int temp = taken<<(N-1);
            bhr = bhr | temp;

            if (gsh_counter != taken)
                mispredict++;
            break;
            }
        case 2:         //HYBRID
            {
            if (bimodal[bi_index] >= 2)
                bi_counter = 1;

            if (gshare[gsh_index] >= 2)
                gsh_counter = 1;

            if (chooser[cho_index]<2)
            {
                if(taken == 1 && bimodal[bi_index] < 3)
                    bimodal[bi_index]++;
                else if (taken == 0 && bimodal[bi_index] > 0)
                    bimodal[bi_index]--;

                if (bi_counter != taken)
                    mispredict++;
            }
            else
            {
                if (taken == 1 && gshare[gsh_index] < 3)
                    gshare[gsh_index]++;
                else if (taken == 0 && gshare[gsh_index] > 0)
                    gshare[gsh_index]--;

                if (gsh_counter != taken)
                    mispredict++;
            }

            bhr = bhr>>1;
            int temp = taken<<(N-1);
            bhr = bhr | temp;

            if (gsh_counter == taken)
            {
                if (bi_counter != taken)
                {
                    if (chooser[cho_index] < 3)
                        chooser[cho_index]++;
                }
            }
            else
            {
                if (bi_counter == taken)
                {
                    if (chooser[cho_index] > 0)
                        chooser[cho_index]--;
                }
            }

            }
        }
    }

	//INDEX CALCULATION
	
    void calcIndex (long addr)
    {
        int bhr_mask, ind_mask, cho_mask, temp_mask, index;
        switch(mode)
        {
        case 0:     //BIMODAL
            {
            ind_mask = pow(2,M2) - 1;
            bi_index = addr & ind_mask;
            break;
            }
        case 1:     //GSHARE
            {
            int diff = M1 - N;

            ind_mask = pow(2,M1) - 1;
            bhr_mask = pow(2,N) - 1;
            temp_mask = (pow(2,diff) - 1);

            index = addr & ind_mask;

            int temp = index & temp_mask;
            index = (index>>diff)^(bhr & bhr_mask);
            index = (index<<diff)|temp;
            gsh_index = index;
            break;
            }
        case 2:     //HYBRID
            {
            ind_mask = pow(2,M2) - 1;
            bi_index = addr & ind_mask;

            /*---------------------*/

            int diff = M1 - N;

            ind_mask = pow(2,M1) - 1;
            bhr_mask = pow(2,N) - 1;

            index = addr & ind_mask;

            int temp = index & (int)(pow(2,diff) - 1);
            index = (index>>diff)^(bhr & bhr_mask);
            index = (index<<diff)|temp;
            gsh_index = index;

            /*---------------------*/

            cho_mask = pow(2,K) - 1;
            cho_index = addr & cho_mask;
            break;
            }
        }
    }


int main (int argc, char *argv[])
{
    int branches = 0;
    string predictor;
    char *trace =  (char *)malloc(20);
    predictor = argv[1];

    if (predictor == "bimodal") // NEED JUST ONE ARRAY
    {
        M2 = atoi(argv[2]);
        trace = argv[3];
        initPred(0,M2,0,0);
    }
    else if (predictor == "gshare")  // NEED TWO ARRAYS (ONE FOR THE BHR AND ONE FOR THE PREDICTOR)
    {
        M1 = atoi(argv[2]);
        N = atoi(argv[3]);
        trace = argv[4];
        initPred(M1,0,N,0);
    }
    else if (predictor == "hybrid")  // NEED 3 ARRAYS
    {
        K = atoi(argv[2]);
        M1 = atoi(argv[3]);
        N = atoi(argv[4]);
        M2 = atoi(argv[5]);
        trace = argv[6];
        initPred(M1,M2,N,K);
    }
    else
    {
        cout<<"Wrong input for mode";
        exit(0);
    }


   ifstream fin;
   FILE * pFile;
   char op;

	pFile = fopen (trace,"r"); //trace

	if(pFile == 0)
	{
		printf("Trace file problem\n");
		exit(0);
	}

	fin.open(trace, ios::in); //trace
	string line;

	while (getline(fin,line))
	{
		long addr_in_hex;
		int taken;

        op = line[7];
        if (op == 't')
            taken = 1;
        else
            taken = 0;

        string address = line.substr(0,6);

        istringstream buffer (address);
        buffer >> hex >> addr_in_hex;

		// ACCESS THE PREDICTOR
        Access(addr_in_hex, taken);
        branches++;

	}

	fclose(pFile);

    cout<<"COMMAND \n";
    for (int i = 0; i<argc; i++)
    	cout<<argv[i]<<" ";
    cout<<"\n OUTPUT";
    cout<<"\n number of predictions: "<<branches;
    cout<<"\n number of mispredictions: "<<mispredict;
    cout<<"\n misprediction rate: "<<fixed <<setprecision(2) <<((((float)(mispredict))/branches)*100)<<"%";

    switch(mode)
    {
    case 0:
        {
            cout<<"\n  FINAL BIMODAL CONTENTS ";
            int x = pow(2,M2);
            for (int i=0; i<x; i++)
                cout<<"\n "<<i<<" "<<bimodal[i];
        break;
        }
    case 1:
        {
        cout<<"\n  FINAL GSHARE CONTENTS ";
        int x = pow(2,M1);
        for (int i=0; i<x; i++)
            cout<<"\n "<<i<<" "<<gshare[i];

        break;
        }
    case 2:
        {
        cout<<"\n FINAL CHOOSER CONTENTS ";
        int x = pow(2,K);
        for (int i=0; i<x; i++)
            cout<<"\n "<<i<<" "<<chooser[i];

        cout<<"\n  FINAL GSHARE CONTENTS ";
        int z = pow(2,M1);
        for (int i=0; i<z; i++)
            cout<<"\n "<<i<<" "<<gshare[i];

	cout<<"\n  FINAL BIMODAL CONTENTS ";
        int y = pow(2,M2);
        for (int i=0; i<y; i++)
            cout<<"\n "<<i<<" "<<bimodal[i];
        }
    }

    cout<<endl;
}
