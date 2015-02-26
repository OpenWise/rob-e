'use strict';

angular.module('app').directive('servo', function($http) {
    return {
        restrict: 'E',
        replace: true,
        scope: {
            data: '=',
			onPlusClick: '&',
			onMinusClick: '&'
        },
        templateUrl: 'views/templates/servo.html'
    }
});
