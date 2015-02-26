'use strict';

angular.module('app').controller('mainController', function($scope, $http, cordovaService) {
    $scope.ready = false;
    $scope.isCordovaApp = undefined;

    cordovaService.ready.then(
        function resolved(resp) {
            $scope.isCordovaApp = true;
            $scope.ready = true;
        },
        function rejected(resp) {
            $scope.isCordovaApp = false;
            $scope.ready = true;
        }
    );
});
