#pragma once

#include <cassert>

#include "types.hh"

namespace sdk {
class RecvTable;
class RecvProp;
class RecvDecoder;

struct RecvProxyData {
public:
    const RecvProp *prop;      // The property it's receiving.
    u8              value[16]; // The value given to you to store.
    int             element;   // Which array element you're getting.
    int             id;        // The object being referred to.
};

class RecvProp {
public:
    using RecvVarProxyFn          = void(const RecvProxyData *data, void *this_ptr, void *out);
    using ArrayLengthRecvProxyFn  = void(void *this_ptr, int id, int cur_length);
    using DataTableRecvVarProxyFn = void(const RecvProp *prop, void **out, void *data, int id);

    RecvProp() = delete;

    auto as_datatable() const {
        return data_table;
    }

    auto as_array() const {
        return array_prop;
    }

    auto is_inside_array() const {
        return inside_array;
    }

    // If it's one of the numbered "000", "001", etc properties in an array, then
    // these can be used to get its array property name for debugging.
    auto parent_name() {
        return parent_array_name;
    }

    // Member ordering must remain the same!

public:
    const char *network_name;
    int         recv_type;
    int         flags;
    int         string_buffer_size;

    bool inside_array; // Set to true by the engine if this property sits inside an array.

    // Extra data that certain special property types bind to the property here.
    const void *extra_data;

    // If this is an array (DPT_Array).
    RecvProp *array_prop;

    ArrayLengthRecvProxyFn *array_length_proxy;

    RecvVarProxyFn *proxy_fn;

    DataTableRecvVarProxyFn *datatable_proxy_fn; // For RDT_DataTable.
    RecvTable *              data_table;         // For RDT_DataTable.
    int                      offset;

    int element_stride;
    int element_count;

private:
    // If it's one of the numbered "000", "001", etc properties in an array, then
    // these can be used to get its array property name for debugging.
    const char *parent_array_name;
};

class RecvTable {
public:
    RecvTable() = delete;

    auto prop(int i) {
        assert(i < prop_count);
        return &props[i];
    }

    // C++11 iterator functions
    auto begin() { return prop(0); }
    auto end() { return props + prop_count; }

public:
    // Properties described in a table.
    RecvProp *props;
    int       prop_count;

    // The decoder. NOTE: this covers each RecvTable AND all its children (ie: its children
    // will have their own decoders that include props for all their children).
    RecvDecoder *decoder;

    const char *name; // The name matched between client and server.
};
} // namespace sdk
