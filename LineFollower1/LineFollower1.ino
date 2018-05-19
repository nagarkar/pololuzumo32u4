/*$file${.::LineFollower1.ino} #############################################*/
/*
* Model: LineFollower1.qm
* File:  ${.::LineFollower1.ino}
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
/*$endhead${.::LineFollower1.ino} ##########################################*/
#define Q_PARAM_SIZE 4 // 32 bit parameters (e.g. pointers) passed between events.

#include "qpn.h"     // QP-nano framework
#include "Arduino.h" // Arduino API

#include "Zumo32U4.h"

static int DISABLE_PRINT = 0; // false

#define PRINT_IF_ENABLED if (!DISABLE_PRINT)
#define MY_ENABLE if (1==1)
#define MY_DISABLE if (1==0)

static int started = 0;

//============================================================================
// declare all AO classes...
/*$declare${AOs::LineEvtSimulator} #########################################*/
/*${AOs::LineEvtSimulator} .................................................*/
typedef struct LineEvtSimulator {
/* protected: */
    QActive super;

/* public: */
    const uint8_t period = 5;
} LineEvtSimulator;

/* public: */
static void LineEvtSimulator_printToLCD(LineEvtSimulator * const me, uint16_t * lineSensorValues);
static void LineEvtSimulator_printBar(LineEvtSimulator * const me, uint8_t height);

/* protected: */
static QState LineEvtSimulator_initial(LineEvtSimulator * const me);
static QState LineEvtSimulator_running(LineEvtSimulator * const me);
static QState LineEvtSimulator_waiting_to_run(LineEvtSimulator * const me);
/*$enddecl${AOs::LineEvtSimulator} #########################################*/
/*$declare${AOs::ButtonEvtSimulator} #######################################*/
/*${AOs::ButtonEvtSimulator} ...............................................*/
typedef struct ButtonEvtSimulator {
/* protected: */
    QActive super;

/* public: */
    const uint8_t period =50;
} ButtonEvtSimulator;

/* protected: */
static QState ButtonEvtSimulator_initial(ButtonEvtSimulator * const me);
static QState ButtonEvtSimulator_running(ButtonEvtSimulator * const me);
/*$enddecl${AOs::ButtonEvtSimulator} #######################################*/
/*$declare${AOs::Sumo} #####################################################*/
/*${AOs::Sumo} .............................................................*/
typedef struct Sumo {
/* protected: */
    QActive super;

/* public: */
    uint16_t ready_time_remaining = 0;
    uint16_t turn_speed = 400;
    const uint16_t ready_timeout_ms = 2000;
    const uint16_t calibration_timeout = 2000;
    bool debug_mode = true;
    const float kp = 0.7;
    const float ki = 1;
    const float kd = 6;
    int last_error = 0;
    long total_error = 0;
    const uint8_t calibration_cycle_time = 50;
} Sumo;

/* protected: */
static QState Sumo_initial(Sumo * const me);
static QState Sumo_paused(Sumo * const me);
static QState Sumo_ready(Sumo * const me);
static QState Sumo_calibrating(Sumo * const me);
static QState Sumo_waiting_and_refresh_pos(Sumo * const me);
static QState Sumo_pid_line_follow(Sumo * const me);
/*$enddecl${AOs::Sumo} #####################################################*/

//...


// AO instances and event queue buffers for them...
Sumo AO_Sumo;
ButtonEvtSimulator AO_ButtonSim;
LineEvtSimulator AO_LineSim;


// Other objects
Zumo32U4LCD lcd;
Zumo32U4ButtonA buttonA;
Zumo32U4Motors motors;
Zumo32U4LineSensors lineSensors;


static QEvt l_SumoQSto[100]; // Event queue storage for Blinky
static QEvt l_ButtonSimQSto[100];
static QEvt l_LineSimQSto[100];

//...

//============================================================================
// QF_active[] array defines all active object control blocks ----------------
QActiveCB const Q_ROM QF_active[] = {
    { (QActive *)0,               (QEvt *)0,           0U                        },
    { (QActive *)&AO_Sumo,        l_SumoQSto,          Q_DIM(l_SumoQSto)         },
    { (QActive *)&AO_ButtonSim,   l_ButtonSimQSto,     Q_DIM(l_ButtonSimQSto)    },
    { (QActive *)&AO_LineSim,     l_LineSimQSto,       Q_DIM(l_LineSimQSto)      }
};

//============================================================================
// various constants for the application...
enum {
    BSP_TICKS_PER_SEC = 1000, // number of system clock ticks in one second
};

enum Signals {
 BUTTON_PRESS_SIG = Q_USER_SIG,
 READY_TIMEOUT_SIG,
 RUN_SIG,
 LINE_POSITION_UPDATE_SIG,
 MAXSIGS
};



