'use strict';

angular.module('app').controller('robeController', function($scope, robeFactory, robeService, serverService, $http) {
    $scope.data = [];
    
    $scope.robot = {
        name:   "robe",
        servos: 4
    }

    $scope.servoBase = {
        name: "base",
        id: 1,
        angle: 90,
		xLane: 100,
		yLane: 50,
    }
	
	$scope.servoShoulder = {
        name: "shoulder",
        id: 2,
        angle: 90,
		xLane: 100,
		yLane: 60,
    }
    
    $scope.servoElbow = {
        name: "elbow",
        id: 3,
        angle: 90,
		xLane: 100,
		yLane: 70,
    }

    var eventSourceCallback = function(idx) {
        
    }

    $scope.plusClick = function(servo) {
        if (servo.angle < 180) {
            servo.angle += 5;
            robeFactory.rotateServo({
                id: servo.id,
                angle: servo.angle
            });
            // rotateServo($http, servo.id, servo.angle);
        }
    }
	
	$scope.minusClick = function(servo) {
        if (servo.angle > 0) {
            servo.angle -= 5;
            robeFactory.rotateServo({
                id: servo.id,
                angle: servo.angle
            });
            // rotateServo($http, servo.id, servo.angle);
        }
    }
    
    var rotateServo = function ($http, servoId, angle) {
        $http.get(serverService.server + 'api/set_servo_angle/' + servoId + '/' + angle).
        success(function(data, status, headers, config) {}).
        error(function(data, status, headers, config) {});
    }
});
