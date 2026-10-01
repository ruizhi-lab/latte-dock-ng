import QtQuick
import QtTest
import "../../declarativeimports/components" as LatteComponents

TestCase {
    id: testCase
    name: "LatteScrollAreaWheel"
    when: windowShown
    visible: true
    width: 240
    height: 240

    property int upCount: 0
    property int downCount: 0

    LatteComponents.ScrollArea {
        id: area
        width: 180
        height: 180
        delay: 75
        onScrolledUp: function(wheel) { testCase.upCount++ }
        onScrolledDown: function(wheel) { testCase.downCount++ }
    }

    function test_directionThrottleAndThreshold() {
        waitForRendering(area)
        mouseWheel(area, 40, 40, 0, 120)
        compare(area.wheelIsBlocked, true)
        compare(upCount, 1)
        compare(downCount, 0)

        mouseWheel(area, 40, 40, 0, -120)
        compare(downCount, 0)

        wait(100)
        mouseWheel(area, 40, 40, 0, -120)
        compare(downCount, 1)

        wait(100)
        mouseWheel(area, 40, 40, 0, 96)
        compare(upCount, 1)
    }
}
