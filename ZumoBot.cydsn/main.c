#include <project.h>
#include <stdio.h>
#include "Motor.h"
#include "Ultra.h"
#include "Nunchuk.h"
#include "Reflectance.h"
#include "I2C_made.h"
#include "Gyro.h"
#include "Accel_magnet.h"
#include "IR.h"
#include "Ambient.h"
#include "Beep.h"

int rread(void);


void waitForSignal() {
    while (IR_receiver_Read() == 1)
    {
        IR_receiver_Read();
    } 
}

//battery level//
int main()
{
    CyGlobalIntEnable; 
    UART_1_Start();
    ADC_Battery_Start();        
    int16 adcresult =0;
    float volts = 0.0;
    motor_start();
    motor_forward(0,0);
    
    struct sensors_ ref;
    struct sensors_ dig;

    printf("\nBoot\n");

    //BatteryLed_Write(1); // Switch led on 
    BatteryLed_Write(0); // Switch led off 
    uint8 button;
    button = SW1_Read(); // read SW1 on pSoC board
    
    while (button == 1) {
        button = SW1_Read();
        CyDelay(50);
       
        BatteryLed_Write(0);
        ADC_Battery_StartConvert();
        if(ADC_Battery_IsEndConversion(ADC_Battery_WAIT_FOR_RESULT)) {   // wait for get ADC converted value
            adcresult = ADC_Battery_GetResult16();
            volts = (float)adcresult / 4095 * 5 * 1.5;
        
            printf("%d %f\r\n",adcresult, volts);
        }
        CyDelay(500);
        
        if (volts < 4) {
            BatteryLed_Write(1);
            CyDelay(500);
        }   
    }
    
    //Otetaan anturiarvot talteen kalibrointia varten
    
    sensor_isr_StartEx(sensor_isr_handler);      
    reflectance_start();
    IR_led_Write(1);
    CyDelay(10);
    reflectance_read(&ref);
    int vasenUlkoValkoinen = ref.l3;
    int vasenValkoinen = ref.l1;
    int oikeaValkoinen = ref.r1;
    int oikeaUlkoValkoinen = ref.r3;
      
    Beep(150, 150);
    
    //Odottaa infrapunasignaalia
    
    waitForSignal();
    
    reflectance_read(&dig);
    int leftI = 50;
    int rightI = 42;
    int leftMinI = 10;
    int rightMinI = 10;
    
    while (ref.l3 < 16000 && ref.r3 < 16000) {   
        reflectance_read(&ref); 
        float vasenSensori = (float)(ref.l1 - vasenValkoinen)/(23999 - vasenValkoinen);
        float oikeaSensori = (float)(ref.r1 - oikeaValkoinen)/(23999 - oikeaValkoinen);  
        motor_turn(leftI,rightI,1);                 
        
        //Aloitusviivalle ajo
        
        //Suoraan
        if (vasenSensori > 0.43 && oikeaSensori > 0.44) {
            leftI = 50;
            rightI = 42;
        }
        
        //Erittäin loiva käännös
        else if (vasenSensori < 0.26 && oikeaSensori > 0.26) {
            rightI *= 0.91;
            if (rightI < rightMinI) {
                rightI = rightMinI;              
            }
        } else if (vasenSensori > 0.31 && oikeaSensori < 0.31) {
            leftI *= 0.91;
            if (leftI < leftMinI) {
                leftI = leftMinI;
            }
        } else {
            printf("xd");
        }
        reflectance_read(&dig);
        
    }
    motor_turn(0,0,1);
    
    //Odotetaan infrapunasignaalia
    
    waitForSignal();

    //Ensimmäisen viivan ylitys
    
    motor_turn(255,247,150);
 
    for(;;)
    {
        //Muuttujat 
        
        int left = 255;
        int right = 247;
        int leftMin = 80;
        int rightMin = 80;
        int leftSoftMin = 170;
        int rightSoftMin = 170;
        int sharpTurnDirection = 0;
        int rememberDirection = 0;
        int finish = 0;
        
        while (finish != 2)
        {   
            reflectance_read(&ref);
            
            //Muutetaan analogiarvot 0:n ja 1:n väliarvoihin
            float vasenSensori = (float)(ref.l1 - vasenValkoinen)/(23999 - vasenValkoinen);
            float oikeaSensori = (float)(ref.r1 - oikeaValkoinen)/(23999 - oikeaValkoinen);
            float vasenUlkoSensori = (float)(ref.l3 - vasenUlkoValkoinen)/(23999 - vasenUlkoValkoinen);          
            float oikeaUlkoSensori = (float)(ref.r3 - oikeaUlkoValkoinen)/(23999 - oikeaUlkoValkoinen);
            printf("%f ja %f\n", vasenSensori, oikeaSensori);
            
            
            //Ajetaan millisekunti
    
            motor_sharpTurn(left,right,1,sharpTurnDirection); 
            
            //sharpTurnDirection 1 on vasen 2 oikea
                  
            
            //Suoraan
            if (vasenSensori > 0.43 && oikeaSensori > 0.44) {
                left = 255;
                right = 247;
                sharpTurnDirection = 0;
            }
            
            //Loiva käännös
            else if (vasenSensori < 0.26 && oikeaSensori > 0.26) {
                right *= 0.91;
                sharpTurnDirection = 0;
                if (right < rightMin) {
                    right = rightMin;              
                }
            } else if (vasenSensori > 0.31 && oikeaSensori < 0.31) {
                left *= 0.91;
                sharpTurnDirection = 0;
                if (left < leftMin) {
                    left = leftMin;
                }   
            //Jyrkkä käännös
            } else if (vasenSensori < 0.43 && oikeaSensori > 0.44) {            
                sharpTurnDirection = 2;
                rememberDirection = 2;
                left = 247;
                right = 110;
                
            } else if (vasenSensori > 0.43 && oikeaSensori < 0.44) {
                sharpTurnDirection = 1;
                rememberDirection = 1;
                left = 110;
                right = 247;
                
            //Erittäin loiva käännös
            } else if (vasenSensori < 0.43 && oikeaSensori > 0.44) {
                right *= 0.91;
                sharpTurnDirection = 0;
                if (right < rightSoftMin) {
                    right = rightSoftMin;              
                }          
            } else if (vasenSensori > 0.43 && oikeaSensori < 0.44) {
                left *= 0.91;
                sharpTurnDirection = 0;
                if (left < leftSoftMin) {
                    left = leftSoftMin;
                }       
            }   
            
            //Ulosajo ja takaisin kääntyminen rememberDirectionin avulla
             else if (vasenSensori < 0.14 && oikeaSensori < 0.14) {
                if (rememberDirection == 1) {
                    sharpTurnDirection = 1;
                    left = 110;
                } else if (rememberDirection == 2) {
                    sharpTurnDirection = 2;
                    right = 110;
                } else {
                    sharpTurnDirection = 0;
                }
            }      
            
            //Pysähtyminen       
            while (vasenUlkoSensori > 0.44 && oikeaUlkoSensori > 0.44 && finish == 1) {
               motor_stop();
               finish = 2;
               break;
            }
            while (vasenUlkoSensori > 0.44 && oikeaUlkoSensori > 0.44) {
               motor_turn(255,247,50);
               finish = 1;
               break;
            }        
            
        }
    }
    
}

