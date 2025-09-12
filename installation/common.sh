: "${REPO:=dennisppaul/umfeld}"   # allow forks: REPO=user/fork
: "${UMFELD_REF:=main}"           # branch | tag | commit
: "${BASE_RAW:=https://raw.githubusercontent.com}"

BASE_URL="${BASE_URL:-$BASE_RAW/$REPO/$UMFELD_REF/installation}"
BUNDLE_URL="${BUNDLE_URL:-$BASE_URL/Brewfile}"