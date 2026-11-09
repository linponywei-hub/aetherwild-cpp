#pragma once
#include "aetherwild/domain.hpp"
namespace aw::auth_detail {
inline bool sameCode(const std::string& supplied,const char* expected) {
            if(!expected||!*expected||supplied.size()>512)return false;
            std::string configured(expected);
            std::size_t difference=supplied.size()^configured.size();
            for(std::size_t i=0;i<std::max(supplied.size(),configured.size());++i)
                difference|=(i<supplied.size()?static_cast<unsigned char>(supplied[i]):0)^(i<configured.size()?static_cast<unsigned char>(configured[i]):0);
            return difference==0;
        }
}
