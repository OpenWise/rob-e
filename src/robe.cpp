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
#include <cmath>

#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"

#include "mraa.h"

#define PWM_BASE 	    3
#define PWM_SHOULDER 	5
#define PWM_ELBOW 	    6
#define PWM_WHRIST      9
#define PWM_GRIPPER     4

#define BASE        0
#define SHOULDER    1
#define ELBOW       2
#define WHRIST      3
#define GRIPPER     4

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

#define SERVO_SPEED_LOW       0
#define SERVO_SPEED_MIDDLE    1
#define SERVO_SPEED_HIGH      2

using namespace std;

typedef struct {
    mraa_pwm_context pwmCtx;
    int              currentAngle;
} servo_context_t;

typedef struct {
    float x;
    float y;
    float z;
    int   p;
} coordinate_t;

typedef struct {
    float tn;
    float j1;
    float j2;
    float j3;
} arm_angles_t;

typedef struct {
    float           z_offset;
    float           coxa;
    float           fermur;
    float           tibia;
    coordinate_t    coord;
    arm_angles_t    angles;
    arm_angles_t*   angles_ptr;
} arm_context_t;

arm_angles_t angleMap[] = {
    {  95.0, 100.0, 165.0, 135.0 },
    { 100.0, 110.0, 140.0, 135.0 },
    { 105.0, 125.0, 105.0, 135.0 },
    { 130.0,  95.0, 165.0, 135.0 },
    { 125.0, 110.0, 130.0, 135.0 },
    { 120.0, 135.0,  85.0, 145.0 },
    { 155.0, 110.0, 135.0, 130.0 },
    { 145.0, 120.0, 115.0, 140.0 },
    { 135.0, 140.0,  85.0, 130.0 },

    {  95.0,  65.0, 165.0, 165.0 },
    { 100.0,  90.0, 120.0, 165.0 },
    { 100.0, 120.0,  80.0, 165.0 },
    { 130.0,  90.0, 120.0, 180.0 },
    { 120.0, 110.0,  95.0, 180.0 },
    { 120.0, 110.0,  95.0, 150.0 },
    { 155.0,  85.0, 125.0, 155.0 },
    { 140.0,  90.0, 120.0, 150.0 },
    { 125.0, 120.0,  90.0, 130.0 },
    
    { 105.0,  60.0, 125.0, 180.0 },
    { 105.0,  80.0, 110.0, 165.0 },
    { 105.0, 100.0,  85.0, 155.0 },
    { 125.0,  65.0, 120.0, 180.0 },
    { 125.0,  80.0, 115.0, 170.0 },
    { 120.0, 105.0,  85.0, 155.0 },
    { 155.0,  70.0, 125.0, 165.0 },
    { 135.0, 100.0,  80.0, 180.0 },
    { 125.0, 120.0,  55.0, 155.0 },
};

void connectCallback(const redisAsyncContext *c, int status);
void disconnectCallback(const redisAsyncContext *c, int status);
void * redisSubscriber (void *);
void setAngle (servo_context_t& ctx, int angle, uint8_t speed);
void publish (redisContext* ctx, char* buffer);
void servoMsgFactory (char* buffer, int id, int angle);
uint8_t calculateAngles (arm_context_t& ctx);
uint8_t findAnglesMap (arm_context_t& ctx);

