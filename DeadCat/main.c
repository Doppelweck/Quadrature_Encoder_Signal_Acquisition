/*
 *  ======= main ========
 *
 *  Created on: 20.11.2017
 *  Author:     Roman Grewenig M.Sc.
 *  Company:    Hochschule Trier
 *
 *  Target: TM4C123GH6PM
 *  JTAG Chain: Stellaris In-Circuit Debug Interface (on-board)
 *
 *  Compiler: TI-ARM v17.9
 *  IDE: CCS v6.2.0.00050
 *
 *  Description:
 *  ------------
 *  Minimal example for the EK-TM4C123GXL Launch Pad.
 *  Use as a starting point for your own projects.
 *  This example blinks all three colours of the RGB LED using a state machine and a
 *  cyclic task. The current state is transmitted every time step (CAN-ID 0x1).
 *
 *  Important note: Check Project > Properties > Resource > Linked Resources for relative
 *  file paths.
 *
 *  Needs TI-RTOS (an the included TivaWare) to be installed from:
 *  http://software-dl.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/tirtos/2_16_01_14/exports/tirtos_tivac_setupwin32_2_16_01_14.exe
 */

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>

/* Standard IO stream etc. */
#include <stdbool.h>                // Defines for standard boolean representation.
#include <stdio.h>              // IO-stream for console display etc.

/* Drivers etc. */
#include "inc/hw_ints.h"            // Macros defining the interrupts.
#include "inc/hw_memmap.h"      // Macros defining the memory map of the device.
#include "inc/hw_sysctl.h"      // Macros for usage with SysCtl.
#include "inc/hw_types.h"       // Macros for common types.
#include "inc/hw_can.h"         // Macros for usage with the CAN controller peripheral.
#include "inc/hw_qei.h"
#include "inc/hw_timer.h"
//#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"        // Macros to debug the driverlib.
#include "driverlib/gpio.h"     // Macros defining the GPIO assignments.
#include "driverlib/pin_map.h"  // Macros defining the pin map of the device.
#include "driverlib/sysctl.h"   // Prototypes for the SysCtl driver.
#include "driverlib/can.h"      // Defines and macros for the CAN controller.
#include "driverlib/qei.h"      // Defines and macros for the QEI Module.
#include "driverlib/timer.h"
//#include "driverlib/interrupt.h"


/* Global variables */
// Put your global variables here.
tCANMsgObject MsgObjectTx;
uint8_t pui8TxBuffer[8];
int32_t pui32TxBuffer[2];


#define PPI  180 // Pulses per Inch
#define PPR  5000 // Pulses per Revolutions
#define EPP  4 //Edges per period. Both Signals
#define MIN_QEI_VALUE 340//283//566


double dblVelocityPeriod = 0.001;      // 1ms
/* Multiplier for QEIPos to um
 * MM_PER_INCH      25.4 mm/Inch
 * -------------- = ---------------- = 35.2777 um/Edge
 * EDGES_PER_INCH   4*180 Edges/Inch
 */
double dblMultiplierPos = (double)25400/(EPP*PPI);
/* Multiplier for QEIVel to 1um/s:
 *
 * MM_PER_INCH           25.4 mm/Inch             25400 mm/s    35.277... mm/s   35,277.7... um/s
 * ------------------- = ---------------------- = ----------- = -------------- = ----------------
 * dt * EDGES_PER_INCH   1ms * 4*180 Edges/Inch   4*180 Edges   Edges            Edges
 */
double dblMultiplierVelQEI = (double)25400/(EPP*PPI)*1000;
/* Multiplier for timer based velocity
 *                            F_CLK * dblMultiplierPos*EPP   dblMultiplierVelTimer
 * v = f * dblMultiplierPos = ---------------------------- = ---------------------
 *                                 Timer_period               Timer_period
 *
 * dblMultiplierVelTimer = F_CLK * dblMultiplierPos * EPP
 */
double dblMultiplierVelTimer;

/* Multiplier for QEIAngle to mdeg
 * 360,000 mdeg/revolution   360,000 mdeg/rev
 * ----------------------- = ---------------- = 18 mdeg/Edge
 * EDGES_PER_REV             4*5000 Edges/rev
 */
