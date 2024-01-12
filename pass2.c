#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPTAB_SIZE  32
#define SYMTAB_SIZE 32

struct OpTab {
    char mnemonic[5];
    int opcode;
} optab[OPTAB_SIZE];

struct SymTab {
    char symbol[32];
    int address;
} symtab[SYMTAB_SIZE];

struct TextRecord {
    int start_address;
    int length;
    char object_code[128];
};

void FetchOpTab(FILE *optab_file)
{
    char mnemonic[5];
    int opcode;

    int i = 0;
    while (fscanf(optab_file, "%s %x", mnemonic, &opcode) != EOF) {
        strcpy(optab[i].mnemonic, mnemonic);
        optab[i].opcode = opcode;
        i++;
    }
}

int GetOpCodeFromOpTab(char *mnemonic)
{
    for (int i = 0; i < OPTAB_SIZE; ++i)
        if (strcmp(optab[i].mnemonic, mnemonic) == 0) return optab[i].opcode;
    return -1;
}

void FetchSymTab(FILE *symtab_file)
{
    char symbol[32];
    int address;

    int i = 0;
    while (fscanf(symtab_file, "%s %x", symbol, &address) != EOF) {
        strcpy(symtab[i].symbol, symbol);
        symtab[i].address = address;
        i++;
    }
}

int GetAddressFromSymTab(char *symbol)
{
    for (int i = 0; i < SYMTAB_SIZE; ++i) {
        if (strcmp(symtab[i].symbol, symbol) == 0) return symtab[i].address;
    }
    return -1;
}

int main(void)
{
    int start_address, length, address;

    FILE *listing_file      = fopen("listing.txt", "w");
    FILE *optab_file        = fopen("optab.txt", "r");
    FILE *symtab_file       = fopen("symtab.txt", "r");
    FILE *intermediate_file = fopen("intermediate.txt", "r");
    FILE *obj_file          = fopen("output.obj", "w");
    FILE *length_file       = fopen("length.txt", "r");

    FetchOpTab(optab_file);
    fclose(optab_file);

    FetchSymTab(symtab_file);
    fclose(symtab_file);
    
    fscanf(length_file, "%d", &length);
    fclose(length_file);

    char label[32], mnemonic[32], operand[32];

    fscanf(intermediate_file, "%*s %s %s %s", label, mnemonic, operand); //%*s mean discard that value dont store anywhere
    printf("%s\t%s\t%s",label,mnemonic,operand);

    if (strcmp(mnemonic, "START") == 0) {
        fprintf(listing_file, "\t%s\t%s\t%s\n", label, mnemonic, operand);
        //printf("%s",operand);
        start_address = strtol(operand, NULL, 16);  /*COnvert string to hex*/
        //printf("\t%x",start_address);
        fprintf(obj_file, "H^%-6s^%06x^%06x\n", label, start_address, length);  //header record
    }

    fscanf(intermediate_file, "%x %s %s %s", &address, label, mnemonic, operand);

    struct TextRecord text_record = {
        .start_address = address,
        .length = 0,
        .object_code[0] = '\0'
    };

    while (strcmp(mnemonic, "END") != 0) {
        int objcode = 0;
        int objcode_length = 0;

        int opcode = GetOpCodeFromOpTab(mnemonic);
        printf("\nFetched opcode for %s %d",mnemonic,opcode);
        if (opcode >= 0) {
            int is_indexed = 0;
            int operand_address = 0;
            char *symbol;

            if (strcmp(operand, "-") != 0) {
                if (strlen(operand) > 2 && strcmp(operand + strlen(operand) - 2, ",X") == 0) {
                    is_indexed = 1;
                    //printf("%s",symbol);
                    symbol = strtok(operand, ",");
                    //printf("\t%s",symbol);
                }
                else symbol = operand;
                
                operand_address = GetAddressFromSymTab(symbol);
                if (operand_address < 0) {
                    printf("Error: Undefined Symbol `%s`", symbol);
                    operand_address = 0;
                }
            }
            printf("\n%d",operand_address);
            objcode = (opcode << 16) | (is_indexed << 15) | operand_address;
            printf("\nObject code %x",objcode);   //Left shit opcode by 16bits  bitwise or if is_indexed is 1 then left shift by 15 bits and then bitwise or with operand_Address
            objcode_length = 6; 
        }
        else if (strcmp(mnemonic, "BYTE") == 0) {
            if (operand[0] == 'C') {
                for (unsigned i = 2; i < strlen(operand); ++i) {  //2 cuz skipping C and '
                    objcode = (objcode << 8) | operand[i];   //C'F1'    First iteration objcode = ASCII of F,second iteration objcode is concat of ASCII of F and 1
                    objcode_length += 2;                       //4831
                }
            }
            else if (operand[0] == 'X') {
                objcode = strtol(strtok(operand + 2, "'"), NULL, 16);
                objcode_length = strlen(operand) - 3;
            }
        }
        else if (strcmp(mnemonic, "WORD") == 0) {
            objcode = strtol(operand, NULL, 16);
            objcode_length = 6;
        }
        
        if (strcmp(mnemonic, "RESW") == 0 || strcmp(mnemonic, "RESB") == 0) {
            fprintf(listing_file, "%x\t%s\t%s\t%s\n", address, label, mnemonic, operand);
            fscanf(intermediate_file, "%x %s %s %s", &address, label, mnemonic, operand);
            continue;
        }
        else {
            fprintf(listing_file, "%x\t%s\t%s\t%s\t%06x\n", address, label, mnemonic, operand, objcode);   //IF not RESW or RESB print to file
            //printf("%x\t%s\t%s\t%s\t%06x\n", address, label, mnemonic, operand, objcode);
        }

        if (address + objcode_length/2 - 1 > text_record.start_address + 60/2 - 1) {   //checking if max lenght of text record is reached
            fprintf(obj_file, "T^%06x^%02x%s\n", text_record.start_address,            //if true create a new text record
                                                   text_record.length/2 ,
                                                   text_record.object_code);
            
            text_record = (struct TextRecord) {
                .start_address = address,
                .length = objcode_length,
                .object_code[0] = '\0'
            };
            sprintf(text_record.object_code + strlen(text_record.object_code), "^%0*x", objcode_length, objcode);

            fscanf(intermediate_file, "%x %s %s %s", &address, label, mnemonic, operand);
            continue;
        }
        else {
            sprintf(text_record.object_code + strlen(text_record.object_code), "^%0*x", objcode_length, objcode);
            text_record.length += objcode_length;        
        }

        fscanf(intermediate_file, "%x %s %s %s", &address, label, mnemonic, operand);
    }

    fprintf(listing_file, "\t%s\t%s\t%s\n", label, mnemonic, operand);

    fprintf(obj_file, "T^%06x^%02x%s\n", text_record.start_address, 
                                           text_record.length/2,
                                           text_record.object_code);
    fprintf(obj_file, "E^%06x\n", start_address);

    fclose(intermediate_file);
    fclose(listing_file);
    fclose(obj_file);

    printf("Generated contents in ./output.obj, ./listing.txt\n");

    return 0;
}