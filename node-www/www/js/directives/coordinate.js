'use strict';

angular.module('app').directive('coordinate', function($http) {
    return {
        restrict: 'E',
        replace: true,
        scope: {
            data: '=',
			onSetPositionClick: '&'
        },
        templateUrl: 'views/templates/coordinate.html'
    }
});