double dblMultiplierAngle = 18.0;
/* Multiplier QEIAngleVelocity to 0.001 deg/s
 * 360 deg/Revolution   360 deg/rev              90 deg/s      18 deg/s   18000 mdeg/s
 * ------------------ = ---------------------- = ----------- = -------- = ------------
 * dt * EDGES_PER_REV   1ms * 4*5000 Edges/rev   5 Edges       Edges      Edges
 */
double dblMultiplierOmegaQEI = (double)18000;
#define MILLI_DEG_PER_SEC_PER_EDGE 18000
/* Multiplier for timer based angular velocity (omega)
 *                                   F_CLK * dblMultiplierAngle * EPP   dblMultiplierOmegaTimer
 * omega = f * dblMultiplierAngle = --------------------------------- = -----------------------
 *                                        Timer_period                  Timer_period
 *
 * dblMultiplierOmegaTimer = F_CLK * dblMultiplierAngle * EPP
 */
double dblMultiplierOmegaTimer;

/*
 * Define measured values as global variables
 */
double Velocity = 0;    // [um/s]
double Position = 0;    // [um]
double Angle = 0;       // [milli degree]
double AngleSpeed = 0;  // [milli degree/s]




/* Global timer measurement variables */
uint32_t TmrPosOld =0;
uint32_t TmrPosNew =0;
uint32_t TmrPosDelta = 0;
int8_t TmrPosFlag = 0;

uint32_t TmrAngOld =0;
uint32_t TmrAngNew =0;
uint32_t TmrAngDelta = 0;
int8_t TmrAngFlag = 0;


/* Function prototypes */
void Init_Clock(void);
void Init_GPIO(void);
void Init_CAN(void);
void Init_QEI(void);
void Init_WTIMER3(void);
void Init_WTIMER2(void);
<<<<<<< HEAD
void Init_TIMER2(void);
void Init_Timer1CCP(void);

void inputInt(void);
=======
>>>>>>> ff1668546cfb28239c6c3c8972f79d14cf1fbd73
void Init_UART(void);

void ISR_GPIOC(void);
void ISR_GPIOD(void);
void ISR_WTIMER3(void);
void ISR_WTIMER2(void);

/*
 * ======== main ========
 */
void main(void)
  {
    printf("BIOS starting...\n");

    // Initialize the main clock/oscillator.
    Init_Clock();

    // Initialize your peripherals (GPIOs etc.).
    Init_GPIO();

    // Initialize the CAN peripheral.
    Init_CAN();

    // Initialize the QEI peripheral.
    Init_QEI();

    // Initialize the Timer for period measurement.
    //Init_WTIMER3();

    //Init_WTIMER2();

    Init_Timer1CCP();

    printf("Finished.\n");

    // Start TI-RTOS. Everything is now controlled by the BIOS, as defined in your project .CFG file.
    BIOS_start();
}

/*
 * ======== tasks ========
 */

// This is the background task.
void Task_Idle(void)
{
    //Required for using System_printf() command within interrupt routines.
    System_flush();
    return;
}

