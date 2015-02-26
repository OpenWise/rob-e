'use strict';

var app = angular.module('app', ['ngRoute', 'ngResource']);

app.config(['$routeProvider',
    function($routeProvider) {
        $routeProvider
            .when('/', {
                templateUrl: "/views/robe_control.html",
                controller: 'robeController'
            }).when('/robe', {
                templateUrl: "/views/robe_control.html",
                controller: 'robeController'
            })
            .otherwise({
                redirectTo: '/'
        });
    }
]);
