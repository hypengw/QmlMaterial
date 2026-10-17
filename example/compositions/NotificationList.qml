pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Item {
    id: root
    implicitHeight: content.implicitHeight
    readonly property alias feed: feed
    property bool animationsEnabled: true
    NotificationFeed {
        id: feed
        objectName: 'notificationFeed'
    }
    Column {
        id: content
        width: parent.width
        spacing: 2
        Repeater {
            model: feed
            delegate: NotificationCard {
                required property int index
                required property string uid
                required property var model
                objectName: 'notification-' + uid
                width: content.width
                animationsEnabled: root.animationsEnabled
                app: model.app
                title: model.title
                message: model.message
                detail: model.detail
                age: model.age
                symbol: model.symbol
                thumbnail: model.thumbnail
                ongoing: model.ongoing
                canReply: model.canReply
                firstInSection: index === 0
                lastInSection: index === feed.count - 1
                onDismissRequested: feed.dismiss(uid)
                onReplyRequested: text => feed.reply(uid, text)
            }
        }
        MD.Text {
            width: parent.width
            height: 96
            visible: feed.count === 0
            text: 'No notifications'
            horizontalAlignment: Text.AlignHCenter
            typescale: MD.Token.typescale.body_large
        }
    }
}
