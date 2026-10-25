#pragma once
#include <QString>
#include <QObject>
#include <QMetaType>
#include <QtQml/qqmlregistration.h>

class Track {
    Q_GADGET
    QML_VALUE_TYPE(track)
    Q_PROPERTY(QString title MEMBER title)
public:
    QString title;
};
Q_DECLARE_METATYPE(Track)