arm_context_t    robe;
servo_context_t  servoCtxList[4];
int              running     = NO;
redisContext*    redisCtx    = NULL;
pthread_t        redisSubscriberThread;

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

    robe.z_offset   = 5;
    robe.coxa       = 5.5;
    robe.fermur     = 5.5;
    robe.tibia      = 8;

    mraa_init();
	fprintf(stdout, "MRAA Version: %s\n", mraa_get_version());
    
    servoCtxList[BASE].pwmCtx           = mraa_pwm_init (PWM_BASE);
    servoCtxList[BASE].currentAngle     = 90;
    servoCtxList[SHOULDER].pwmCtx       = mraa_pwm_init (PWM_SHOULDER);
    servoCtxList[SHOULDER].currentAngle = 50;
    servoCtxList[ELBOW].pwmCtx          = mraa_pwm_init (PWM_ELBOW);
    servoCtxList[ELBOW].currentAngle    = 160;
    servoCtxList[WHRIST].pwmCtx         = mraa_pwm_init (PWM_WHRIST);
    servoCtxList[WHRIST].currentAngle   = 170;

    mraa_pwm_period_us (servoCtxList[BASE].pwmCtx,     PERIOD_WIDTH);
	mraa_pwm_period_us (servoCtxList[SHOULDER].pwmCtx, PERIOD_WIDTH);
    mraa_pwm_period_us (servoCtxList[ELBOW].pwmCtx,    PERIOD_WIDTH);
    mraa_pwm_period_us (servoCtxList[WHRIST].pwmCtx,   PERIOD_WIDTH);
    
    mraa_pwm_enable (servoCtxList[BASE].pwmCtx,     ENABLE);
	mraa_pwm_enable (servoCtxList[SHOULDER].pwmCtx, ENABLE);
    mraa_pwm_enable (servoCtxList[ELBOW].pwmCtx,    ENABLE);
    mraa_pwm_enable (servoCtxList[WHRIST].pwmCtx,   ENABLE);
    
    printf("Starting the listener... [SUCCESS]\n");

    setAngle (servoCtxList[BASE],      servoCtxList[BASE].currentAngle,     SERVO_SPEED_LOW);
    setAngle (servoCtxList[SHOULDER],  servoCtxList[SHOULDER].currentAngle, SERVO_SPEED_LOW);
    setAngle (servoCtxList[ELBOW],     servoCtxList[ELBOW].currentAngle,    SERVO_SPEED_LOW);
    setAngle (servoCtxList[WHRIST],    servoCtxList[WHRIST].currentAngle,   SERVO_SPEED_LOW);
	
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
                        float coordinateX	= root.get("x", 0).asFloat();
                        float coordinateY	= root.get("y", 0).asFloat();
                        float coordinateZ	= root.get("z", 0).asFloat();
                        float coordinateP	= root.get("p", 0).asFloat();
                        std::cout  	<< "COORDINATE ("
							<< coordinateX << "," << coordinateY << "," 
                            << coordinateZ << "," << coordinateP << ") ";

                        robe.coord.x = coordinateX;
                        robe.coord.y = coordinateY;
                        robe.coord.z = coordinateZ;
                        robe.coord.p = coordinateP;

                        // TODO - Inverse Kinematics
                        if (findAnglesMap (robe)) {
                            // std::cout   << "ANGLES ("
                            // << robe.angles.tn << ", " << robe.angles.j1 << ", " 
                            // << robe.angles.j2 << ", " << robe.angles.j3 << ")";

                            // angles fix
                            // robe.angles.tn += 90;
                            // robe.angles.j1 = 180 - abs(robe.angles.j1);
                            // robe.angles.j2 = (abs(robe.angles.j2) + 90);
                            // robe.angles.j3 = (abs(robe.angles.j3) + 90);

                            std::cout   << "("
                            << robe.angles.tn << ", " << robe.angles.j1 << ", " 
                            << robe.angles.j2 << ", " << robe.angles.j3 << ")\n\n";

                            setAngle (servoCtxList[BASE],     robe.angles_ptr->tn, SERVO_SPEED_LOW);
                            setAngle (servoCtxList[SHOULDER], robe.angles_ptr->j1, SERVO_SPEED_LOW);
                            setAngle (servoCtxList[ELBOW],    robe.angles_ptr->j2, SERVO_SPEED_LOW);
                            setAngle (servoCtxList[WHRIST],   robe.angles_ptr->j3, SERVO_SPEED_LOW);
                        }
                    }
                    break;
                    case SERVO: {
                        int servoID	= root.get("id", 0).asInt();
                        int angle	= root.get("angle", 0).asInt();
                        
                        if (servoID > 0) {
                            std::cout  	<< "SERVO ("
                                        << servoID - 1 << ", " << angle << ")\n";
                            setAngle (servoCtxList[servoID - 1], angle, SERVO_SPEED_LOW);

                            char msg[128];
                            servoMsgFactory (msg, servoID, angle);
                            publish (redisCtx, msg);
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
setAngle (servo_context_t& ctx, int angle, uint8_t speed) {
	float notches = ((float)(MAX_PULSE_WIDTH - MIN_PULSE_WIDTH) / 180);
    int16_t width = notches * (float) angle + MIN_PULSE_WIDTH;

    switch (speed) {
        case SERVO_SPEED_LOW:
            mraa_pwm_pulsewidth_us (ctx.pwmCtx, width);
        break;
        case SERVO_SPEED_MIDDLE:
            mraa_pwm_pulsewidth_us (ctx.pwmCtx, width);
        break;
        case SERVO_SPEED_HIGH:
            mraa_pwm_pulsewidth_us (ctx.pwmCtx, width);
        break;
        default: { // TODO - Somehow to make the speed work
            int delta = abs(angle - ctx.currentAngle);
            int direction = (angle - ctx.currentAngle > 0) ? 1 : -1;

            for (int i = 0; i < delta; i++) {
                width = notches * (float) (ctx.currentAngle + direction) + MIN_PULSE_WIDTH;
                mraa_pwm_pulsewidth_us (ctx.pwmCtx, width);
                ctx.currentAngle += direction;
                usleep (100000);
            }
        }
        break;
    }
}

void
publish (redisContext* ctx, char* buffer) {
    char message[256];
    redisReply* reply = NULL;

    sprintf (message, "PUBLISH MODULE-INFO %s", buffer);
    reply = (redisReply *)redisCommand (ctx, message);
    // printf ("ERROR %d", reply->type);
    freeReplyObject(reply);

    printf ("%s\n", message);
}

void
servoMsgFactory (char* buffer, int id, int angle) {
    sprintf (buffer, "{\"type\":\"SERVO\",\"id\":\"%d\",\"angle\":\"%d\"}", id, angle);
}

uint8_t
findAnglesMap (arm_context_t& ctx) {
    int index = ((ctx.coord.z - 1) * 9) + ((ctx.coord.y - 1) * 3) + ctx.coord.x - 1;
    printf ("index = %d\n", index);
    ctx.angles_ptr = (arm_angles_t*) &angleMap[index];
    
    return YES;
}

uint8_t
calculateAngles (arm_context_t& ctx) {
    float a0, a1, a2, a3, a12, aG;
    float wT, w1, w2, z1, z2, l12;
    
    a0 = atan(ctx.coord.y / ctx.coord.x);
    wT = sqrt(ctx.coord.x*ctx.coord.x + ctx.coord.y*ctx.coord.y);
    
    aG = -0.785398163;
    w2 = wT;
    z2 = ctx.coord.z;
    
    l12 = sqrt((w2*w2) + (z2*z2));
    a12 = atan (z2/w2);
    
    std::cout   << "L12 (" << l12 << ") ";
    /*if (l12 > ctx.coxa + ctx.fermur) {
        return NO;
    }*/
    
    a1 = acos(((ctx.coxa*ctx.coxa) + (l12*l12) - (ctx.fermur*ctx.fermur)) / (2 * ctx.coxa * l12 )) + a12;
    
    w1 = ctx.coxa * cos(a1);
    z1 = ctx.coxa * sin(a1);
    a2 = atan ((z2 - z1) / (w2 - w1)) - a1;
    a3 = aG - a1 - a2;
    
    ctx.angles.tn = a0 * 180 / 3.14;
    ctx.angles.j1 = a1 * 180 / 3.14;
    ctx.angles.j2 = a2 * 180 / 3.14;

    /*float a0, a1, a2, a3, a12, aG;
    float wT, w1, w2, z1, z2, l12;
    
    a0 = atan(ctx.coord.y / ctx.coord.x);
    wT = sqrt(ctx.coord.x*ctx.coord.x + ctx.coord.y*ctx.coord.y);
    
    aG = -0.785398163;
    w2 = wT - ctx.tibia * cos(aG);
    z2 = ctx.coord.z - ctx.tibia * sin(aG);
    
    l12 = sqrt((w2*w2) + (z2*z2));
    a12 = atan (z2/w2);
    
    std::cout   << "L12 (" << l12 << ") ";
    if (l12 > ctx.coxa + ctx.fermur) {
        return NO;
    }
    
    a1 = acos(((ctx.coxa*ctx.coxa) + (l12*l12) - (ctx.fermur*ctx.fermur)) / (2 * ctx.coxa * l12 )) + a12;
    
    w1 = ctx.coxa * cos(a1);
    z1 = ctx.coxa * sin(a1);
    a2 = atan ((z2 - z1) / (w2 - w1)) - a1;
    a3 = aG - a1 - a2;
    
    ctx.angles.tn = a0 * 180 / 3.14;
    ctx.angles.j1 = a1 * 180 / 3.14;
    ctx.angles.j2 = a2 * 180 / 3.14;
    ctx.angles.j3 = a3 * 180 / 3.14;*/

    /*float xt    = ctx.coord.x;
    float l     = sqrt (ctx.coord.x*ctx.coord.x + ctx.coord.y*ctx.coord.y);
    float c     = 0;
    float theta = 0;
    float ang   = 0;
    float x1    = 0;
    float z1    = 0;
    float d     = 0;

    std::cout   << "COORDINATE_T ("
                        << ctx.coord.x << "," << ctx.coord.y << "," 
                        << ctx.coord.z << "," << ctx.coord.p << ")\n";

    // tn angle
    ctx.angles.tn = atan(ctx.coord.y / ctx.coord.x);

    // j2 angle
    ctx.coord.x = l;
    ctx.coord.z += ctx.tibia;
    c = sqrt (ctx.coord.x*ctx.coord.x + ctx.coord.z*ctx.coord.z);
    ctx.angles.j2 = acos ((ctx.fermur*ctx.fermur + ctx.coxa*ctx.coxa - c*c) / (2 * ctx.fermur * ctx.coxa)) * 180 / 3.14;

    // j1 angle
    theta = acos ((c*c + ctx.coxa*ctx.coxa - ctx.fermur*ctx.fermur) / (2 * c * ctx.coxa));
    ang = atan (ctx.coord.z / ctx.coord.x) + theta;
    ctx.angles.j1 = (atan (ctx.coord.z / ctx.coord.x) + theta) * 180 / 3.14;

    // j3 angle
    x1 = ctx.coxa * cos (ang);
    z1 = ctx.coxa * sin (ang);
    d = sqrt ((ctx.coord.x - x1)*(ctx.coord.x - x1) + (ctx.coord.z - ctx.tibia - z1)*(ctx.coord.z - ctx.tibia - z1));
    ctx.angles.j3 = acos ((ctx.fermur*ctx.fermur + ctx.tibia*ctx.tibia - d*d) / (2 * ctx.fermur * ctx.tibia)) * 180 / 3.14;
    ctx.angles.tn = atan(ctx.coord.y / xt) * 180 / 3.14;*/

    return YES;
}
