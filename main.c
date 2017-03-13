#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "tinydir.h"



#define TRAIN_FILE_PATH "D:/SUDHA/Fall 2016/Adv Micro/P4/P4/train_double/"
#define TEST_FILE_PATH "D:/SUDHA/Fall 2016/Adv Micro/P4/P4/test/"

#define DIMENSION 10000
#define CHARACTERS 256

// P2 and P3 combined for P4
const int NumOfClass = 16;
const char *ClassLabel[16] = {"acq_1","acq_2","crude1","crude2","earn1","earn2","grain1","grain2","money-fx1","money-fx2","interest1","interest2","ship1","ship2","trade1","trade2"};
const char *ClassMap[16] = {"ac","ac","cr","cr","ea","ea","gr","gr","mo","mo","in","in","sh","sh","tr","tr"};


double norm_values(long *vec, int n) {
    double result = 0;
    int i;
    for(i=0; i<n; i++) {
        result += (double)vec[i]*vec[i];
    }
    result = sqrt(result);

    return result;
}

double cosAngle_val(long *u, long *v, int n) {
    double dot = 0;
    int i;
    for(i=0; i<n; i++) dot += (double)u[i]*v[i];
    double temp = (norm_values(u,n)*norm_values(v,n));
    double result = dot/temp;

    return result;
}

int* randPerm(int SZ) {
    int *perm;
    perm = (int *) malloc(SZ * sizeof(int));

    for (int i = 0; i < SZ; i++) perm[i] = i;
    for (int i = 0; i < SZ; i++) {
        int j, t;
        j = rand() % (SZ-i) + i;
        t = perm[j];
        perm[j] = perm[i];
        perm[i] = t;
    }
    return perm;
}

void RandomHV(int *randomHV) {
    int i;
    int* perm = randPerm(DIMENSION);
    for(i =0; i < DIMENSION/2; i++) randomHV[perm[i]] = 1;
    for(i=DIMENSION/2; i < DIMENSION; i++) randomHV[perm[i]] = -1;
    free(perm);
}

int* check_ItemMemory(int** itemMemory, char key){
    if(itemMemory[key] == NULL) {
        itemMemory[key]  = (int *) malloc(DIMENSION * sizeof(int));
        RandomHV(itemMemory[key]);
    }
    return itemMemory[key];
}

void binary_HV(long *v, int size){
    int i;
    int threshold = 0;
    for(i =0; i<size; i++) {
        if (v[i]>threshold) v[i] = 1;
        else v[i] = -1;
    }
}

void permute_val(int **in, int xdim, int ydim) {
    int i;
    int j;

    int *temp = in[xdim-1];
    for (i=xdim-1; i>0; i--) in[i] = in[i-1];
    in[0] = temp;

    for(i=0; i<xdim; i++) {
        int tempj = in[i][ydim-1];
        for(j=ydim-1; j>0; j--) in[i][j] = in[i][j-1];
        in[i][0] = tempj;
    }
}

int computeHvForWord(char* tempWord, int wordLength, int ** itemMemory, long *sumHV ){
    int isIMChanged = 0;
    int *nGrams = NULL;
    int **tempBlock = NULL;
    tempBlock = (int **) malloc(wordLength * sizeof(int *));
    for(int i=0; i<wordLength; i++) {
        tempBlock[i] = (int *) calloc(DIMENSION, sizeof(int));
    }

    char character;
    nGrams = (int *) calloc(DIMENSION, sizeof(int));

    for(int wordHead=0; wordHead< wordLength; wordHead++){
        character = tempWord[wordHead];
        permute_val(tempBlock, wordLength, DIMENSION);
        if(itemMemory[character] == NULL)
            isIMChanged = 1;

        int * tempHv = check_ItemMemory(itemMemory, character);
        memcpy(tempBlock[0], tempHv, DIMENSION * sizeof(int));

        memcpy(nGrams, tempBlock[0], DIMENSION * sizeof(int));
        for(int i = 1; i<wordLength; i++) {
            for(int j=0; j<DIMENSION; j++) {
                nGrams[j] *= tempBlock[i][j];
                sumHV[j] += nGrams[j];
            }
        }

    }
    for(int i=0; i<wordLength; i++) free(tempBlock[i]);
    free(nGrams);
    free(tempBlock);
    memset(tempWord,'\0',strlen(tempWord));

    return isIMChanged;
}

