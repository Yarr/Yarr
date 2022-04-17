rm -f src/libYarr/include/yarr_version.h

GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
GIT_TAG=$(git describe --tag)
GIT_HASH=$(git describe --always --dirty --abbrev=40 --match="NoTagWithThisName")
GIT_DATE=$(git log -1 --format=%ad --date=iso)
GIT_SUBJECT=$(git log -1 --format=%s)

cat <<EOF > src/libYarr/include/yarr_version.h
#define YARR_GIT_BRANCH "$GIT_BRANCH"
#define YARR_GIT_TAG "$GIT_TAG"
#define YARR_GIT_HASH "$GIT_HASH"
#define YARR_GIT_DATE "$GIT_DATE"
#define YARR_GIT_SUBJECT "$GIT_SUBJECT" 
EOF
