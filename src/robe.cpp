/*
 * Author: Yevgeniy Kiveisha <yevgeniy.kiveisha@intel.com>
 * Copyright (c) 2014 Intel Corporation.
 */

#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <string>
#include "json/json.h"
#include <stdlib.h>     /* srand, rand */
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>

#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"

#include "mraa.h"

#define PWM_BASE 	    3
#define PWM_SHOULDER 	6
#define PWM_ELBOW 	    5

#define NO  0
#define YES 1

#define HIGH_PULSE 	0
#define LOW_PULSE  	1

#define ENABLE 		1
#define DISABLE  	0

#define PERIOD_WIDTH	19800
#define MIN_PULSE_WIDTH 600
#define MAX_PULSE_WIDTH 2200

#define COORDINATE  1
#define SERVO       2

using namespace std;

mraa_pwm_context robeServoList[3];

void connectCallback(const redisAsyncContext *c, int status);
void disconnectCallback(const redisAsyncContext *c, int status);
void * redisSubscriber (void *);
void setAngle (mraa_pwm_context pwm, uint16_t angle);

int             running     = NO;
redisContext*   redisCtx    = NULL;
pthread_t       redisSubscriberThread;

int
main (int argc, char **argv) {
    mraa_result_t last_error = MRAA_SUCCESS;
	redisCtx = redisConnect("127.0.0.1", 6379);
	if (redisCtx->err) {
		exit (EXIT_FAILURE);
	}
    
    int error = pthread_create (&redisSubscriberThread, NULL, redisSubscriber, NULL);
    if (error) {
        exit(EXIT_FAILURE);
    }
    
    mraa_init();
	fprintf(stdout, "MRAA Version: %s\n", mraa_get_version());
    
    robeServoList[0] = mraa_pwm_init (PWM_BASE);
    robeServoList[1] = mraa_pwm_init (PWM_SHOULDER);
    robeServoList[2] = mraa_pwm_init (PWM_ELBOW);
    
    mraa_pwm_enable (robeServoList[0], ENABLE);
	mraa_pwm_enable (robeServoList[1], ENABLE);
    mraa_pwm_enable (robeServoList[2], ENABLE);
    
    printf("Starting the listener... [SUCCESS]\n");
	
	while (!running) {
        usleep (10);
	}
    
    redisFree(redisCtx);
    exit (EXIT_SUCCESS);
}

void subCallback(redisAsyncContext *c, void *r, void *priv) {
    redisReply * reply = (redisReply *)r;
    if (reply == NULL) return;
    if ( reply->type == REDIS_REPLY_ARRAY && reply->elements == 3 ) {
        if ( strcmp( reply->element[0]->str, "subscribe" ) != 0 ) {
            printf( "Received[%s] channel %s: %s\n", (char*)priv, reply->element[1]->str, reply->element[2]->str );
			
			Json::Value root;
			Json::Reader reader;
			bool parsingSuccessful = reader.parse( reply->element[2]->str, root );
			if (!parsingSuccessful) {
				std::cout  << "Failed to parse configuration\n"
						   << reader.getFormattedErrorMessages();
			} else {
                int handlerId	= root.get("handler", 0).asInt();
                switch (handlerId) {
                    case COORDINATE: {
                        int coordinateX	= root.get("x", 0).asInt();
                        int coordinateY	= root.get("y", 0).asInt();
                        int coordinateZ	= root.get("z", 0).asInt();
                        int coordinateP	= root.get("p", 0).asInt();
                        std::cout  	<< "COORDINATE ("
							<< coordinateX << "," << coordinateY << "," 
                            << coordinateZ << "," << coordinateP << ")\n";
                        // TODO - Inverse Kinematics
                    }
                    break;
                    case SERVO: {
                        int servoID	= root.get("id", 0).asInt();
                        int angle	= root.get("angle", 0).asInt();
                        
                        if (servoID > 0) {
                            std::cout  	<< "SERVO ("
                                        << servoID - 1 << ", " << angle << ")\n";
                            setAngle (robeServoList[servoID - 1], angle);
                        }
                    }
                    break;
                } 
			}
        }
    }
}

void
connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        return;
    }
    printf("Connected...\n");
}

void
disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        return;
    }
    printf("Disconnected...\n");
}

void *
redisSubscriber (void *) {
	signal(SIGPIPE, SIG_IGN);
    struct event_base *base = event_base_new();

    redisAsyncContext *redisAsyncCtx = redisAsyncConnect ("127.0.0.1", 6379);
    if (redisAsyncCtx->err) {
        return NULL;
    }

    redisLibeventAttach (redisAsyncCtx, base);
    redisAsyncSetConnectCallback (redisAsyncCtx, connectCallback);
    redisAsyncSetDisconnectCallback (redisAsyncCtx, disconnectCallback);
    redisAsyncCommand (redisAsyncCtx, subCallback, (char*) "sub", "SUBSCRIBE ROBE-IN");

    event_base_dispatch (base);
}

void
setAngle (mraa_pwm_context pwm, uint16_t angle) {
	float notches = ((float)(MAX_PULSE_WIDTH - MIN_PULSE_WIDTH) / 180);
	uint16_t width = notches * (float) angle + MIN_PULSE_WIDTH;

	mraa_pwm_pulsewidth_us (pwm, width);
}
