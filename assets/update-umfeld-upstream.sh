#!/usr/bin/env bash

# NOTE run this script from the toplevel of the working tree.

git fetch umfeld-upstream
git subtree pull --prefix=umfeld/cores/sdl/umfeld umfeld-upstream main --squash

git branch -D umfeld-split 2>/dev/null || true
git subtree split --prefix=umfeld/cores/sdl/umfeld -b umfeld-split
git push umfeld-upstream umfeld-split:main
git branch -D umfeld-split

#git push

# check upstream for new commits:
# ```sh
# git log umfeld-upstream/main --oneline | head
# ```

# push changes back upstream (if you modify the vendored code):
# ```sh
# git subtree split --prefix=umfeld/cores/sdl/umfeld -b umfeld-split
# git push umfeld-upstream umfeld-split:main
# ```
