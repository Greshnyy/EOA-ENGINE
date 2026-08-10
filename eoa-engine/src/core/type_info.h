#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <vector>
#include <any>
#include <optional>
#include <variant>
#include <type_traits>
#include <limits>
#include "core/object.h"

namespace eoa {

class Property;
class Function;
class Enum;
class Class;
class ReflectionSystem;

enum class PropertyType { None, Bool, Int, Int8, Int16, Int64, UInt, UInt8, UInt16, UInt64, Float, Double, String, Vec2, Vec3, Vec4, Quat, Mat2, Mat3, Mat4, Color, Object, Enum, Array, Map, Custom, Function };

inline const char* PropertyTypeToString(PropertyType type) {
    switch(type) {
        case PropertyType::Bool:return "Bool"; case PropertyType::Int:return "Int"; case PropertyType::Int8:return "Int8"; case PropertyType::Int16:return "Int16"; case PropertyType::Int64:return "Int64";
        case PropertyType::UInt:return "UInt"; case PropertyType::UInt8:return "UInt8"; case PropertyType::UInt16:return "UInt16"; case PropertyType::UInt64:return "UInt64";
        case PropertyType::Float:return "Float"; case PropertyType::Double:return "Double"; case PropertyType::String:return "String"; case PropertyType::Vec2:return "Vec2"; case PropertyType::Vec3:return "Vec3"; case PropertyType::Vec4:return "Vec4";
        case PropertyType::Quat:return "Quat"; case PropertyType::Mat2:return "Mat2"; case PropertyType::Mat3:return "Mat3"; case PropertyType::Mat4:return "Mat4"; case PropertyType::Color:return "Color"; case PropertyType::Object:return "Object";
        case PropertyType::Enum:return "Enum"; case PropertyType::Array:return "Array"; case PropertyType::Map:return "Map"; case PropertyType::Custom:return "Custom"; case PropertyType::Function:return "Function"; default:return "None";
    }
}

struct PropertyMetadata {
    std::string displayName, category, tooltip, enumType, arrayElementType, group, fileFilter;
    float minValue=std::numeric_limits<float>::lowest(), maxValue=std::numeric_limits<float>::max(), step=0.01f;
    bool readOnly=false, visible=true, advanced=false, isColor=false, hasAlpha=true, isFilePath=false, multiline=false;
    int sortOrder=0, multilineRows=3;
    std::function<bool(const std::any&)> validator;
};
struct FunctionMetadata { std::string displayName, category, tooltip, returnType; bool isPure=false,isStatic=false; std::vector<std::string> paramNames,paramTooltips; };

class Property {
public:
    using GetterFunc=std::function<std::any(void*)>; using SetterFunc=std::function<void(void*,const std::any&)>;
    Property(const std::string& n,PropertyType t,GetterFunc g,SetterFunc s, const PropertyMetadata& m={}):name_(n),type_(t),getter_(std::move(g)),setter_(std::move(s)),metadata_(m){}
    const std::string& GetName()const{return name_;} PropertyType GetType()const{return type_;} const PropertyMetadata& GetMetadata()const{return metadata_;}
    const std::string& GetTypeName()const{return typeName_;} void SetTypeName(const std::string& n){typeName_=n;}
    std::any Get(void* i)const{return getter_&&i?getter_(i):std::any();}
    bool Set(void* i,const std::any& v)const{if(!setter_||!i||metadata_.readOnly)return false;try{setter_(i,v);return true;}catch(const std::bad_any_cast&){return false;}}
    template<typename T> bool IsType()const{
        if constexpr(std::is_same_v<T,bool>)return type_==PropertyType::Bool; else if constexpr(std::is_same_v<T,int>)return type_==PropertyType::Int; else if constexpr(std::is_same_v<T,int8_t>)return type_==PropertyType::Int8; else if constexpr(std::is_same_v<T,int16_t>)return type_==PropertyType::Int16; else if constexpr(std::is_same_v<T,int64_t>)return type_==PropertyType::Int64; else if constexpr(std::is_same_v<T,uint32_t>)return type_==PropertyType::UInt; else if constexpr(std::is_same_v<T,uint8_t>)return type_==PropertyType::UInt8; else if constexpr(std::is_same_v<T,uint16_t>)return type_==PropertyType::UInt16; else if constexpr(std::is_same_v<T,uint64_t>)return type_==PropertyType::UInt64; else if constexpr(std::is_same_v<T,float>)return type_==PropertyType::Float; else if constexpr(std::is_same_v<T,double>)return type_==PropertyType::Double; else if constexpr(std::is_same_v<T,std::string>)return type_==PropertyType::String; else if constexpr(std::is_same_v<T,glm::vec2>)return type_==PropertyType::Vec2; else if constexpr(std::is_same_v<T,glm::vec3>)return type_==PropertyType::Vec3; else if constexpr(std::is_same_v<T,glm::vec4>)return type_==PropertyType::Vec4; else if constexpr(std::is_same_v<T,glm::quat>)return type_==PropertyType::Quat; else if constexpr(std::is_same_v<T,glm::mat2>)return type_==PropertyType::Mat2; else if constexpr(std::is_same_v<T,glm::mat3>)return type_==PropertyType::Mat3; else if constexpr(std::is_same_v<T,glm::mat4>)return type_==PropertyType::Mat4; else return false; }
    bool IsArray()const{return type_==PropertyType::Array;} bool IsEnum()const{return type_==PropertyType::Enum;} const std::string& GetEnumType()const{return metadata_.enumType;}
private: std::string name_,typeName_; PropertyType type_; GetterFunc getter_; SetterFunc setter_; PropertyMetadata metadata_;
};

template<typename C,typename M> std::unique_ptr<Property> MakeProperty(const std::string& name,PropertyType type,M C::*member){return std::make_unique<Property>(name,type,[member](void* o){return std::any(static_cast<C*>(o)->*member);},[member](void* o,const std::any& v){static_cast<C*>(o)->*member=std::any_cast<M>(v);});}
template<typename C,typename M> std::unique_ptr<Property> MakeProperty(const std::string& name,PropertyType type,M C::*member,const PropertyMetadata& meta){return std::make_unique<Property>(name,type,[member](void* o){return std::any(static_cast<C*>(o)->*member);},[member](void* o,const std::any& v){static_cast<C*>(o)->*member=std::any_cast<M>(v);},meta);}
template<typename C,typename M> std::unique_ptr<Property> MakeReadOnlyProperty(const std::string& name,PropertyType type,M C::*member){PropertyMetadata m;m.readOnly=true;return MakeProperty<C,M>(name,type,member,m);}

template<typename C,typename M> std::unique_ptr<Function> MakeFunction(const std::string& name,M, const FunctionMetadata& meta={}) { return std::make_unique<Function>(name,[](void*,const std::vector<std::any>&){return std::any();},meta); }

class Function { public: using FuncPtr=std::function<std::any(void*,const std::vector<std::any>&)>; Function(const std::string& n,FuncPtr f,const FunctionMetadata& m={}):name_(n),func_(std::move(f)),metadata_(m){} const std::string& GetName()const{return name_;} const FunctionMetadata& GetMetadata()const{return metadata_;} std::any Invoke(void* i,const std::vector<std::any>& p={})const{return func_?func_(i,p):std::any();} size_t GetParamCount()const{return metadata_.paramNames.size();} const std::string& GetParamName(size_t i)const{static const std::string e;return i<metadata_.paramNames.size()?metadata_.paramNames[i]:e;} private:std::string name_;FuncPtr func_;FunctionMetadata metadata_;};

class Enum { public: explicit Enum(const std::string& n):name_(n){} const std::string& GetName()const{return name_;} void AddValue(const std::string& n,int64_t v){values_[n]=v;valueToName_[v]=n;} std::optional<int64_t> GetValueByName(const std::string& n)const{auto i=values_.find(n);return i==values_.end()?std::nullopt:std::optional<int64_t>(i->second);} std::string GetNameByValue(int64_t v)const{auto i=valueToName_.find(v);return i==valueToName_.end()?"":i->second;} const std::unordered_map<std::string,int64_t>& GetValues()const{return values_;} size_t GetValueCount()const{return values_.size();} private:std::string name_;std::unordered_map<std::string,int64_t> values_;std::unordered_map<int64_t,std::string> valueToName_;};

class Class { public: using ConstructorFunc=std::function<std::unique_ptr<Object>()>; using DestructorFunc=std::function<void(Object*)>; Class(const std::string& n,const std::string& p=""):name_(n),parentName_(p){} const std::string& GetName()const{return name_;} const std::string& GetParentName()const{return parentName_;} void SetSize(size_t s){size_=s;} size_t GetSize()const{return size_;} void AddProperty(std::unique_ptr<Property> p){if(p)properties_[p->GetName()]=std::move(p);} Property* GetProperty(const std::string& n){auto i=properties_.find(n);return i!=properties_.end()?i->second.get():nullptr;} const Property* GetProperty(const std::string& n)const{auto i=properties_.find(n);return i!=properties_.end()?i->second.get():nullptr;} const std::unordered_map<std::string,std::unique_ptr<Property>>& GetProperties()const{return properties_;} void AddFunction(std::unique_ptr<Function> f){if(f)functions_[f->GetName()]=std::move(f);} Function* GetFunction(const std::string& n){auto i=functions_.find(n);return i!=functions_.end()?i->second.get():nullptr;} const Function* GetFunction(const std::string& n)const{auto i=functions_.find(n);return i!=functions_.end()?i->second.get():nullptr;} const std::unordered_map<std::string,std::unique_ptr<Function>>& GetFunctions()const{return functions_;} void SetConstructor(ConstructorFunc f){constructor_=std::move(f);} void SetDestructor(DestructorFunc f){destructor_=std::move(f);} std::unique_ptr<Object> CreateInstance()const{return constructor_?constructor_():nullptr;} void DestroyInstance(Object* o)const{if(destructor_&&o)destructor_(o);} bool IsChildOf(const std::string& n)const; template<typename T>bool IsChildOf()const{return IsChildOf(T::StaticClassName());} private:std::string name_,parentName_;size_t size_=0;std::unordered_map<std::string,std::unique_ptr<Property>>properties_;std::unordered_map<std::string,std::unique_ptr<Function>>functions_;ConstructorFunc constructor_;DestructorFunc destructor_;};

class ReflectionSystem { public: static ReflectionSystem& Get(){static ReflectionSystem i;return i;} void RegisterClass(std::unique_ptr<Class> c){if(c)classes_[c->GetName()]=std::move(c);} Class* GetClass(const std::string& n){auto i=classes_.find(n);return i!=classes_.end()?i->second.get():nullptr;} const Class* GetClass(const std::string& n)const{auto i=classes_.find(n);return i!=classes_.end()?i->second.get():nullptr;} std::unique_ptr<Object> CreateObject(const std::string& n)const{auto*c=GetClass(n);return c?c->CreateInstance():nullptr;} void RegisterEnum(std::unique_ptr<Enum> e){if(e)enums_[e->GetName()]=std::move(e);} Enum* GetEnum(const std::string& n){auto i=enums_.find(n);return i!=enums_.end()?i->second.get():nullptr;} private:std::unordered_map<std::string,std::unique_ptr<Class>>classes_;std::unordered_map<std::string,std::unique_ptr<Enum>>enums_;};

} // namespace eoa
