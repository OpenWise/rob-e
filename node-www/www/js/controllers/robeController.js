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
        minBorder: 0,
        maxBorder: 180,
    }
	
	$scope.servoShoulder = {
        name: "shoulder",
        id: 2,
        angle: 50,
		xLane: 100,
		yLane: 60,
        minBorder: 40,
        maxBorder: 180,
    }
    
    $scope.servoElbow = {
        name: "elbow",
        id: 3,
        angle: 160,
		xLane: 100,
		yLane: 70,
        minBorder: 0,
        maxBorder: 180,
    }

    $scope.servoWhrist = {
        name: "whrist",
        id: 4,
        angle: 170,
        xLane: 100,
        yLane: 80,
        minBorder: 0,
        maxBorder: 180,
    }

    var eventBaseSourceCallback = function() {
        return function (event) {
            var msg = JSON.parse(event.data);
            $scope.servoBase.angle = msg.angle;
            $scope.$apply();
        }
    }

    var eventShoulderSourceCallback = function() {
        return function (event) {
            var msg = JSON.parse(event.data);
            $scope.servoShoulder.angle = msg.angle;
            $scope.$apply();
        }
    }

    var eventElbowSourceCallback = function() {
        return function (event) {
            var msg = JSON.parse(event.data);
            $scope.servoElbow.angle = msg.angle;
            $scope.$apply();
        }
    }

    var eventWhristSourceCallback = function() {
        return function (event) {
            var msg = JSON.parse(event.data);
            $scope.servoWhrist.angle = msg.angle;
            $scope.$apply();
        }
    }

    var servoBaseSource             = new EventSource(serverService.server + "sse/servo/" + $scope.servoBase.id);
    servoBaseSource.onmessage       = eventBaseSourceCallback();
    var servoShoulderSource         = new EventSource(serverService.server + "sse/servo/" + $scope.servoShoulder.id);
    servoShoulderSource.onmessage   = eventShoulderSourceCallback();
    var servoElbowSource            = new EventSource(serverService.server + "sse/servo/" + $scope.servoElbow.id);
    servoElbowSource.onmessage      = eventElbowSourceCallback();
    var servoWhristSource           = new EventSource(serverService.server + "sse/servo/" + $scope.servoWhrist.id);
    servoWhristSource.onmessage     = eventWhristSourceCallback();

    $scope.plusClick = function(servo) {
        if (servo.angle < servo.maxBorder) {
            var newAngle = parseInt(servo.angle) + 5;
            robeFactory.rotateServo({
                id: servo.id,
                angle: newAngle
            });
            // rotateServo($http, servo.id, servo.angle);
        }
    }
	
	$scope.minusClick = function(servo) {
        if (servo.angle > servo.minBorder) {
            var newAngle = parseInt(servo.angle) - 5;
            robeFactory.rotateServo({
                id: servo.id,
                angle: newAngle
            });
            // rotateServo($http, servo.id, servo.angle);
        }
    }

    $scope.setPosistion = function(x,y,z,p) {
        
    }
    
    // NOTE - Not for production, only for tests
    var rotateServo = function ($http, servoId, angle) {
        $http.get(serverService.server + 'api/set_servo_angle/' + servoId + '/' + angle).
        success(function(data, status, headers, config) {}).
        error(function(data, status, headers, config) {});
    }
});
