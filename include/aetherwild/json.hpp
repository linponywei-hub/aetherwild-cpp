#pragma once
#include "domain.hpp"
#include <charconv>
#include <iomanip>
#include <limits>
#include <sstream>
#include <variant>
namespace aw {
    class Json {
        public: using Array=std::vector<Json>;
        using Object=std::map<std::string,Json>;
        using Value=std::variant<std::monostate,bool,std::int64_t,double,std::string,Array,Object>;
        Value value;
        Json()=default;
        Json(bool v):value(v) {}
        template<class T> requires(std::is_integral_v<T>&&!std::is_same_v<T,bool>) Json(T v):value(static_cast<std::int64_t>(v)) {}
        Json(double v):value(v) {
            require(std::isfinite(v),"JSON數值必須有限");
        }
        Json(const char*v):value(std::string(v)) {}
        Json(std::string v):value(std::move(v)) {}
        Json(Array v):value(std::move(v)) {}
        Json(Object v):value(std::move(v)) {}
        Json(std::initializer_list<std::pair<const std::string,Json>> v):value(Object(v)) {}
        static Json array() {
            return Array {};
        }
        static Json object() {
            return Object {};
        }
        bool contains(const std::string&k)const {
            return std::holds_alternative<Object>(value)&&std::get<Object>(value).contains(k);
        }
        const Json& at(const std::string&k)const {
            require(contains(k),"缺少欄位："+k);
            return std::get<Object>(value).at(k);
        }
        Json& operator[](const std::string&k) {
            if(std::holds_alternative<std::monostate>(value))value=Object {};
            require(std::holds_alternative<Object>(value),"JSON不是物件");
            return std::get<Object>(value)[k];
        }
        const Array& arrayItems()const {
            require(std::holds_alternative<Array>(value),"JSON不是陣列");
            return std::get<Array>(value);
        }
        const Object& objectItems()const {
            require(std::holds_alternative<Object>(value),"JSON不是物件");
            return std::get<Object>(value);
        }
        void push(Json j) {
            require(std::holds_alternative<Array>(value),"JSON不是陣列");
            std::get<Array>(value).push_back(std::move(j));
        }
        std::string string()const {
            require(std::holds_alternative<std::string>(value),"JSON不是字串");
            return std::get<std::string>(value);
        }
        double number()const {
            if(auto p=std::get_if<double>(&value))return *p;
            if(auto p=std::get_if<std::int64_t>(&value))return static_cast<double>(*p);
            throw RuleError("JSON不是數值");
        }
        std::int64_t integer()const {
            if(auto p=std::get_if<std::int64_t>(&value))return *p;
            double n=number();
            require(std::trunc(n)==n&&std::abs(n)<9007199254740992.0,"JSON整數範圍錯誤");
            return static_cast<std::int64_t>(n);
        }
        bool boolean()const {
            require(std::holds_alternative<bool>(value),"JSON不是布林值");
            return std::get<bool>(value);
        }
        std::string str(const std::string&k,std::string d="")const {
            return contains(k)?at(k).string():d;
        }
        double num(const std::string&k,double d=0)const {
            return contains(k)?at(k).number():d;
        }
        bool flag(const std::string&k,bool d=false)const {
            return contains(k)?at(k).boolean():d;
        }
        std::string dump()const;
        static Json parse(const std::string&s);
    };
}
