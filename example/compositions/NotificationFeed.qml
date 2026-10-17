import QtQuick
import Qcm.Material as MD

ListModel {
    id: root
    function indexOf(uid) {
        for (let i = 0; i < count; ++i) {
            if (get(i).uid === uid)
                return i;
        }
        return -1;
    }
    function dismiss(uid) {
        const index = indexOf(uid);
        if (index >= 0 && !get(index).ongoing) {
            remove(index);
        }
    }
    function reply(uid, text) {
        const index = indexOf(uid);
        const value = text.trim();
        if (index < 0 || !get(index).canReply || !value.length)
            return;
        setProperty(index, 'message', 'You: ' + value);
        setProperty(index, 'detail', 'You: ' + value);
        setProperty(index, 'age', 'Now');
    }
    function reset() {
        clear();
        append([
            {
                uid: 'message',
                app: 'Messages',
                title: 'Alex Morgan',
                message: 'Meet at the studio at 4?',
                detail: 'The new prints are ready. Meet at the studio at 4? I can bring the samples.',
                age: '2 min',
                symbol: MD.Token.icon.chat_bubble,
                thumbnail: '',
                ongoing: false,
                canReply: true
            },
            {
                uid: 'calendar',
                app: 'Calendar',
                title: 'Design review',
                message: 'Today, 4:00 - 4:30 PM',
                detail: 'Today, 4:00 - 4:30 PM\nStudio meeting room\nBring the updated colour studies.',
                age: '12 min',
                symbol: MD.Token.icon.event,
                thumbnail: '',
                ongoing: false,
                canReply: false
            }
        ]);
    }
    Component.onCompleted: reset()
}