// This is a repetitive task that is executed every 1 ms.
// Timing can be modified in the .CFG file (TI-RTOS).
void Task_1ms(void)
{
    int32_t QEIPosPosition, QEIPosVelocity, QEIPosDirection;
    int32_t QEIAngPosition, QEIAngVelocity, QEIAngDirection;
    uint32_t TmrAngDeltaTemp, TmrPosDeltaTemp;
    uint32_t ui32AngUpdateFlag = 0;
    uint32_t ui32PosUpdateFlag = 0;


    QEIPosPosition = QEIPositionGet(QEI0_BASE);
    QEIPosVelocity = QEIVelocityGet(QEI0_BASE);
    QEIPosDirection = QEIDirectionGet(QEI0_BASE);
    TmrPosDeltaTemp = TmrPosDelta;

    QEIAngPosition = QEIPositionGet(QEI1_BASE);
    QEIAngVelocity = QEIVelocityGet(QEI1_BASE);
    QEIAngDirection = QEIDirectionGet(QEI1_BASE);
    TmrAngDeltaTemp = TmrAngDelta;


    Position = ((double)QEIPosPosition)*dblMultiplierPos;    // in mm


    if((MIN_QEI_VALUE)>=QEIPosVelocity)
    {   // Use timer value
        GPIOIntEnable(GPIO_PORTD_BASE, GPIO_PIN_6);

            Velocity = (double)dblMultiplierVelTimer*QEIPosDirection/((double)TmrPosDeltaTemp);
            if(QEIPosVelocity==0)
                    ui32PosUpdateFlag = 0x0010;
    }
    else
    {   // use QEI value
        GPIOIntDisable(GPIO_PORTD_BASE, GPIO_PIN_6);
        Velocity = ((double)QEIPosVelocity)*QEIPosDirection*dblMultiplierVelQEI;
    }

    Angle = ((double)QEIAngPosition)*dblMultiplierAngle;


    if(MIN_QEI_VALUE>=QEIAngVelocity)
    {   // Use timer value
        GPIOIntEnable(GPIO_PORTC_BASE, GPIO_PIN_6);
        AngleSpeed = (double)QEIAngDirection*dblMultiplierOmegaTimer/(TmrAngDeltaTemp);
        if(QEIAngVelocity==0)
            ui32AngUpdateFlag = 0x0010;
    }
    else
    {   // use QEI value
        GPIOIntDisable(GPIO_PORTC_BASE, GPIO_PIN_6);
        AngleSpeed = ((double)QEIAngVelocity)*QEIAngDirection*dblMultiplierOmegaQEI;

    }

    // Send a CAN message with the current state placed in the least significant byte.

    pui32TxBuffer[0] = (int32_t)Position;
    pui32TxBuffer[1] = (int32_t)Velocity;
    MsgObjectTx.ui32MsgID = 0x0001 | ui32PosUpdateFlag;         // Message ID is '1'.

    CANMessageSet(CAN0_BASE, 1, &MsgObjectTx, MSG_OBJ_TYPE_TX);

    pui32TxBuffer[0] = (int32_t)Angle;
    pui32TxBuffer[1] = (int32_t)AngleSpeed;
    MsgObjectTx.ui32MsgID = 0x0002 | ui32AngUpdateFlag;         // Message ID is '2'.

    CANMessageSet(CAN0_BASE, 1, &MsgObjectTx, MSG_OBJ_TYPE_TX);

}

/*
 * ======== user functions ========
 */

// This function initializes the clock/oscillator used in the project.
void Init_Clock(void)
{
    // Settings for 80 MHz.
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    dblMultiplierVelTimer = SysCtlClockGet()*dblMultiplierPos*EPP;
    dblMultiplierOmegaTimer = SysCtlClockGet()*dblMultiplierAngle*EPP;
    System_printf("SystemClock: %d\n",SysCtlClockGet());
    return;
}

// This function initializes the GPIO resources used in the project.
// Modify to your demands.
void Init_GPIO(void)
{
    printf("Initializing GPIOs...");
    // Enable the peripheral GPIO Bank F.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
        // Wait until it is ready...
        while (! SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
    // Set the GPIO directions for the respective pins.
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, (GPIO_PIN_2 | GPIO_PIN_3)); //LED Green and Blue

    //
    // Enable gpio interrupt
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    while (! SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC));


// Enable GPIOC interrupt
    GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, GPIO_PIN_6);
    //GPIOIntDisable(GPIO_PORTC_BASE, GPIO_PIN_6);
    GPIOIntClear(GPIO_PORTC_BASE, GPIO_INT_PIN_5);
    //GPIOIntTypeSet(GPIO_PORTC_BASE, GPIO_PIN_6,GPIO_BOTH_EDGES);
    GPIOIntTypeSet(GPIO_PORTC_BASE, GPIO_PIN_6,GPIO_RISING_EDGE);
    //GPIOIntEnable(GPIO_PORTC_BASE, GPIO_PIN_6);

    // Enable GPIOD interrupt (position)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
        while (! SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));
    GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_6);
    //GPIOIntDisable(GPIO_PORTD_BASE, GPIO_PIN_6);
    GPIOIntClear(GPIO_PORTD_BASE, GPIO_INT_PIN_6);
    //GPIOIntTypeSet(GPIO_PORTD_BASE, GPIO_PIN_6,GPIO_BOTH_EDGES);
    GPIOIntTypeSet(GPIO_PORTD_BASE, GPIO_PIN_6,GPIO_RISING_EDGE);
    //GPIOIntEnable(GPIO_PORTD_BASE, GPIO_PIN_6);



    ////////////////////////////////////////////////////////

    printf("done.\n");
    return;
}

