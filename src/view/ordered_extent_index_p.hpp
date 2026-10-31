#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace qml_material
{
class OrderedExtentIndex {
public:
    explicit OrderedExtentIndex(double spacing = 0): m_spacing(spacing) {}
    OrderedExtentIndex(OrderedExtentIndex&&) noexcept            = default;
    OrderedExtentIndex& operator=(OrderedExtentIndex&&) noexcept = default;

    bool set(int key, double value) {
        if (key < 0 || ! std::isfinite(value) || value < 0 || ! std::isfinite(m_spacing) ||
            m_spacing < 0)
            return false;
        const auto*       old   = find(m_root.get(), key);
        const long double total = sum(m_root.get()) - (old ? old->extent : -m_spacing) + value;
        if (total > std::numeric_limits<double>::max()) return false;
        m_root = insert(std::move(m_root), key, value, m_spacing);
        return true;
    }
    bool erase(int key) {
        if (! find(m_root.get(), key)) return false;
        m_root = remove(std::move(m_root), key, m_spacing);
        return true;
    }
    double extent(int key) const {
        const auto* node = find(m_root.get(), key);
        return node ? node->extent : 0;
    }
    double prefix(int key) const {
        long double result = 0;
        for (auto* node = m_root.get(); node;) {
            if (key <= node->key) {
                node = node->left.get();
            } else {
                result += sum(node->left.get()) + node->extent + m_spacing;
                node = node->right.get();
            }
        }
        return double(result);
    }
    double totalExtent() const {
        return m_root ? double(std::max(0.0L, sum(m_root.get()) - m_spacing)) : 0;
    }
    int first() const { return firstPositive(m_root.get()); }
    int last() const { return lastPositive(m_root.get()); }
    int previous(int key) const { return previousPositive(m_root.get(), key); }
    int next(int key) const { return nextPositive(m_root.get(), key); }

    // Zero-sized rows retain their spacing, but cannot be geometry query results.
    int indexAt(double position) const {
        if (! std::isfinite(position)) return -1;
        const int result = firstAfter(m_root.get(), 0, position);
        return result >= 0 ? result : last();
    }
    int nearestCenter(double position) const {
        if (! std::isfinite(position)) return -1;
        const int after  = lowerCenter(m_root.get(), 0, position);
        const int before = after >= 0 ? previous(after) : last();
        if (before < 0) return after;
        if (after < 0) return before;
        return std::abs(prefix(before) + extent(before) / 2 - position) <=
                       std::abs(prefix(after) + extent(after) / 2 - position)
                   ? before
                   : after;
    }
    std::vector<int> intersecting(double begin, double end,
                                  size_t limit   = std::numeric_limits<size_t>::max(),
                                  bool   reverse = false) const {
        std::vector<int> result;
        if (std::isfinite(begin) && std::isfinite(end) && begin < end && limit)
            collect(m_root.get(), 0, begin, end, limit, reverse, result);
        return result;
    }

private:
    struct Node {
        int                   key;
        double                extent;
        int                   height = 1;
        int                   positive;
        long double           sum;
        std::unique_ptr<Node> left, right;
        Node(int k, double e, double spacing)
            : key(k), extent(e), positive(e > 0), sum(e + static_cast<long double>(spacing)) {}
    };
    static int         height(const Node* node) { return node ? node->height : 0; }
    static int         positive(const Node* node) { return node ? node->positive : 0; }
    static long double sum(const Node* node) { return node ? node->sum : 0; }
    static void        refresh(Node& node, double spacing) {
        node.height   = 1 + std::max(height(node.left.get()), height(node.right.get()));
        node.positive = positive(node.left.get()) + (node.extent > 0) + positive(node.right.get());
        node.sum      = sum(node.left.get()) + node.extent + spacing + sum(node.right.get());
    }
    static std::unique_ptr<Node> rotateLeft(std::unique_ptr<Node> node, double spacing) {
        auto next   = std::move(node->right);
        node->right = std::move(next->left);
        refresh(*node, spacing);
        next->left = std::move(node);
        refresh(*next, spacing);
        return next;
    }
    static std::unique_ptr<Node> rotateRight(std::unique_ptr<Node> node, double spacing) {
        auto next  = std::move(node->left);
        node->left = std::move(next->right);
        refresh(*node, spacing);
        next->right = std::move(node);
        refresh(*next, spacing);
        return next;
    }
    static std::unique_ptr<Node> balance(std::unique_ptr<Node> node, double spacing) {
        if (! node) return node;
        refresh(*node, spacing);
        const int delta = height(node->left.get()) - height(node->right.get());
        if (delta > 1) {
            if (height(node->left->left.get()) < height(node->left->right.get()))
                node->left = rotateLeft(std::move(node->left), spacing);
            return rotateRight(std::move(node), spacing);
        }
        if (delta < -1) {
            if (height(node->right->right.get()) < height(node->right->left.get()))
                node->right = rotateRight(std::move(node->right), spacing);
            return rotateLeft(std::move(node), spacing);
        }
        return node;
    }
    static std::unique_ptr<Node> insert(std::unique_ptr<Node> node, int key, double value,
                                        double spacing) {
        if (! node) return std::make_unique<Node>(key, value, spacing);
        if (key < node->key)
            node->left = insert(std::move(node->left), key, value, spacing);
        else if (key > node->key)
            node->right = insert(std::move(node->right), key, value, spacing);
        else
            node->extent = value;
        return balance(std::move(node), spacing);
    }
    static std::unique_ptr<Node> remove(std::unique_ptr<Node> node, int key, double spacing) {
        if (! node) return node;
        if (key < node->key) {
            node->left = remove(std::move(node->left), key, spacing);
        } else if (key > node->key) {
            node->right = remove(std::move(node->right), key, spacing);
        } else {
            if (! node->left) return std::move(node->right);
            if (! node->right) return std::move(node->left);
            auto* replacement = node->right.get();
            while (replacement->left) replacement = replacement->left.get();
            node->key    = replacement->key;
            node->extent = replacement->extent;
            node->right  = remove(std::move(node->right), replacement->key, spacing);
        }
        return balance(std::move(node), spacing);
    }
    static const Node* find(const Node* node, int key) {
        while (node && node->key != key)
            node = key < node->key ? node->left.get() : node->right.get();
        return node;
    }
    static int firstPositive(const Node* node) {
        if (! positive(node)) return -1;
        if (positive(node->left.get())) return firstPositive(node->left.get());
        if (node->extent > 0) return node->key;
        return firstPositive(node->right.get());
    }
    static int lastPositive(const Node* node) {
        if (! positive(node)) return -1;
        if (positive(node->right.get())) return lastPositive(node->right.get());
        if (node->extent > 0) return node->key;
        return lastPositive(node->left.get());
    }
    static int previousPositive(const Node* node, int key) {
        if (! positive(node)) return -1;
        if (node->key >= key) return previousPositive(node->left.get(), key);
        const int result = previousPositive(node->right.get(), key);
        if (result >= 0) return result;
        return node->extent > 0 ? node->key : lastPositive(node->left.get());
    }
    static int nextPositive(const Node* node, int key) {
        if (! positive(node)) return -1;
        if (node->key <= key) return nextPositive(node->right.get(), key);
        const int result = nextPositive(node->left.get(), key);
        if (result >= 0) return result;
        return node->extent > 0 ? node->key : firstPositive(node->right.get());
    }
    int firstAfter(const Node* node, long double base, double position) const {
        if (! positive(node) || base + sum(node) <= position) return -1;
        const int left = firstAfter(node->left.get(), base, position);
        if (left >= 0) return left;
        const long double start = base + sum(node->left.get());
        if (node->extent > 0 && start + node->extent > position) return node->key;
        return firstAfter(node->right.get(), start + node->extent + m_spacing, position);
    }
    int lowerCenter(const Node* node, long double base, double position) const {
        if (! positive(node)) return -1;
        const long double start = base + sum(node->left.get());
        if (start + node->extent / 2 < position)
            return lowerCenter(node->right.get(), start + node->extent + m_spacing, position);
        const int left = lowerCenter(node->left.get(), base, position);
        if (left >= 0) return left;
        return node->extent > 0 ? node->key : firstPositive(node->right.get());
    }
    void collect(const Node* node, long double base, double begin, double end, size_t limit,
                 bool reverse, std::vector<int>& result) const {
        if (! positive(node) || result.size() >= limit || base >= end || base + sum(node) <= begin)
            return;
        const long double start = base + sum(node->left.get());
        const long double after = start + node->extent + m_spacing;
        if (reverse)
            collect(node->right.get(), after, begin, end, limit, reverse, result);
        else
            collect(node->left.get(), base, begin, end, limit, reverse, result);
        if (result.size() >= limit) return;
        if (node->extent > 0 && start < end && start + node->extent > begin)
            result.push_back(node->key);
        if (reverse)
            collect(node->left.get(), base, begin, end, limit, reverse, result);
        else
            collect(node->right.get(), after, begin, end, limit, reverse, result);
    }
    double                m_spacing;
    std::unique_ptr<Node> m_root;
};
} // namespace qml_material
