// ═══════════════════════════════════════════════════════════════
// 【示例】最简单的单向链表 —— 仅用于演示可视化框架的用法。
//
// 复习链表时请替换成你自己的实现(双链表/循环链表/带头结点…)。
// 注意:数据结构本身不包含任何绘图/动画代码,保持纯粹。
// ═══════════════════════════════════════════════════════════════
#pragma once

namespace ds {

struct ListNode {
    int data;
    ListNode* next = nullptr;
};

struct LinkedList {
    ListNode* head = nullptr;

    ~LinkedList() { clear(); }

    void clear() {
        while (head) {
            ListNode* p = head;
            head = head->next;
            delete p;
        }
    }

    void pushBack(int v) {
        ListNode** slot = &head;
        while (*slot) slot = &(*slot)->next;
        *slot = new ListNode{v, nullptr};
    }
};

} // namespace ds