// This function initializes the CAN controller resources used in the project.
// Modify to your demands.
void Init_CAN(void)
{
    printf("Initializing CAN...");
    // Enable the peripheral.
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOB);
        // Wait until it is ready...
        while (! SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));
    // Set the GPIO special functions (CANTX, CANRX) for the respective pins.
    GPIOPinConfigure(GPIO_PB5_CAN0TX);
    GPIOPinConfigure(GPIO_PB4_CAN0RX);
    GPIOPinTypeCAN (GPIO_PORTB_BASE , GPIO_PIN_4 | GPIO_PIN_5);
    // Enable the CAN peripheral CAN0.
    SysCtlPeripheralEnable (SYSCTL_PERIPH_CAN0);
    // Reset the CAN controller.
    CANInit(CAN0_BASE);
    // Set the baud rate to 250 kBaud/s based on the system clocking. Modify if needed.
    CANBitRateSet(CAN0_BASE, SysCtlClockGet(), 250000);
    CANBitRateSet(CAN0_BASE, SysCtlClockGet(), 500000);
    // Disable auto-retry if no ACK-bit is received by the CAN controlled.
    CANRetrySet(CAN0_BASE, 0);
    // More sophisticated CAN communication requires CAN interrupt handling.
    // For CAN0, the interrupt vector number is 55 (Macro: INT_CAN0_TM4C123).
    // Use the BIOS configuration file to set up a HWI for CAN0, with a suitable interrupt handler.
    // Uncomment the following lines to enable CAN interrupts.
    //CANIntEnable (CAN0_BASE , CAN_INT_MASTER);
    //IntEnable (INT_CAN0);TmrAngDeltaTemp
    CANEnable(CAN0_BASE);

    // Initialize all required message objects.
    MsgObjectTx.ui32MsgID = 0x0001;         // Message ID is '1'.
    MsgObjectTx.ui32Flags = 0x0000;         // No flags are used on this message object.
    MsgObjectTx.ui32MsgIDMask = 0x0000;     // No masking is used for TX.
    MsgObjectTx.ui32MsgLen = 8;             // Set the DLC to '8' (8 bytes of data)
    MsgObjectTx.pui8MsgData = pui32TxBuffer; // A buffer, to which this message object points for data storage.

    CANMessageSet(CAN0_BASE, 1, &MsgObjectTx, MSG_OBJ_TYPE_TX);

    printf("done.\n");
    printf("System clock used for CAN bus timing: %d Hz\n", SysCtlClockGet());
    return;
}


// This function initializes the GEI-Module used in the project.
// Modify to your demands.

