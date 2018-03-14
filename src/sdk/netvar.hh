#pragma once

#include <utility>
#include <vector>

#include "types.hh"

namespace sdk {
class Netvar {

    class Tree {

        struct node;
        using netvar_tree = std::vector<std::pair<const char *, node *>>;

        struct node {
            netvar_tree     children;
            class RecvProp *p;
        };

        netvar_tree prop_tree;

        void populate_recursive(class RecvTable *t, netvar_tree *nodes);

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

    static void init_all();
};
} // namespace sdk
