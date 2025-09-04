#!/usr/bin/env bash

git fetch umfeld-upstream
git subtree pull --prefix=umfeld/cores/sdl/umfeld umfeld-upstream main --squash

# check upstream for new commits:
# ```sh
# git log umfeld-upstream/main --oneline | head
# ```

# push changes back upstream (if you modify the vendored code):
# ```sh
# git subtree split --prefix=umfeld/cores/sdl/umfeld -b umfeld-split
# git push umfeld-upstream umfeld-split:main
# ```
