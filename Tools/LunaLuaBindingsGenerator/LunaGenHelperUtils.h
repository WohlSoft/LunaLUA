#ifndef LunaGenHelperUtils_hhhhh
#define LunaGenHelperUtils_hhhhh


// Macro Utils
// Found from http://stackoverflow.com/a/1872506/5082374
#define STRINGIZE(arg)  STRINGIZE1(arg)
#define STRINGIZE1(arg) STRINGIZE2(arg)
#define STRINGIZE2(arg) #arg

#define CONCATENATE(arg1, arg2)   CONCATENATE1(arg1, arg2)
#define CONCATENATE1(arg1, arg2)  CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2)  arg1##arg2

#define FOR_EACH_1(what, x, ...) what(x)
#define FOR_EACH_2(what, x, ...)\
  what(x),\
  FOR_EACH_1(what,  __VA_ARGS__)
#define FOR_EACH_3(what, x, ...)\
  what(x),\
  FOR_EACH_2(what,  __VA_ARGS__)
#define FOR_EACH_4(what, x, ...)\
  what(x),\
  FOR_EACH_3(what,  __VA_ARGS__)
#define FOR_EACH_5(what, x, ...)\
  what(x),\
  FOR_EACH_4(what,  __VA_ARGS__)
#define FOR_EACH_6(what, x, ...)\
  what(x),\
  FOR_EACH_5(what,  __VA_ARGS__)
#define FOR_EACH_7(what, x, ...)\
  what(x),\
  FOR_EACH_6(what,  __VA_ARGS__)
#define FOR_EACH_8(what, x, ...)\
  what(x),\
  FOR_EACH_7(what,  __VA_ARGS__)

#define FOR_EACH_NARG(...) FOR_EACH_NARG_(__VA_ARGS__, FOR_EACH_RSEQ_N())
#define FOR_EACH_NARG_(...) FOR_EACH_ARG_N(__VA_ARGS__) 
#define FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N 
#define FOR_EACH_RSEQ_N() 8, 7, 6, 5, 4, 3, 2, 1, 0

#define FOR_EACH_(N, what, x, ...) CONCATENATE(FOR_EACH_, N)(what, x, __VA_ARGS__)
#define FOR_EACH(what, x, ...) FOR_EACH_(FOR_EACH_NARG(x, __VA_ARGS__), what, x, __VA_ARGS__)


#include <string>
#include <luabind/luabind.hpp>

namespace LunaGen 
{
    template<class T, char const* clsName>
    struct LunaGenBaseClassUtils
    {
        using cls = T;

        static luabind::class_<T> defClass() {
            return luabind::class_<T>(clsName);
        }

        static std::string getName(T& cls) {
            return std::string(clsName);
        }

        static const char *getRawName() {
            return clsName;
        }
    };
}





#ifdef __LUNA_CODE_GENERATOR__
#define LUNAGEN_ANNOTATE(elem) annotate(elem)
#define LUNAGEN_ANNOTATE_ELEMS(...) FOR_EACH(LUNAGEN_ANNOTATE, __VA_ARGS__)
#define LUNAGEN_ATTR_ANNOTATE(...) __attribute__((LUNAGEN_ANNOTATE_ELEMS(__VA_ARGS__)))

// 1. LunaGen Option --> LUNAGEN_ATTR_NAMES
//    Alternative names 
#define LUNAGEN_ATTR_NAMES_ELEM(elem) "lunagen_alt_name:" ## elem
#define LUNAGEN_ATTR_NAMES_ELEMS(...) FOR_EACH(LUNAGEN_ATTR_NAMES_ELEM, __VA_ARGS__)
#define LUNAGEN_ATTR_NAMES(...) LUNAGEN_ATTR_ANNOTATE(LUNAGEN_ATTR_NAMES_ELEMS(__VA_ARGS__))

// 2. LunaGEN Option --> LUNAGEN_ATTR_HIDDEN
//    Hide fields and do not generate bindings for it
#define LUNAGEN_ATTR_HIDDEN LUNAGEN_ATTR_ANNOTATE("lunagen_hidden")

#else
#define LUNAGEN_ATTR_NAMES(...)
#define LUNAGEN_ATTR_HIDDEN
#endif

#define LUNAGEN_HELPCLASS_NAME(name) HelperClass_ ## name
#define LUNAGEN_HELPCLASS_STR_NAME(name) _cls_ ## name

#define LUNAGEN_DEF_CLASS_HELPER(classType, name) \
    extern const char LUNAGEN_HELPCLASS_STR_NAME(name) [] = #name ; \
    typedef LunaGen::LunaGenBaseClassUtils< classType , LUNAGEN_HELPCLASS_STR_NAME(name) > LUNAGEN_HELPCLASS_NAME(name) ;

#define LUNAGEN_DEF_CLASS(name) \
    luabind::class_< LUNAGEN_HELPCLASS_NAME(name) ::cls>( LUNAGEN_HELPCLASS_NAME(name) ::getRawName()) \
        .property("__type", & LUNAGEN_HELPCLASS_NAME(name) ::getName)

#define LUNAGEN_DEF_CLASS_SMART_PTR_SHARED(name, smartPtrClass) \
    luabind::class_< LUNAGEN_HELPCLASS_NAME(name) ::cls, smartPtrClass ## < LUNAGEN_HELPCLASS_NAME(name) ::cls > >( LUNAGEN_HELPCLASS_NAME(name) ::getRawName()) \
        .property("__type", & LUNAGEN_HELPCLASS_NAME(name) ::getName)


#endif