void Init_QEI(void)
{
    uint32_t MeasureVelocityPeriod; // Specifies the number of clock ticks over which to measure the velocity.

    printf("Initializing QEI Module 0...");
<<<<<<< HEAD
    //MeasureVelocityPeriod = SysCtlClockGet();
    //dblVelocityPeriod = (double)MeasureVelocityPeriod/SysCtlClockGet();
    dblVelocityPeriod  = 0.001;
=======

>>>>>>> ff1668546cfb28239c6c3c8972f79d14cf1fbd73
    MeasureVelocityPeriod = (uint32_t)(dblVelocityPeriod*SysCtlClockGet());

    // Enable the peripheral.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_QEI0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    // Wait until it is ready...
            while (! SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
            while (! SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));

    // Set the GPIO special functions (IDX0, PHA, PHB) for the respective pins.
    GPIOPinConfigure(GPIO_PF4_IDX0); // QEI module 0 index (Pin 5)
    GPIOPinConfigure(GPIO_PD6_PHA0); // QEI module 0 phase A  (Pin 28)
    GPIOPinConfigure(GPIO_PF1_PHB0); // QEI module 0 phase B  (Pin 29)
    GPIOPinTypeQEI(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_1 );
    GPIOPinTypeQEI(GPIO_PORTD_BASE, GPIO_PIN_6 );

    SysCtlDelay(20);

    //Configures the quadrature encoder. The module must be configured before it is enabled.
    QEIConfigure(QEI0_BASE, (QEI_CONFIG_CAPTURE_A_B | QEI_CONFIG_QUADRATURE | QEI_CONFIG_RESET_IDX | QEI_CONFIG_NO_SWAP), 0xFFFFFFFF);

    SysCtlDelay(20);

    //Enable velocity capture QEI Module 0
    QEIVelocityConfigure(QEI0_BASE, QEI_VELDIV_1, MeasureVelocityPeriod);
    QEIVelocityEnable(QEI0_BASE);

    //Enable the quadrature encoder Module 0. The module must be configured before it is enabled.
    QEIEnable(QEI0_BASE);

    //Set Register start Values
    QEIPositionSet(QEI0_BASE,0);

    //Clear all QEI Interrupt sources
    QEIIntClear(QEI0_BASE, (QEI_INTERROR | QEI_INTDIR | QEI_INTTIMER | QEI_INTINDEX));
    QEIIntDisable(QEI0_BASE,(QEI_INTTIMER));

    printf("done.\n");

    printf("Initializing QEI Module 1...");

    // Enable the peripheral.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_QEI1);
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOC);
    // Wait until it is ready...
        while (! SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC));

    // Set the GPIO special functions (IDX0, PHA, PHB) for the respective pins.
    GPIOPinConfigure(GPIO_PC4_IDX1); // QEI module 1 index (Pin 5)
    GPIOPinConfigure(GPIO_PC5_PHA1); // QEI module 1 phase A  (Pin 28)
    GPIOPinConfigure(GPIO_PC6_PHB1); // QEI module 1 phase B  (Pin 29)
    GPIOPinTypeQEI(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6);

    SysCtlDelay(20);

    //Configures the quadrature encoder. The module must be configured before it is enabled.
    QEIConfigure(QEI1_BASE, (QEI_CONFIG_CAPTURE_A_B | QEI_CONFIG_RESET_IDX | QEI_CONFIG_QUADRATURE | QEI_CONFIG_NO_SWAP), 0xFFFFFFFF);

    SysCtlDelay(20);

    //Enable velocity capture QEI Module 1
    QEIVelocityConfigure(QEI1_BASE, QEI_VELDIV_1, MeasureVelocityPeriod);
    QEIVelocityEnable(QEI1_BASE);

    //Enable the quadrature encoder Module 1. The module must be configured before it is enabled.
    QEIEnable(QEI1_BASE);

    //Set Register start Values
    QEIPositionSet(QEI1_BASE,0);

    //Clear all QEI Interrupt sources
    QEIIntClear(QEI1_BASE, (QEI_INTERROR | QEI_INTDIR | QEI_INTTIMER | QEI_INTINDEX));
    QEIIntDisable(QEI1_BASE,(QEI_INTTIMER));

    printf("done.\n");

    return;
}


