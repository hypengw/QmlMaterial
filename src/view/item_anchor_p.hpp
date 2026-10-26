#pragma once

#include "qml_material/model/item_snapshot.hpp"

namespace qml_material
{
struct ItemAnchor {
    QString key;
    qreal   screenOffset = 0;

    template<class Offset>
    static ItemAnchor capture(const ItemSnapshotPtr& before, const ItemSnapshotPtr& after,
                              int index, qreal position, Offset offset) {
        ItemAnchor result;
        if (! before || ! after || ! before->stableKeys() || ! after->stableKeys() || index < 0)
            return result;
        const auto select = [&](int candidate) {
            const auto& key = before->descriptorAt(candidate).key;
            if (after->indexOfKey(key) < 0) return false;
            result = { key, offset(candidate) - position };
            return true;
        };
        if (! select(index)) {
            for (int i = index + 1; i < before->count() && result.key.isEmpty(); ++i) select(i);
            for (int i = index - 1; i >= 0 && result.key.isEmpty(); --i) select(i);
        }
        return result;
    }
};
} // namespace qml_material
