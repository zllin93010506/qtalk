// JavaScript source code
var myApp = angular.module('phrApp', []);

myApp.value('UserData', {
    list1: [{ name: "filbert", number: 123456 },
             { name: "vvn", number: 987654 }],
    list2: [{ name: "nicky", number: 123456 },
            { name: "tommy", number: 987654 },
            { name: "kiky", number: 245697 }],
    GetData: function () {
        return [{ name: "aaa", number: 123456 },
             { name: "bbb", number: 987654 }];
    },
    GetDate: function (FirstDay) {
        var week = [FirstDay];
        for (var i = 1; i < 7; i++) {
            var d = new Date();
            d.setTime(FirstDay.getTime() - i * 86400000);
            week[i] = d;
        }
            
        return week;
    }
});

myApp.controller('phr_controller', ['$scope', 'UserData', function ($scope, UserData) {

    $scope.CurrentDay = new Date();

    $scope.CurrentWeek = UserData.GetDate($scope.CurrentDay);

    $scope.connList = UserData.GetData();

    $scope.NextWeekEnable = false;

    $scope.init = function () {
        $scope.connList = UserData.GetData();
        $scope.CurrentDay = new Date();
        $scope.CurrentWeek = UserData.GetDate($scope.CurrentDay);
        $scope.NextWeekEnable = false;
        $scope.$apply();
    };

    $scope.NextWeek = function () {
        $scope.CurrentDay.setDate($scope.CurrentDay.getDate() + 7);
        $scope.CurrentWeek = UserData.GetDate($scope.CurrentDay);
        checkDate();
    }

    $scope.LastWeek = function () {
        $scope.CurrentDay.setDate($scope.CurrentDay.getDate() - 7);
        $scope.CurrentWeek = UserData.GetDate($scope.CurrentDay);
        checkDate();
    }

    function checkDate() {
        var dd = new Date();
        if ($scope.CurrentDay.getFullYear() == dd.getFullYear()
            && $scope.CurrentDay.getMonth() == dd.getMonth()
            && $scope.CurrentDay.getDate() == dd.getDate()) {
            $scope.NextWeekEnable = false;
        } else {
            $scope.NextWeekEnable = true;
        }
    }

}]);