int compute_topicHV(char *fileBuffer, long fileLength, int ** itemMemory, long *sumHV) {

    int isIMChanged = 0;
    int wordLength = 0;
    char tempWord[15];

    for(int head=0; head < fileLength; head++) {
        if (fileBuffer[head] != ' ' && wordLength < 14) {
            tempWord[wordLength] = fileBuffer[head];
            wordLength++;
            continue;
        } else if(wordLength > 1 && wordLength < 14) {
            computeHvForWord(tempWord, wordLength, itemMemory, sumHV);
        }
        for(int i=0; i < 15; i++) tempWord[i] = '\0';
        wordLength = 0;
    }

    return isIMChanged;
}

long load_file(char const* path, char* buffer) {
    long length = -1;
    FILE * f = fopen (path, "rb");
    if (f){
        fseek (f, 0, SEEK_END);
        length = ftell (f);
        fseek (f, 0, SEEK_SET);
        fread (buffer, sizeof(char), length, f);
        fclose (f);
    }
    return length;
}

void buildtext(long **associativeMemory, int **itemMemory) {
    int i;
    char *path= (char*)calloc(150, sizeof(char));
    char *buffer = (char*)calloc(3*1024*1024, sizeof(char));
    long length;

    for(i=0; i<NumOfClass; i++) {
        path[0] = '\0';
        strcat(path, TRAIN_FILE_PATH);
        strcat(path, ClassLabel[i]);
        strcat(path, ".txt");
        length = load_file(path, buffer);
        printf("Training file %s \n", path);
        associativeMemory[i] = (long *) calloc(DIMENSION, sizeof(long));
        compute_topicHV(buffer,(length),itemMemory, associativeMemory[i]);
    }
    free(path);
    free(buffer);
}

void test_files(long **langAM, int **itemMemory ) {
    int total = 0;
    int correct = 0;
    float accuracy = 0;

    char actLabel[3];
    char preLang[3];
    int i;
    char *path = (char*)calloc(150, sizeof(char));
    char *buffer = (char*)calloc(10*1024, sizeof(char));
    long* textHV = (long *) malloc(DIMENSION * sizeof(long));

    tinydir_dir dir;
    tinydir_open_sorted(&dir, TEST_FILE_PATH);
    double angle;
    double maxAngle;
    int k;
    actLabel[2] = '\0';
    preLang[2] = '\0';
    printf("dir.n_files = %d \n", dir.n_files);

    for (k = 0; k<dir.n_files; k++) {
        tinydir_file file;
        tinydir_readfile_n(&dir, &file, k);
        if (strstr(file.name, ".txt") == NULL) continue;
        for(i=0; i<2; i++)
            actLabel[i] = file.name[i];
        path[0] = '\0';
        strcpy(path, TEST_FILE_PATH);
        strcat(path, file.name);
        long length = load_file(path, buffer);
        printf("Testing file %s \n", path);

        int isChanged = compute_topicHV(buffer, length, itemMemory, textHV);
        binary_HV(textHV, DIMENSION);
        if(isChanged) {
            printf("Error in File \n");
            exit(0);
        }

        maxAngle = 0;
        for(i=0; i<NumOfClass; i++){
            angle = fabs(cosAngle_val(langAM[i], textHV, DIMENSION));
             if(angle > maxAngle) {
                maxAngle = angle;
                strcpy(preLang, ClassMap[i]);
            }
        }

        if(strcmp(preLang, actLabel) == 0)
            correct += 1;
        else
            printf("No Match: %s --> %s \n", actLabel, preLang);

        total = total + 1;
    }
    free(path);
    free(textHV);
    free(buffer);
    tinydir_close(&dir);
    accuracy = correct/(float)total * 100;
    printf("Accuracy = %f percent\n", accuracy);
}

int main(int argc, char **argv)
{

    long **associativeMemory = (long **) malloc(NumOfClass * sizeof(long *));
    int **characterMemory= (int **) malloc(CHARACTERS * sizeof(int *));

    for (int i=0; i<CHARACTERS; i++) characterMemory[i] = NULL;

    buildtext(associativeMemory, characterMemory);

    for(int x=0; x< NumOfClass; x++){
        for(int y=0; y< DIMENSION; y++){
            printf("%5d",associativeMemory[x][y]);
        }
        printf("\n");
    }

    test_files(associativeMemory, characterMemory);
    free(characterMemory);
    free(associativeMemory);
    return 0;
}
