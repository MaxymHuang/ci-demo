#include "core/Roi.h"

#include <QJsonValue>

namespace xri {

QJsonObject roiToJson(const Roi& roi)
{
    return {
        { "id", roi.id.toString(QUuid::WithoutBraces) },
        { "name", roi.name },
        { "rect",
          QJsonObject{ { "x", roi.rect.x() },
                       { "y", roi.rect.y() },
                       { "w", roi.rect.width() },
                       { "h", roi.rect.height() } } },
        { "algorithmId", roi.algorithmId },
        { "params", QJsonObject::fromVariantMap(roi.params) },
    };
}

Roi roiFromJson(const QJsonObject& obj)
{
    Roi roi;
    roi.id = QUuid::fromString(obj.value("id").toString());
    roi.name = obj.value("name").toString();
    const QJsonObject r = obj.value("rect").toObject();
    roi.rect = QRectF(r.value("x").toDouble(), r.value("y").toDouble(),
                      r.value("w").toDouble(), r.value("h").toDouble());
    roi.algorithmId = obj.value("algorithmId").toString();
    roi.params = obj.value("params").toObject().toVariantMap();
    return roi;
}

} // namespace xri
