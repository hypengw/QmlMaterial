import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 620
    height: 380
    color: MD.Token.color.surface
    Column {
        x: 24
        y: 24
        spacing: 28
        MD.TextField {
            width: 280
            type: MD.Enum.TextFieldFilled
            placeholderText: 'Amount'
            prefix: '$'
            suffix: 'USD'
            text: '25'
            supportingText: 'Amount before tax'
        }
        MD.TextField {
            width: 280
            placeholderText: 'Username'
            text: 'A long username'
            error: true
            errorText: 'The username is too long. Choose a shorter name.'
        }
        MD.TextField {
            width: 280
            type: MD.Enum.TextFieldFilled
            placeholderText: 'Disabled'
            text: '123'
            error: true
            enabled: false
            supportingText: 'Supporting text'
        }
    }
    Column {
        x: 340
        y: 24
        spacing: 28
        MD.TextField {
            width: 256
            placeholderText: 'Amount'
            prefix: '$'
            suffix: 'USD'
            text: '25'
            supportingText: 'Mirrored field'
            LayoutMirroring.enabled: true
        }
        MD.TextField {
            width: 256
            type: MD.Enum.TextFieldFilled
            placeholderText: 'Amount'
            prefix: '$'
            suffix: 'USD'
            supportingText: 'Affixes hidden while the label rests'
        }
    }
}
