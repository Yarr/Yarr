#include <string>
#include "storage.hpp"
#include "yarr_version.h"

namespace yarr {
namespace  version {
   static const std::string git_branch=YARR_GIT_BRANCH;
   static const std::string git_tag=YARR_GIT_TAG;
   static const std::string git_hash=YARR_GIT_HASH;
   static const std::string git_date=YARR_GIT_DATE;
   static const std::string git_subject=YARR_GIT_SUBJECT;
   json get() {
      json j;
      j["git_branch"] = git_branch;
      j["git_tag"] = git_tag;
      j["git_hash"] = git_hash;
      j["git_date"] = git_date;
      j["git_subject"] = git_subject;
      return j;
   }
}
}
