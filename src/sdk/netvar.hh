#pragma once

#include <utility>
#include <vector>

#include "types.hh"

namespace sdk {
class Netvar {

    class Tree {

        struct Node;
        using TreeNode = std::vector<std::pair<const char *, std::shared_ptr<Node>>>;

        struct Node {
            TreeNode        children;
            class RecvProp *p;
        };

        TreeNode prop_tree;

        void populate_recursive(class RecvTable *t, TreeNode *nodes);

    public:
        Tree();

        void init();

        uptr find_offset(std::vector<const char *> t);
    };
    static Tree netvar_tree;

    // the offset of this netvar from its parent instance
    uptr offset;

    // used internally for registering
    Netvar *next;

    std::vector<const char *> tables;

    void add_to_init_list();
    void init();

public:
    template <typename... A>
    Netvar(A... args) : tables({args...}) {
        add_to_init_list();
    }

    template <typename T>
    auto &get(void *instance) {
        return *reinterpret_cast<T *>(static_cast<char *>(instance) + offset);
    }

    void offset_delta(u32 d) { offset += d; }

    static void init_all();
};
} // namespace sdk
