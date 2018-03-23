#include "precompiled.hh"

#define PLACE_CHECKER

#include "class_id.hh"

#include "interface.hh"
#include "log.hh"
#include "sdk.hh"

using namespace sdk;
using namespace class_id;

internal_checker::ClassIDChecker *internal_checker::ClassIDChecker::head = nullptr;

internal_checker::ClassIDChecker::ClassIDChecker(const char *name, const u32 value) : name(name), intended_value(value) {
    this->name           = name;
    this->intended_value = value;
    next                 = head;
    head                 = this;
}

static auto find_class_id(const char *name) {
    for (auto client_class = IFace<Client>()->get_all_classes();
         client_class != nullptr;
         client_class = client_class->next)
        if (strcmp(client_class->network_name, name) == 0) return client_class->class_id;

    return -1;
}

bool internal_checker::ClassIDChecker::check_correct() {
    auto found_value = find_class_id(name);

    if (found_value == -1) {
        logging::msg("[ClassID] Unable to find correct value for '%s'", name);
        return false;
    }

    if (found_value != intended_value) {
        logging::msg("[ClassID] value for %s is wrong (wanted %d, got %d)", name, intended_value, found_value);
        return false;
    }

    return true;
}

void internal_checker::ClassIDChecker::check_all_correct() {
    for (auto checker = internal_checker::ClassIDChecker::head; checker != nullptr; checker = checker->next) {
        checker->check_correct();
    }
}