<<<<<<< HEAD
/*void Init_TIMER(void){
    printf("Initializing Timer...");

    // Enable timer3
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);
    SysCtlDelay(3);
    TimerDisable(TIMER3_BASE,TIMER_A);
    //Stop timer
    TimerClockSourceSet(TIMER3_BASE, TIMER_CLOCK_SYSTEM);
    TimerConfigure(TIMER3_BASE, TIMER_CFG_PERIODIC);  //Full-width periodic timer that counts down.
    TimerLoadSet(TIMER3_BASE, TIMER_A, 0xFFFFFFFF);       //Max value 9999999
    TimerControlStall(TIMER3_BASE, TIMER_A,0);
    TimerEnable(TIMER3_BASE, TIMER_A);

    printf("done.\n");
}

=======
>>>>>>> ff1668546cfb28239c6c3c8972f79d14cf1fbd73
void Init_WTIMER3(void){
    printf("Initializing Wide Timer3...");

    // Enable timer3
    SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER3);
    while (! SysCtlPeripheralReady(SYSCTL_PERIPH_WTIMER3));


    TimerDisable(WTIMER3_BASE,TIMER_A);
    //Stop timer
    TimerClockSourceSet(WTIMER3_BASE, TIMER_CLOCK_SYSTEM);
    TimerConfigure(WTIMER3_BASE, ( TIMER_CFG_A_CAP_TIME));  //Full-width periodic timer that counts down.
    TimerLoadSet(WTIMER3_BASE, TIMER_A, 0xFFFFFFFF);      //Max value 9999999
    TimerControlStall(WTIMER3_BASE, TIMER_A, 0);
    TimerEnable(WTIMER3_BASE, TIMER_A);
    TimerIntDisable(WTIMER3_BASE,(TIMER_CAPA_EVENT | TIMER_TIMA_DMA | TIMER_TIMA_TIMEOUT));
    TimerIntClear(WTIMER3_BASE,(TIMER_CAPA_EVENT | TIMER_TIMA_DMA | TIMER_TIMA_TIMEOUT));

    // Enable T3CCP(0/1)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
            while (! SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));
        GPIOPinTypeTimer(GPIO_PORTD_BASE, (GPIO_PIN_2 | GPIO_PIN_3));
        GPIOPinConfigure(GPIO_PD2_WT3CCP0);
        GPIOPinConfigure(GPIO_PD3_WT3CCP1);

        //TimerIntEnable(WTIMER3_BASE, TIMER_CAPA_EVENT);
        //TimerIntRegister(WTIMER3_BASE, TIMER_A, ISR_WTIMER3);
    printf("done.\n");
}


// Init velocity timer
void Init_WTIMER2(void){
    printf("Initializing Wide Timer4...");

    // Enable timer2
    SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER2);
    while (! SysCtlPeripheralReady(SYSCTL_PERIPH_WTIMER2));




    TimerDisable(WTIMER2_BASE,TIMER_BOTH);
    //Stop timer
    TimerClockSourceSet(WTIMER2_BASE, TIMER_CLOCK_SYSTEM);
    TimerConfigure(WTIMER2_BASE, (TIMER_CFG_A_CAP_TIME ));  //Full-width periodic timer that counts down.

    // Set timer event:
    TimerControlEvent(WTIMER2_BASE,TIMER_A,TIMER_EVENT_POS_EDGE);
    //TimerControlEvent(WTIMER2_BASE,TIMER_B,TIMER_EVENT_POS_EDGE);
    //TimerDMAEventSet(WTIMER2_BASE,TIMER_DMA_CAPEVENT_A);

    // Egal:
    TimerLoadSet(WTIMER2_BASE, TIMER_A, 0xFFFFFFFF);      //Max value 9999999
    // For debugging:
    TimerControlStall(WTIMER2_BASE, TIMER_A, 0);
    //


    // Enable T3CCP(0/1)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
         while (! SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));
    GPIOPinTypeTimer(GPIO_PORTD_BASE, (GPIO_PIN_0 | GPIO_PIN_1));
    GPIOPinConfigure(GPIO_PD0_WT2CCP0);
    GPIOPinConfigure(GPIO_PD1_WT2CCP1);

    TimerIntDisable(WTIMER2_BASE,(TIMER_CAPA_EVENT | TIMER_TIMA_DMA | TIMER_TIMA_TIMEOUT));
    TimerIntClear(WTIMER2_BASE,(TIMER_CAPA_EVENT | TIMER_TIMA_DMA | TIMER_TIMA_TIMEOUT));
    // Enable
    //SysCtlIntEnable(INT_WTIMER2A);
    TimerIntEnable(WTIMER2_BASE, (TIMER_CAPA_EVENT ));
    //TimerIntClear(WTIMER2_BASE, (TIMER_CAPA_EVENT | TIMER_CAPB_EVENT));
    //IntEnable(INT_WTIMER2A);

    TimerEnable(WTIMER2_BASE, (TIMER_A));

<<<<<<< HEAD
        TimerIntEnable(WTIMER2_BASE, TIMER_CAPA_EVENT);
=======
>>>>>>> ff1668546cfb28239c6c3c8972f79d14cf1fbd73
    printf("done.\n");
}*/

void Init_Timer1CCP(void){
    // Enable and configure Timer0 peripheral.


    // Enable and configure Timer0 peripheral.
        SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

        // Initialize timer A and B to count up in edge time mode
        TimerConfigure(TIMER0_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME_UP | TIMER_CFG_B_CAP_TIME_UP));

        // Timer a records pos edge time and Timer b records neg edge time
        TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_POS_EDGE);
        TimerControlEvent(TIMER0_BASE, TIMER_B, TIMER_EVENT_NEG_EDGE);

        //set the value that the timers count to (0x9C3F = 39999)
        //CO2 sensor outputs 1khz pwm so with mcu at 40Mhz, timers should stay in sync with CO2 output
        TimerLoadSet(TIMER0_BASE, TIMER_BOTH, 0x9C3F);

        //Configure the pin that the timer reads from (PB6)
        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
        GPIOPinConfigure(GPIO_PB6_T0CCP0);
        GPIOPinConfigure(GPIO_PB7_T0CCP1);
        GPIOPinTypeTimer(GPIO_PORTB_BASE, GPIO_PIN_6);
        GPIOPinTypeTimer(GPIO_PORTB_BASE, GPIO_PIN_7);


        // Registers a interrupt function to be called when timer b hits a neg edge event
        //IntRegister(INT_TIMER0A, duty_cycle);
        // Makes sure the interrupt is cleared
        TimerIntClear(TIMER0_BASE, TIMER_CAPA_EVENT);
        // Enable the indicated timer interrupt source.
        //TimerIntEnable(TIMER0_BASE, TIMER_CAPA_EVENT);
        // The specified interrupt is enabled in the interrupt controller.
        //IntEnable(INT_TIMER0A);

}

