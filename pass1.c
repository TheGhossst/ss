#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_HASH 300

struct symtab{
    char label[20];
    int address;
    unsigned int hashKey;
    struct symtab* next;
};
struct symtab* Sym[MAX_HASH];
unsigned int generateHashKey(char *s){
    unsigned int hashVal=0,hashkey;
    char str[20];
    strcpy(str,s);
    while(*s)   hashVal=(hashVal << 5) +(*s++);
    hashkey=hashVal % MAX_HASH;
    printf("\nSymbol %s generated hashKey %d",str,hashkey);
    return hashkey;
}

void insert(char symbol[20],int address){
    struct symtab* new=(struct symtab*)malloc(sizeof(struct symtab));
    strcpy(new->label,symbol);
    new->address=address;
    unsigned int hashkey=generateHashKey(symbol);
    new->hashKey=hashkey;
    new->next=Sym[hashkey];
    Sym[hashkey]=new;

    FILE *fsym;
    fsym=fopen("symtab.txt","a");
    fprintf(fsym,"%s\t%x\t%d\n",new->label,new->address,new->hashKey);
    fclose(fsym);
}
void pass1(){
    FILE *fin,*fout,*fop,*fsym,*flen;
    fin=fopen("input.txt","r");
    fout=fopen("intermediate.txt","w");
    flen=fopen("length.txt","w");

    //printf("\nCheckpoint");
    char symbol[20],mnemonic[20],operand[20],mne[20],code[20];
    int locctr,startAddress;

    fscanf(fin,"%s\t%s\t%x",symbol,mnemonic,&startAddress);
    printf("%s\t%s\t%x",symbol,mnemonic,startAddress);
    if(strcmp(mnemonic,"START") == 0){
        locctr = startAddress;
        fprintf(fout,"%s\t%s\t%x\n",symbol,mnemonic,startAddress);
        fscanf(fin,"%s\t%s\t%s",symbol,mnemonic,operand);
        printf("\n%s\t%s\t%s",symbol,mnemonic,operand);
    }
    else locctr =0;
   // printf("\nCheckpoint");
    while(strcmp(mnemonic,"END") != 0){
        fprintf(fout,"%x\t%s\t%s\t%s\n",locctr,symbol,mnemonic,operand);
        printf("\n%s",symbol);
        if(strcmp(symbol,"**") != 0){
            printf("\nEntering %s into symtab",symbol);
            insert(symbol,locctr);
        }
        fop=fopen("optab.txt","r");
        fscanf(fop,"%s\t%s",mne,code);
        while(strcmp(mne,"END") !=0){
            //printf("\n%s\t%s",mne,code);
            if(strcmp(mnemonic,mne) == 0){
                printf("\n%s is valid",mnemonic);
                locctr+=3;
                break;
            }
            fscanf(fop,"%s\t%s",mne,code);
        }
        fclose(fop);
        if(strcmp(mnemonic,"WORD") == 0)    locctr+=3;
        else if(strcmp(mnemonic,"RESW") == 0) locctr+=3* atoi(operand);
        else if(strcmp(mnemonic,"BYTE") == 0) {
            if(operand[0] == 'C')   locctr+=strlen(operand) -3;
            else if(operand[0] == 'X')   locctr += strlen(operand)/2  -3;
        }
        else if(strcmp(mnemonic,"RESB") == 0)   locctr+=atoi(operand);
        fscanf(fin,"%s\t%s\t%s",symbol,mnemonic,operand);
    }
    fprintf(fout,"%x\t%s\t%s\t%s\n",locctr,symbol,mnemonic,operand);
    fprintf(flen,"%x",locctr-startAddress);
    fclose(fin);
    fclose(fout);
    fclose(flen);
}
void main(){
    FILE *sym=fopen("symtab.txt","w");
    fclose(sym);
    for(int i=0;i<MAX_HASH;i++) Sym[i]=NULL;
    pass1();
}