//............................................................................
void setup() {
    // initialize the QF-nano framework
    QF_init(Q_DIM(QF_active));

    // initialize all AOs...
    QActive_ctor(&AO_Sumo.super,         Q_STATE_CAST(&Sumo_initial));

    QActive_ctor(&AO_ButtonSim.super,    Q_STATE_CAST(&ButtonEvtSimulator_initial));

    QActive_ctor(&AO_LineSim.super,      Q_STATE_CAST(&LineEvtSimulator_initial));
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
    int batt = readBatteryMillivolts();
    if (batt > 6000) {
        me->turn_speed = 300;
    } else if (batt > 5000) {
        me->turn_speed = 350;
    } else {
        me->turn_speed = 400;
    }
    return Q_TRAN(&Sumo_paused);
}
/*${AOs::Sumo::SM::paused} .................................................*/
static QState Sumo_paused(Sumo * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /*${AOs::Sumo::SM::paused} */
        case Q_ENTRY_SIG: {
            ledRed(HIGH);

            MY_ENABLE {
                lcd.clear();
                lcd.print(F("Press A:"));


                lcd.gotoXY(0, 1);
                lcd.print(readBatteryMillivolts());
                lcd.print(":");
                if(started == 0) {
                    lcd.print("rst");
                    started = 1;
                } else {
                    lcd.print("ret");
                }
            }
            status_ = Q_HANDLED();
            break;
        }
        /*${AOs::Sumo::SM::paused::BUTTON_PRESS} */
        case BUTTON_PRESS_SIG: {
            ledRed(LOW);
            me->ready_time_remaining = me->ready_timeout_ms;
            QActive_armX((QActive *)me, 0 /*Tick rate */, 10 /* millis */, 10);

            // Toggle speed (used for debugging)
            //if (me->debug_mode) {
            //    if (me->turn_speed == 0) {
            //        me->turn_speed = 250;
            //    } else {
            //        me->turn_speed = 0;
            //    }
            //}
            status_ = Q_TRAN(&Sumo_ready);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*${AOs::Sumo::SM::ready} ..................................................*/
static QState Sumo_ready(Sumo * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /*${AOs::Sumo::SM::ready} */
        case Q_ENTRY_SIG: {
            MY_ENABLE {
                lcd.clear();
                lcd.print("READY");
            }
            status_ = Q_HANDLED();
            break;
        }
        /*${AOs::Sumo::SM::ready::Q_TIMEOUT} */
        case Q_TIMEOUT_SIG: {
            me->ready_time_remaining -= 10; // reduce by 10 millis.
            float timeToPrint = ((float)me->ready_time_remaining)/1000;
            lcd.clear();
            lcd.print(timeToPrint, 2);
            if(me->ready_time_remaining <= 0){
                QActive_disarmX((QActive *)me, 0 /*Tick rate */);
                QACTIVE_POST((QActive *)me, READY_TIMEOUT_SIG, 0);
            }
            status_ = Q_TRAN(&Sumo_ready);
            break;
        }
        /*${AOs::Sumo::SM::ready::READY_TIMEOUT} */
        case READY_TIMEOUT_SIG: {
            QActive_disarmX((QActive *)me, 0 /*Tick rate */);

            QActive_armX((QActive *)me, 0 /*Tick rate */,
                 me->calibration_cycle_time, me->calibration_cycle_time);
            me->ready_time_remaining = me->calibration_timeout;
            status_ = Q_TRAN(&Sumo_calibrating);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*${AOs::Sumo::SM::calibrating} ............................................*/
static QState Sumo_calibrating(Sumo * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /*${AOs::Sumo::SM::calibrating} */
        case Q_ENTRY_SIG: {
            motors.setSpeeds(-200, 200);
            MY_ENABLE {
                lcd.clear();
                lcd.print("Calib");
            }

            status_ = Q_HANDLED();
            break;
        }
        /*${AOs::Sumo::SM::calibrating} */
        case Q_EXIT_SIG: {
            MY_DISABLE {
                lcd.clear();
                lcd.print(*lineSensors.calibratedMinimumOff);
                lcd.print(",");
                lcd.print(*lineSensors.calibratedMinimumOn);
                lcd.gotoXY(0,1);
                lcd.print(*lineSensors.calibratedMaximumOff);
                lcd.print(",");
                lcd.print(*lineSensors.calibratedMaximumOn);
            }
            status_ = Q_HANDLED();
            break;
        }
        /*${AOs::Sumo::SM::calibrating::BUTTON_PRESS} */
        case BUTTON_PRESS_SIG: {
            status_ = Q_TRAN(&Sumo_paused);
            break;
        }
        /*${AOs::Sumo::SM::calibrating::Q_TIMEOUT} */
        case Q_TIMEOUT_SIG: {
            // Main action
            lineSensors.calibrate();


            // Time keeping
            me->ready_time_remaining -= 50; // reduce by 10 millis.
            float timeToPrint = ((float)me->ready_time_remaining)/1000;
            lcd.clear();
            lcd.print(timeToPrint, 2);

            // Exit condition
            if(me->ready_time_remaining <= 0){
                QActive_disarmX((QActive *)me, 0 /*Tick rate */);
                QACTIVE_POST((QActive *)me, READY_TIMEOUT_SIG, 0);
                QACTIVE_POST((QActive *)&AO_LineSim, RUN_SIG, 0 /* don't care */);
            }
            status_ = Q_TRAN(&Sumo_calibrating);
            break;
        }
        /*${AOs::Sumo::SM::calibrating::READY_TIMEOUT} */
        case READY_TIMEOUT_SIG: {
            MY_ENABLE {
                lcd.clear();
                lcd.print("WAIT");
            }
            status_ = Q_TRAN(&Sumo_waiting_and_refresh_pos);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*${AOs::Sumo::SM::waiting_and_refresh_pos} ................................*/
static QState Sumo_waiting_and_refresh_pos(Sumo * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /*${AOs::Sumo::SM::waiting_and_refresh_pos} */
        case Q_ENTRY_SIG: {
            motors.setSpeeds(0,0);
            status_ = Q_HANDLED();
            break;
        }
        /*${AOs::Sumo::SM::waiting_and_refr~::BUTTON_PRESS} */
        case BUTTON_PRESS_SIG: {
            MY_ENABLE {
                lcd.clear();
                lcd.print("PID");
            }
            status_ = Q_TRAN(&Sumo_pid_line_follow);
            break;
        }
        /*${AOs::Sumo::SM::waiting_and_refr~::LINE_POSITION_UPDATE} */
        case LINE_POSITION_UPDATE_SIG: {
            unsigned int line_position = Q_PAR(me);
            MY_ENABLE {
                lcd.clear();
                lcd.print(line_position);
            }
            status_ = Q_TRAN(&Sumo_waiting_and_refresh_pos);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*${AOs::Sumo::SM::pid_line_follow} ........................................*/
static QState Sumo_pid_line_follow(Sumo * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /*${AOs::Sumo::SM::pid_line_follow} */
        case Q_ENTRY_SIG: {

            unsigned int line_position = Q_PAR(me);
            MY_DISABLE {
                lcd.clear();
                lcd.print(line_position);
            }

            if (line_position >= 0) {
                // Calculate error
                int16_t error = line_position - 2000; /* 2000 is desired position */

                // Compute PID control
                //int16_t speedDifference = error / 4;
                int16_t speedDifference  =
                          ((float)error)                     * me->kp
                        + ((float)(error - me->last_error))  * me->kd
                      // + ((float)(error + me->total_error)) * me->ki
                    ;

                // Update state;
                me->last_error = error;
                me->total_error = me->total_error + error;

                // Calculate input with PID, within rated limits.
                int16_t leftSpeed = me->turn_speed + speedDifference;
                int16_t rightSpeed = me->turn_speed - speedDifference;

                leftSpeed  = constrain(leftSpeed,  0, me->turn_speed);
                rightSpeed = constrain(rightSpeed, 0, me->turn_speed);

                // Send speed to motor
                motors.setSpeeds(leftSpeed, rightSpeed);

                MY_DISABLE {
                    lcd.clear();
                    lcd.print("LS:");lcd.print(leftSpeed);
                    lcd.gotoXY(0,1);
                    lcd.print("RS:");lcd.print(rightSpeed);
                }
            }
            status_ = Q_HANDLED();
            break;
        }
        /*${AOs::Sumo::SM::pid_line_follow} */
        case Q_EXIT_SIG: {
            motors.setSpeeds(0,0);
            status_ = Q_HANDLED();
            break;
        }
        /*${AOs::Sumo::SM::pid_line_follow::LINE_POSITION_UPDATE} */
        case LINE_POSITION_UPDATE_SIG: {
            MY_DISABLE {
                lcd.clear();
                lcd.print("LPUPD EVENT");
            }
            status_ = Q_TRAN(&Sumo_pid_line_follow);
            break;
        }
        /*${AOs::Sumo::SM::pid_line_follow::BUTTON_PRESS} */
        case BUTTON_PRESS_SIG: {
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
/*$define${AOs::ButtonEvtSimulator} ########################################*/
/*${AOs::ButtonEvtSimulator} ...............................................*/
/*${AOs::ButtonEvtSimulat~::SM} ............................................*/
static QState ButtonEvtSimulator_initial(ButtonEvtSimulator * const me) {
    /*${AOs::ButtonEvtSimulat~::SM::initial} */
    QActive_armX(&me->super, 0 /*Tick rate */, me->period /* millis */, me->period);
    return Q_TRAN(&ButtonEvtSimulator_running);
}
/*${AOs::ButtonEvtSimulat~::SM::running} ...................................*/
static QState ButtonEvtSimulator_running(ButtonEvtSimulator * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /*${AOs::ButtonEvtSimulat~::SM::running::Q_TIMEOUT} */
        case Q_TIMEOUT_SIG: {
            //lcd.clear();
            //lcd.print(F("Timer"));

            if (buttonA.getSingleDebouncedPress()) {
                lcd.clear();
                lcd.print(F("button pressed"));
                QACTIVE_POST((QActive *)&AO_Sumo, BUTTON_PRESS_SIG, 0);
            }
            status_ = Q_TRAN(&ButtonEvtSimulator_running);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*$enddef${AOs::ButtonEvtSimulator} ########################################*/
/*$define${AOs::LineEvtSimulator} ##########################################*/
/*${AOs::LineEvtSimulator} .................................................*/
/*${AOs::LineEvtSimulator::printToLCD} .....................................*/
static void LineEvtSimulator_printToLCD(LineEvtSimulator * const me, uint16_t * lineSensorValues) {
    // On the first line of the LCD, display the bar graph.
    lcd.gotoXY(0, 0);
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t barHeight = map(lineSensorValues[i], 0, 2000, 0, 8);
        LineEvtSimulator_printBar(me, barHeight);
    }
}

/*${AOs::LineEvtSimulator::printBar} .......................................*/
static void LineEvtSimulator_printBar(LineEvtSimulator * const me, uint8_t height) {
    if (height > 8) { height = 8; }
    const char barChars[] = {' ', 0, 1, 2, 3, 4, 5, 6, 255};
    lcd.print(barChars[height]);
}

/*${AOs::LineEvtSimulator::SM} .............................................*/
static QState LineEvtSimulator_initial(LineEvtSimulator * const me) {
    /*${AOs::LineEvtSimulator::SM::initial} */
    return Q_TRAN(&LineEvtSimulator_waiting_to_run);
}
/*${AOs::LineEvtSimulator::SM::running} ....................................*/
static QState LineEvtSimulator_running(LineEvtSimulator * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /*${AOs::LineEvtSimulator::SM::running::Q_TIMEOUT} */
        case Q_TIMEOUT_SIG: {
            unsigned int line_sensor_values[5];

            MY_DISABLE {
                lineSensors.readCalibrated(line_sensor_values);
                lcd.clear();
                LineEvtSimulator_printToLCD(me, line_sensor_values);
            }

            unsigned int position = lineSensors.readLine(line_sensor_values);

            MY_DISABLE {
                lcd.clear();//lcd.gotoXY(0,1);
                lcd.print(position);
            }

            QACTIVE_POST_X((QActive *)&AO_Sumo, 1, LINE_POSITION_UPDATE_SIG, position);
            status_ = Q_TRAN(&LineEvtSimulator_running);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*${AOs::LineEvtSimulator::SM::waiting_to_run} .............................*/
static QState LineEvtSimulator_waiting_to_run(LineEvtSimulator * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /*${AOs::LineEvtSimulator::SM::waiting_to_run} */
        case Q_ENTRY_SIG: {
            lineSensors.initFiveSensors();

            // Setup LCD characters required for printing line values to LCD
            static const char levels[] PROGMEM = {
                0, 0, 0, 0, 0, 0, 0, 63, 63, 63, 63, 63, 63, 63
            };
            lcd.loadCustomCharacter(levels + 0, 0);  // 1 bar
            lcd.loadCustomCharacter(levels + 1, 1);  // 2 bars
            lcd.loadCustomCharacter(levels + 2, 2);  // 3 bars
            lcd.loadCustomCharacter(levels + 3, 3);  // 4 bars
            lcd.loadCustomCharacter(levels + 4, 4);  // 5 bars
            lcd.loadCustomCharacter(levels + 5, 5);  // 6 bars
            lcd.loadCustomCharacter(levels + 6, 6);  // 7 bars
            status_ = Q_HANDLED();
            break;
        }
        /*${AOs::LineEvtSimulator::SM::waiting_to_run::RUN} */
        case RUN_SIG: {
            QActive_armX(&me->super, 0 /*Tick rate */, me->period /* millis */, me->period);
            status_ = Q_TRAN(&LineEvtSimulator_running);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*$enddef${AOs::LineEvtSimulator} ##########################################*/
//...
