#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPTAB_SIZE  32
#define SYMTAB_SIZE 32

#define EMPTY "-"

struct OpTab {
    char mnemonic[5];
    int opcode;
} optab[OPTAB_SIZE];

struct SymTab {
    char symbol[32];
    int address;

    int forward_ref[32][2]; // Forward Reference | Is_Indexed
    int forward_ref_count;
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

void InsertIntoSymTab(char *symbol, int address)
{
    static int count = 0;
    assert(count < SYMTAB_SIZE);

    strcpy(symtab[count].symbol, symbol);
    symtab[count].address = address;
    symtab[count].forward_ref_count = 0;

    count++;
}

int GetAddressFromSymTab(char *symbol)
{
    for (int i = 0; i < SYMTAB_SIZE; ++i) 
        if (strcmp(symtab[i].symbol, symbol) == 0) return symtab[i].address;
    return -2;
}

int ResolveAddressForSymbol(char *symbol, int address)
{
    for (int i = 0; i < SYMTAB_SIZE; ++i)
        if (strcmp(symtab[i].symbol, symbol) == 0) {
            symtab[i].address = address;
            return 0;
        }
    return -1;
}

int AddForwardReference(char *symbol, int locctr, int is_indexed)
{
  for (int i = 0; i < SYMTAB_SIZE; ++i) {
    if (strcmp(symtab[i].symbol, symbol) == 0) {
      symtab[i].forward_ref[symtab[i].forward_ref_count][0] = locctr;
      symtab[i].forward_ref[symtab[i].forward_ref_count][1] = is_indexed;
      symtab[i].forward_ref_count++;

      return 0;
    }
  }
  return -1;
}

void ResolvedForwardReferencesToTextRecord(FILE *obj_file)
{
    for (int i = 0; i < SYMTAB_SIZE; ++i) {
        if (symtab[i].address >= 0 && symtab[i].forward_ref_count > 0) {
          for (int j = 0; j < symtab[i].forward_ref_count; ++j)
            fprintf(obj_file, "T^%06x^04^%04x\n", symtab[i].forward_ref[j][0],
                                                  (symtab[i].forward_ref[j][1] << 15) | symtab[i].address);
          symtab[i].forward_ref_count = 0;
        }
    }
}

int main(void)
{
    int start_address, locctr, next_locctr;

    FILE *optab_file  = fopen(".optab", "r");
    FILE *input_file  = fopen("input.asm", "r");

    FILE *intermediate_file = fopen(".intermediate", "w");
    FILE *symtab_file       = fopen(".symtab", "w");
    FILE *obj_file          = fopen("output.obj", "w");

    FetchOpTab(optab_file);
    fclose(optab_file);

    char label[32], mnemonic[32], operand[32];

    fscanf(input_file, "%s %s %s", label, mnemonic, operand);

    if (strcmp(mnemonic, "START") == 0) {
        start_address = strtol(operand, NULL, 16);
        locctr = start_address;

        fprintf(obj_file, "H^%-6s^%06x^%06x\n", label, start_address, 0x6969);
        fprintf(intermediate_file, "-\t%s\t%s\t%s\n", label, mnemonic, operand);

        fscanf(input_file, "%s %s %s", label, mnemonic, operand);
    } 
    else {
        locctr = 0x0;
        fprintf(obj_file, "H^%-6s^%06x^%06x\n", "", locctr, 0x6969);
    }


    struct TextRecord text_record = {
        .start_address = locctr,
        .length = 0,
        .object_code[0] = '\0'
    };

    while (strcmp(mnemonic, "END") != 0) {
        next_locctr = locctr;
        fprintf(intermediate_file, "%x\t%s\t%s\t%s\n", locctr, label, mnemonic, operand);

        int objcode = 0;
        int objcode_length = 0;
        
        if (strcmp(label, EMPTY) != 0) {
          switch (GetAddressFromSymTab(label)) {
            case -1: ResolveAddressForSymbol(label, locctr); break;
            case -2: InsertIntoSymTab(label, locctr); break;
            default: printf("Error Flag: Duplicate Symbol\n");
          }
        }

        int opcode = GetOpCodeFromOpTab(mnemonic);
        if (opcode >= 0) {
            int is_indexed = 0;
            int operand_address = 0;
            char *symbol = NULL;

            if (strcmp(operand, "-") != 0) {
                if (strlen(operand) > 2 && strcmp(operand + strlen(operand) - 2, ",X") == 0) {
                    is_indexed = 1;
                    symbol = strtok(operand, ",");
                }
                else symbol = operand;
                
                operand_address = GetAddressFromSymTab(symbol);

                if (operand_address == -1) {
                  AddForwardReference(symbol, locctr + 1, is_indexed);
                  operand_address = 0x0;
                }
                else if (operand_address == -2) {
                  InsertIntoSymTab(symbol, -1);
                  AddForwardReference(symbol, locctr + 1, is_indexed);
                  operand_address = 0x0;
                }
            }
            objcode = (opcode << 16) | (is_indexed << 15) | operand_address;
            objcode_length = 6; 
            next_locctr += 3;
        }
        else if (strcmp(mnemonic, "BYTE") == 0) {
            if (operand[0] == 'C') {
                for (unsigned i = 2; i < strlen(operand); ++i) {
                    objcode = (objcode << 8) | operand[i];
                    objcode_length += 2;
                }
                next_locctr += strlen(operand) - 3;
            }
            else if (operand[0] == 'X') {
                objcode = strtol(strtok(operand + 2, "'"), NULL, 16);
                objcode_length = strlen(operand) - 3 + 1; // +1 since X'F1' --> X'F1 by the strtok() above

                next_locctr += (int) objcode_length/2;
            }
        }
        else if (strcmp(mnemonic, "WORD") == 0) {
            objcode = strtol(operand, NULL, 16);
            objcode_length = 6;

            next_locctr += 3;
        }
        
        else if (strcmp(mnemonic, "RESW") == 0) {
            locctr += 3*atoi(operand);
            fscanf(input_file, "%s %s %s", label, mnemonic, operand);
            continue;
        }
        else if (strcmp(mnemonic, "RESB") == 0) {
            locctr += atoi(operand);
            fscanf(input_file, "%s %s %s", label, mnemonic, operand);
            continue;
        }
        if (locctr + objcode_length/2 - 1 > text_record.start_address + 60/2 - 1) {
            fprintf(obj_file, "T^%06x^%02x%s\n", text_record.start_address, 
                                                 text_record.length,
                                                 text_record.object_code);
            
            ResolvedForwardReferencesToTextRecord(obj_file);
            
            text_record = (struct TextRecord) {
                .start_address = locctr,
                .length = objcode_length/2,
                .object_code[0] = '\0'
            };
            sprintf(text_record.object_code + strlen(text_record.object_code), "^%0*x", objcode_length, objcode);

            fscanf(input_file, "%s %s %s", label, mnemonic, operand);
            locctr = next_locctr;
            continue;
        }

        sprintf(text_record.object_code + strlen(text_record.object_code), "^%0*x", objcode_length, objcode);
        text_record.length += objcode_length/2;

        fscanf(input_file, "%s %s %s", label, mnemonic, operand);
        locctr = next_locctr;
    }
    fprintf(intermediate_file, "%x\t%s\t%s\t%s\n", locctr, label, mnemonic, operand);

    fprintf(obj_file, "T^%06x^%02x%s\n", text_record.start_address, 
                                         text_record.length,
                                         text_record.object_code);

    ResolvedForwardReferencesToTextRecord(obj_file);

    fprintf(obj_file, "E^%06x\n", GetAddressFromSymTab(operand));

    fclose(input_file);
    
    fseek(obj_file, 16, SEEK_SET);
    fprintf(obj_file, "%06x", locctr - start_address);

    fclose(obj_file);

    for (int i = 0; i < SYMTAB_SIZE && strcmp(symtab[i].symbol, "") != 0; ++i)
      fprintf(symtab_file, "%s\t%x\n", symtab[i].symbol, symtab[i].address);

    fclose(symtab_file);
    printf("Generated contents in ./output.obj, ./.symtab, ./.intermediate\n");

    return 0;
}
