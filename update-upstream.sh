#!/usr/bin/env bash

git fetch umfeld-upstream
git subtree pull --prefix=umfeld/cores/sdl/umfeld umfeld-upstream main --squash
