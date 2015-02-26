'use strict';

angular.module('app').service('cordovaService', ['$document', '$timeout', '$window', '$q',
    function($document, $timeout, $window, $q) {

        var defer = $q.defer();

        this.ready = defer.promise;
        this.isCordovaDevice = undefined;

        var timeoutPromise = $timeout(function() {
            if ($window.cordova) {
                defer.resolve($window.cordova);
            } else {
                defer.reject("Cordova failed to load");
        }}, 1200);

        angular.element($document)[0].addEventListener('deviceready', function() {
            $timeout.cancel(timeoutPromise);
            defer.resolve($window.cordova);
        });

        var self = this;
        this.ready.then(
            function resolved(resp) {
                self.isCordovaDevice = true;
            },
            function rejected(resp) {
                self.isCordovaDevice = false;
            }
        )
    }
]);
