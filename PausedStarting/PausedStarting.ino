/*$file${.::PausedStarting.ino} ############################################*/
/*
* Model: PausedStarting.qm
* File:  ${.::PausedStarting.ino}
*
* This code has been generated by QM tool (https://state-machine.com/qm).
* DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*/
/*$endhead${.::PausedStarting.ino} #########################################*/
#define Q_PARAM_SIZE 4 // 32 bit parameters (e.g. pointers) passed between events.

#include "qpn.h"     // QP-nano framework
#include "Arduino.h" // Arduino API

#include "Zumo32U4.h"

//============================================================================
// declare all AO classes...
/*$declare${AOs::Sumo} #####################################################*/
/*${AOs::Sumo} .............................................................*/
typedef struct Sumo {
/* protected: */
    QActive super;

/* public: */
    uint16_t ready_time_remaining = 0;
} Sumo;

/* protected: */
static QState Sumo_initial(Sumo * const me);
static QState Sumo_paused(Sumo * const me);
static QState Sumo_getting_ready(Sumo * const me);
/*$enddecl${AOs::Sumo} #####################################################*/
/*$declare${AOs::ButtonPressSimulator} #####################################*/
/*${AOs::ButtonPressSimulator} .............................................*/
typedef struct ButtonPressSimulator {
/* protected: */
    QActive super;
} ButtonPressSimulator;

/* protected: */
static QState ButtonPressSimulator_initial(ButtonPressSimulator * const me);
static QState ButtonPressSimulator_Running(ButtonPressSimulator * const me);
/*$enddecl${AOs::ButtonPressSimulator} #####################################*/
//...

// AO instances and event queue buffers for them...
Sumo AO_Sumo;
ButtonPressSimulator AO_ButtonSim;


// Other objects
Zumo32U4LCD lcd;
Zumo32U4ButtonA buttonA;

static QEvt l_SumoQSto[10]; // Event queue storage for Blinky
static QEvt l_ButtonSimQSto[10];
//...

//============================================================================
// QF_active[] array defines all active object control blocks ----------------
QActiveCB const Q_ROM QF_active[] = {
    { (QActive *)0,               (QEvt *)0,           0U                        },
    { (QActive *)&AO_Sumo,        l_SumoQSto,          Q_DIM(l_SumoQSto)         },
    { (QActive *)&AO_ButtonSim,   l_ButtonSimQSto,     Q_DIM(l_ButtonSimQSto)    }
};

//============================================================================
// various constants for the application...
enum {
    BSP_TICKS_PER_SEC = 1000, // number of system clock ticks in one second
};

enum Signals {
 BUTTON_PRESS_SIG = Q_USER_SIG,
 READY_TIMEOUT_SIG,
 MAXSIGS
};



//............................................................................
void setup() {
    // initialize the QF-nano framework
    QF_init(Q_DIM(QF_active));

    // initialize all AOs...
    QActive_ctor(&AO_Sumo.super, Q_STATE_CAST(&Sumo_initial));

    QActive_ctor(&AO_ButtonSim.super, Q_STATE_CAST(&ButtonPressSimulator_initial));
}

//............................................................................
void loop() {
    QF_run(); // run the QF-nano framework
}

//============================================================================
// interrupts...
ISR(TIMER4_COMPA_vect) {
    QF_tickXISR(0); // process time events for tick rate 0
}

//============================================================================
// QF callbacks...
void QF_onStartup(void) {

    /**
        1. Generally, ignoring TC4H when writing TCNT, OCR and other registers
           (i.e. we assume it is zero) in each case as we expect the count to fit in 8 bits.
        2. General idea is to use OCR4A to generate an interrupt. We could also use the overflow interrupt, but a
           handler for that is defined in the pololu library code, so compilation will fail or we will need to
           remove the handler from the pololu code.
        3. See below for specifics about interaction between OCR4A and OCR4C.
    */

    // TCCR4A is used to generate output pin signals (along with interrupt), but we don't want to generate output.
    TCCR4A = 0;
    // Prescale by 1/128
    TCCR4B = (0<<PWM4X) | (0<<PSR4) | (0<<DTPS41) | (0<<DTPS40) | (1<<CS43) | (0<<CS42) | (0<<CS41) | (0<<CS40);

    // For some reason, not setting TCCR4D to zero explicitly results in doubling the time interval. All initial
    // values of TCCR4D are zero according to the datasheet. Since I was unable to explain this, I have expliclty
    // set all control registers to zero instead of assuming they are zero.
    TCCR4C = 0;
    TCCR4D = 0;
    TCCR4E = 0;

    TCNT4=0x00;

    /* Calculate Timer Count Max.
        1. We don't explicilty set OCR4B/D as it is not used. Setting it has no effect (verified).
        2. We set OCR4C to the same value as OCR4A. OCR4A generates an interrupt on compare match.
           OCR4C contains the 'TOP' value,  i.e. the value to which the timer is reset.
        2.1 Note: Not setting OCR4C causes the uP to double the count. No idea why. (verified)
        2.2 Note: Setting OCR4C to zero or Setting it to a value other than OCR4A, results in no OCR4A interrupt being generated. (verified)
    */
    OCR4A  = (F_CPU / BSP_TICKS_PER_SEC / 128U) - 1;
    OCR4C =(F_CPU / BSP_TICKS_PER_SEC / 128U) - 1U;

    // Enable TIMER compare Interrupt for OCR4A
    TIMSK4= (1<<OCIE4A);
}
//............................................................................
void QV_onIdle(void) {   // called with interrupts DISABLED
    // Put the CPU and peripherals to the low-power mode. You might
    // need to customize the clock management for your application,
    // see the datasheet for your particular AVR MCU.
    SMCR = (0 << SM0) | (1 << SE); // idle mode, adjust to your project
    QV_CPU_SLEEP();  // atomically go to sleep and enable interrupts
}
//............................................................................
void Q_onAssert(char const Q_ROM * const file, int line) {
    // implement the error-handling policy for your application!!!
    QF_INT_DISABLE(); // disable all interrupts
    QF_RESET();  // reset the CPU
}

