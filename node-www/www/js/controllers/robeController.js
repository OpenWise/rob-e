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

    var servoBaseSource             = new EventSource(serverService.server + "sse/servo/" + $scope.servoBase.id);
    servoBaseSource.onmessage       = eventBaseSourceCallback();
    var servoShoulderSource         = new EventSource(serverService.server + "sse/servo/" + $scope.servoShoulder.id);
    servoShoulderSource.onmessage   = eventShoulderSourceCallback();
    var servoElbowSource            = new EventSource(serverService.server + "sse/servo/" + $scope.servoElbow.id);
    servoElbowSource.onmessage      = eventElbowSourceCallback();

    $scope.plusClick = function(servo) {
        if (servo.angle < 180) {
            var newAngle = parseInt(servo.angle) + 5;
            robeFactory.rotateServo({
                id: servo.id,
                angle: newAngle
            });
            // rotateServo($http, servo.id, servo.angle);
        }
    }
	
	$scope.minusClick = function(servo) {
        if (servo.angle > 0) {
            var newAngle = parseInt(servo.angle) - 5;
            robeFactory.rotateServo({
                id: servo.id,
                angle: newAngle
            });
            // rotateServo($http, servo.id, servo.angle);
        }
    }
    
    // NOTE - Not for production, only for tests
    var rotateServo = function ($http, servoId, angle) {
        $http.get(serverService.server + 'api/set_servo_angle/' + servoId + '/' + angle).
        success(function(data, status, headers, config) {}).
        error(function(data, status, headers, config) {});
    }
});
