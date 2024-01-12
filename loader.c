#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char input[10], label[10], ch1, ch2;
int addr, w = 0, start, ptaddr, l, length = 0, end, count = 0, k, taddr, address, i = 0, j = 0;
FILE *f1, *f2;

void format() {
    FILE *fin, *fout;
    fin = fopen("input.txt", "r");
    fout = fopen("input1.txt", "w");
    char c;
    while ((c = fgetc(fin)) != EOF) {
        if (c == '^') fputc(' ', fout);
        else fputc(c, fout);
    }
    fclose(fin);
    fclose(fout);
}

int main() {
    //int ad=16384;
    format();
    f1 = fopen("input1.txt", "r");
    f2 = fopen("output1.txt", "w");
    fscanf(f1, "%s", input);
    printf("\n.............................................................\n");
    printf("Memory Address\t\t\tContents");
    printf("\n..............................................................\n");
    while (strcmp(input, "E") != 0) {
        if (strcmp(input, "H") == 0) {
            fscanf(f1, "%s %x %x %s", label, &start, &end, input);
            address = start;
        } else if (strcmp(input, "T") == 0) {
            l = length;
            ptaddr = addr;
            fscanf(f1, "%x %x %s", &taddr, &length, input);
            addr = taddr;
            if (w == 0) {
                ptaddr = address;
                w = 1;
            }
            for (k = 0; k < (taddr - ptaddr); k++) {
                address = address + 1;
                if (address <= end) {
                    printf("%x\t\t : \t\txx\n", address);
                }
            }
            for (j = 0; j < strlen(input) && address <= end; j += 2) {
                printf("%x\t\t : \t\t%c%c\n", address, input[j], input[j + 1]);
                address += 1;
            }
            fscanf(f1, "%s", input);
        } else {
            for (j = 0; j < strlen(input) && address <= end; j += 2) {
                printf("%x\t\t : \t\t%c%c\n", address, input[j], input[j + 1]);
                address += 1;
            }
            fscanf(f1, "%s", input);
        }
    }
    fclose(f1);
    fclose(f2);
    return 0;
}
