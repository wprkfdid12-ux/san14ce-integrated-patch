#define E __declspec(dllexport)
E volatile int last=-7; E volatile int calls=0;
#ifdef NAME
E void NAME(int on){last=on;calls++;}
#endif
#ifdef POP
E volatile int bonus=-7,loss=-7; E void SAN14_SetPopulationBonus(int b){bonus=b;} E void SAN14_SetWarPopulationLoss(int l){loss=l;} E unsigned SAN14_ApplyPopulation(void){calls++;return 1;}
#endif
int DllMain(void*a,unsigned r,void*b){return 1;}
