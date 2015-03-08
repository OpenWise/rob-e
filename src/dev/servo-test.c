#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "mraa.h"

#define SERVO_BASE      9
#define SERVO_SHOLDER   6
#define SERVO_ELBOW     3

#define HIGH_PULSE 	0
#define LOW_PULSE  	1

#define ENABLE 		1
#define DISABLE  	0

#define TRUE 		1
#define FALSE  		0

#define PERIOD_WIDTH	19800
#define MIN_PULSE_WIDTH 600
#define MAX_PULSE_WIDTH 2200

void setAngle (mraa_pwm_context pwm, uint16_t angle);

uint8_t isWorking = TRUE;

void
sigHandler(int signo) {
    if (signo == SIGINT) {
        isWorking = FALSE;
    }
}

int
main () {
    mraa_result_t last_error = MRAA_SUCCESS;
    mraa_pwm_context pwmBase;
    mraa_pwm_context pwmSholder;
    mraa_pwm_context pwmElbow;

    mraa_init();
    fprintf(stdout, "MRAA Version: %s\n", mraa_get_version());

    pwmBase     = mraa_pwm_init (SERVO_BASE);
    pwmSholder  = mraa_pwm_init (SERVO_SHOLDER);
    pwmElbow    = mraa_pwm_init (SERVO_ELBOW);

    if (pwmBase == NULL || pwmSholder == NULL) {
        return 1;
    }

    mraa_pwm_period_us (pwmBase,    PERIOD_WIDTH);
	mraa_pwm_period_us (pwmSholder, PERIOD_WIDTH);
    mraa_pwm_period_us (pwmElbow,   PERIOD_WIDTH);
	
	mraa_pwm_enable (pwmBase,    ENABLE);
	mraa_pwm_enable (pwmSholder, ENABLE);
    mraa_pwm_enable (pwmElbow,   ENABLE);

	signal (SIGINT, sigHandler);

    while (isWorking == TRUE) {
        printf ("# Angle -> 0\n");
        setAngle (pwmBase,      0);
    	setAngle (pwmSholder,   80);
        setAngle (pwmElbow,     30);
    	usleep (2000000);

        printf ("# Angle -> 180\n");
    	setAngle (pwmBase,      180);
    	setAngle (pwmSholder,   120);
        setAngle (pwmElbow,     100);
    	usleep (2000000);
    }

    return 0;
}

void
setAngle (mraa_pwm_context pwm, uint16_t angle) {
	float notches = ((float)(MAX_PULSE_WIDTH - MIN_PULSE_WIDTH) / 180);
	uint16_t width = notches * (float) angle + MIN_PULSE_WIDTH;

	mraa_pwm_pulsewidth_us (pwm, width);
}

