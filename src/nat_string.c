#include "natalie.h"
#include "nat_string.h"

NatObject *String_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_STRING);
    return self;
}

NatObject *String_ltlt(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_STRING);
    assert(argc == 1);
    NatObject *arg = args[0];
    char *str;
    if (arg->type == NAT_VALUE_STRING) {
        str = arg->str;
    } else {
        NatObject *str_obj = nat_send(env, arg, "to_s", 0, NULL);
        assert(str_obj->type == NAT_VALUE_STRING);
        str = str_obj->str;
    }
    nat_string_append(self, str);
    return self;
}

NatObject *String_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    assert(self->type == NAT_VALUE_STRING);
    NatObject *out = nat_string(env, "\"");
    for (size_t i=0; i<self->str_len; i++) {
        // FIXME: iterate over multibyte chars
        char c = self->str[i];
        if (c == '"' || c == '\\') {
            nat_string_append_char(out, '\\');
            nat_string_append_char(out, c);
        } else {
            nat_string_append_char(out, c);
        }
    }
    nat_string_append_char(out, '"');
    return out;
}
