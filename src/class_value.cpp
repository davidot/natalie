#include "natalie.hpp"

namespace Natalie {

ClassValue *ClassValue::subclass(Env *env, const char *name) {
    return subclass(env, name, m_object_type);
}

ClassValue *ClassValue::subclass(Env *env, const char *name, Type object_type) {
    ClassValue *subclass = new ClassValue { env, klass() };
    subclass->m_env.init_detached(m_env);
    if (singleton_class()) {
        auto singleton_name = String::format("#<Class:{}>", name ? name : "");
        ClassValue *singleton = singleton_class()->subclass(env, singleton_name->c_str());
        subclass->set_singleton_class(singleton);
    }
    subclass->set_class_name(name);
    subclass->m_superclass = this;
    subclass->m_object_type = object_type;
    return subclass;
}

ClassValue *ClassValue::bootstrap_class_class(Env *env) {
    ClassValue *Class = new ClassValue {
        env,
        reinterpret_cast<ClassValue *>(-1)
    };
    Class->m_klass = Class;
    Class->set_class_name("Class");
    return Class;
}

ClassValue *ClassValue::bootstrap_basic_object(Env *env, ClassValue *Class) {
    ClassValue *BasicObject = new ClassValue {
        env,
        reinterpret_cast<ClassValue *>(-1)
    };
    BasicObject->m_klass = Class;
    BasicObject->m_superclass = nullptr;
    BasicObject->set_class_name("BasicObject");
    BasicObject->set_singleton_class(Class->subclass(env, "#<Class:BasicObject>"));
    return BasicObject;
}

}
