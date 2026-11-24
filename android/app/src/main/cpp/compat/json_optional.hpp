#pragma once
// nlohmann 3.12 默认开启隐式转换，不生成 std::optional 的 from_json；
// 商城 DTO 的 ShopPrice / maxCount 需要它。用 adl_serializer 偏特化补齐，
// 避免自由函数 from_json 与 nlohmann 内部泛型重载产生二义性。
#include <json.hpp>
#include <optional>

namespace nlohmann {
template <typename T>
struct adl_serializer<std::optional<T>> {
    template <typename BasicJsonType>
    static void from_json(const BasicJsonType& j, std::optional<T>& opt) {
        if (j.is_null()) {
            opt.reset();
        }
        else {
            opt.emplace(j.template get<T>());
        }
    }

    template <typename BasicJsonType>
    static void to_json(BasicJsonType& j, const std::optional<T>& opt) {
        if (opt.has_value()) {
            j = *opt;
        }
        else {
            j = nullptr;
        }
    }
};
}