//============================================================================
// define all AO classes (state machine)...
/*$define${AOs::Sumo} ######################################################*/
/* Check for the minimum required QP version */
#if ((QP_VERSION < 601) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8)))
#error qpn version 6.0.1 or higher required
#endif
/*${AOs::Sumo} .............................................................*/
/*${AOs::Sumo::SM} .........................................................*/
static QState Sumo_initial(Sumo * const me) {
    /*${AOs::Sumo::SM::initial} */
    return Q_TRAN(&Sumo_paused);
}
/*${AOs::Sumo::SM::paused} .................................................*/
static QState Sumo_paused(Sumo * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /*${AOs::Sumo::SM::paused} */
        case Q_ENTRY_SIG: {
            ledRed(HIGH);

            lcd.clear();
            lcd.print(F("Press A"));
            status_ = Q_HANDLED();
            break;
        }
        /*${AOs::Sumo::SM::paused::BUTTON_PRESS} */
        case BUTTON_PRESS_SIG: {
            ledRed(LOW);
            me->ready_time_remaining = 5000;
            QActive_armX((QActive *)me, 0 /*Tick rate */, 10 /* millis */, 10);
            status_ = Q_TRAN(&Sumo_getting_ready);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*${AOs::Sumo::SM::getting_ready} ..........................................*/
static QState Sumo_getting_ready(Sumo * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /*${AOs::Sumo::SM::getting_ready::Q_TIMEOUT} */
        case Q_TIMEOUT_SIG: {
            me->ready_time_remaining -= 10; // reduce by 10 millis.
            float timeToPrint = ((float)me->ready_time_remaining)/1000;
            lcd.clear();
            lcd.print(timeToPrint, 2);
            if(me->ready_time_remaining <= 0){
                QActive_disarmX((QActive *)me, 0 /*Tick rate */);
                QACTIVE_POST((QActive *)me, READY_TIMEOUT_SIG, 0);
            }
            status_ = Q_TRAN(&Sumo_getting_ready);
            break;
        }
        /*${AOs::Sumo::SM::getting_ready::READY_TIMEOUT} */
        case READY_TIMEOUT_SIG: {
            status_ = Q_TRAN(&Sumo_paused);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*$enddef${AOs::Sumo} ######################################################*/
/*$define${AOs::ButtonPressSimulator} ######################################*/
/*${AOs::ButtonPressSimulator} .............................................*/
/*${AOs::ButtonPressSimul~::SM} ............................................*/
static QState ButtonPressSimulator_initial(ButtonPressSimulator * const me) {
    /*${AOs::ButtonPressSimul~::SM::initial} */
    QActive_armX(&me->super, 0 /*Tick rate */, 10 /* millis */, 10);
    return Q_TRAN(&ButtonPressSimulator_Running);
}
/*${AOs::ButtonPressSimul~::SM::Running} ...................................*/
static QState ButtonPressSimulator_Running(ButtonPressSimulator * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /*${AOs::ButtonPressSimul~::SM::Running::Q_TIMEOUT} */
        case Q_TIMEOUT_SIG: {
            //lcd.clear();
            //lcd.print(F("Timer"));

            if (buttonA.getSingleDebouncedPress()) {
                QACTIVE_POST((QActive *)&AO_Sumo, BUTTON_PRESS_SIG, 0);
            }
            status_ = Q_TRAN(&ButtonPressSimulator_Running);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*$enddef${AOs::ButtonPressSimulator} ######################################*/
//...