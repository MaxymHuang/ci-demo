#pragma once

#include <QJsonObject>
#include <QRectF>
#include <QString>
#include <QUuid>
#include <QVariantMap>

namespace xri {

// One inspection box, in image pixel coordinates.
struct Roi {
    QUuid id = QUuid::createUuid();
    QString name;
    QRectF rect;
    QString algorithmId; // empty = no algorithm assigned
    QVariantMap params;

    bool operator==(const Roi& other) const
    {
        return id == other.id && name == other.name && rect == other.rect
            && algorithmId == other.algorithmId && params == other.params;
    }
};

QJsonObject roiToJson(const Roi& roi);
Roi roiFromJson(const QJsonObject& obj);

} // namespace xri
