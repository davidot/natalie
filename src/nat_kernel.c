#include "natalie.h"
#include "nat_kernel.h"

NatObject *Kernel_puts(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    if (argc == 0) {
        printf("\n");
    } else {
        NatObject *str;
        for (size_t i=0; i<argc; i++) {
            str = nat_send(env, args[i], "to_s", 0, NULL);
            assert(str->type == NAT_VALUE_STRING);
            printf("%s\n", str->str);
        }
    }
    return env_get(env, "nil");
}

NatObject *Kernel_print(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    if (argc > 0) {
        NatObject *str;
        for (size_t i=0; i<argc; i++) {
            str = nat_send(env, args[i], "inspect", 0, NULL);
            assert(str->type == NAT_VALUE_STRING);
            printf("%s", str->str);
        }
    }
    return env_get(env, "nil");
}

NatObject *Kernel_p(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs) {
    if (argc == 0) {
        return env_get(env, "nil");
    } else {
        NatObject *result = nat_array(env);
        for (size_t i=0; i<argc; i++) {
            nat_array_push(result, args[i]);
            args[i] = nat_send(env, args[i], "inspect", 0, NULL);
        }
        Kernel_puts(env, self, argc, args, kwargs);
        if (argc == 1) {
            return result->ary[0];
        } else {
            return result;
        }
    }
}