#ifndef BSW_IO_BUZZER_H_
#define BSW_IO_BUZZER_H_

#include <stdbool.h>

void Buzzer_Init (void);
bool Buzzer_SetFrequency (int f);
void Buzzer_On (void);
void Buzzer_Off (void);
void Buzzer_Buzz (void);

#endif /* BSW_IO_BUZZER_H_ */
