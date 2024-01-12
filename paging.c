#include <stdio.h>
#include <stdlib.h>
void main()
{
  int psize,n,la;
  printf("Input Page size : ");
  scanf("%d",&psize);
  printf("Input Number of pages : ");
  scanf("%d",&n);
  int frame[n];
  int page[n];
  for(int i=0;i<n;i++)
  {
    page[i]=i;
    int a=rand();
    frame[i]=a%100;
    }
  do  
  {
  	printf("\n\nInput Logical address (<%d)(-1 to exit): ",(psize*n));
  	scanf("%d",&la);
  
	  if(la==-1)
      {
	  printf("\n-----End of Execution------");
	  break;
         }
  
	  if(la>=psize*n)
      {
	  printf("\nEnter properly");
	  continue;
	  }
  int fno=la/psize;
  int off=la%psize;
  int phy=frame[fno]*psize+off;
  printf("The corresponding physical address = %d\n",phy);
  printf("The corresponding frame no = %d\n",fno);
  printf("The offset= %d\n",off);
  }while(1);
  
  }