<<<<<<< HEAD
/*void Init_TIMER2(void){
    printf("Initializing Timer...");

    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);
    SysCtlDelay(3);
    TimerDisable(TIMER3_BASE,TIMER_A);
    //Stop timer
    TimerClockSourceSet(TIMER3_BASE, TIMER_CLOCK_SYSTEM);
    TimerConfigure(TIMER3_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME));  //Full-width periodic timer that counts down.
    TimerLoadSet(TIMER3_BASE, TIMER_A, 0xFFFFFFFF);       //Max value 9999999
    TimerControlStall(TIMER3_BASE, TIMER_A,0);
    TimerEnable(TIMER3_BASE, TIMER_A);
    // Aktivierung des Ports B und Konfiguration des Timer-PIN
        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    GPIOPinTypeTimer(GPIO_PORTB_BASE, (GPIO_PIN_2 | GPIO_PIN_3)); // Festlegung des Timer-CCP-Pins
        GPIOPinConfigure(GPIO_PB2_T3CCP0);             // Configurierung des T1CCP0-Pins (PB4)
        GPIOPinConfigure(GPIO_PB3_T3CCP1);             // Configurierung des T1CCP0-Pins (PB4)
    printf("done.\n");
}*/


void inputInt(){
  GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_2); //clear interrupt flag
  if ( GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2) == GPIO_PIN_2){
    HWREG(TIMER3_BASE + TIMER_O_TAV) = 0; //Loads value 0 into the timer.
    TimerEnable(TIMER3_BASE,TIMER_A); //start timer to recor

  }
  else{

    TimerDisable(TIMER3_BASE,TIMER_A); //stop timer
    pulse = TimerValueGet(TIMER3_BASE,TIMER_A); //record value


  }

}

void ISR_QEI0(void){
    QEIIntClear(QEI0_BASE,(QEI_INTTIMER));
//    if(QEIIntStatus(QEI0_BASE,TRUE)==QEI_INTTIMER)
//        {


}

void ISR_QEI1(void){
    QEIIntClear(QEI1_BASE,(QEI_INTTIMER));
//    if(QEIIntStatus(QEI0_BASE,TRUE)==QEI_INTTIMER)
//        {

//        }
//    else
//        System_printf("Other Interrupt\n");


}

/*void ISR_GPIOC(void){
    GPIOIntClear(GPIO_PORTC_BASE, GPIO_PIN_5); //clear interrupt flag
=======
void ISR_GPIOC(void){
    GPIOIntClear(GPIO_PORTC_BASE, GPIO_PIN_6); //clear interrupt flag
>>>>>>> ff1668546cfb28239c6c3c8972f79d14cf1fbd73

    TmrAngNew = TimerValueGet(WTIMER3_BASE, TIMER_A);

    TmrAngDelta = (TmrAngOld - TmrAngNew);

    TmrAngOld = TmrAngNew;

}/*


/*void ISR_GPIOD(void){
    GPIOIntClear(GPIO_PORTD_BASE, GPIO_PIN_6); //clear interrupt flag


    TmrPosNew = TimerValueGet(WTIMER2_BASE, TIMER_A);

    TmrPosDelta = (TmrPosOld - TmrPosNew);

    TmrPosOld = TmrPosNew;

<<<<<<< HEAD
}*/

void ISR_WTIMER2(void){
 TimerIntClear(WTIMER2_BASE, TIMER_A);

}
void ISR_WTIMER3(void){
 TimerIntClear(WTIMER3_BASE, TIMER_A);

}

void ISR_WTimer0A(void){
    TimerIntClear(TIMER0_BASE, (TIMER_CAPA_EVENT | TIMER_TIMA_DMA | TIMER_TIMA_TIMEOUT));
    TmrPosDelta = TimerValueGet(TIMER0_BASE, TIMER_A) - TimerValueGet(TIMER0_BASE, TIMER_B);
=======
>>>>>>> ff1668546cfb28239c6c3c8972f79d14cf1fbd73
}
