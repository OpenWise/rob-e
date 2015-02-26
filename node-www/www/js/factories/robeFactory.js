'use strict';

angular.module('app').factory('robeFactory', function($resource, robeService, serverService) {
    return $resource(serverService.server + 'api/set_servo_angle/:id/:angle', null, {
        getRobotDetails: {
            method: "GET",
            interceptor: {
                response: function(response) {
                    return response.data;
                }
            }
        },
        rotateServo: {
            method: "GET",
            interceptor: {
                response: function(response) {
                    // robeService.setValue(response.data);
                    return response.data;
                }
            }
        }
    });
});