#if 0
int rread(void)
{
    SC0_SetDriveMode(PIN_DM_STRONG);
    SC0_Write(1);
    CyDelayUs(10);
    SC0_SetDriveMode(PIN_DM_DIG_HIZ);
    Timer_1_Start();
    uint16_t start = Timer_1_ReadCounter();
    uint16_t end = 0;
    while(!(Timer_1_ReadStatusRegister() & Timer_1_STATUS_TC)) {
        if(SC0_Read() == 0 && end == 0) {
            end = Timer_1_ReadCounter();
        }
    }
    Timer_1_Stop();
    
    return (start - end);
}
#endif

/* Don't remove the functions below */
int _write(int file, char *ptr, int len)
{
    (void)file; /* Parameter is not used, suppress unused argument warning */
	int n;
	for(n = 0; n < len; n++) {
        if(*ptr == '\n') UART_1_PutChar('\r');
		UART_1_PutChar(*ptr++);
	}
	return len;
}

int _read (int file, char *ptr, int count)
{
    int chs = 0;
    char ch;
 
    (void)file; /* Parameter is not used, suppress unused argument warning */
    while(count > 0) {
        ch = UART_1_GetChar();
        if(ch != 0) {
            UART_1_PutChar(ch);
            chs++;
            if(ch == '\r') {
                ch = '\n';
                UART_1_PutChar(ch);
            }
            *ptr++ = ch;
            count--;
            if(ch == '\n') break;
        }
    }
    return chs;
}
/* [] END OF FILE */
