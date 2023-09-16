#!/bin/bash

# This script goes through a codebase and identifies "approved callsites"
# AKA calls to ~dangerous~ functions that have a comment above them
# saying you intended to make this call.
#
# These calls are indexed into a file which then the ruby interpreter
# loads into memory at startup. This index is consulted every time one
# of these ~dangerous~ functions is called. Anything not in this index
# triggers a SecurityError.
#
# To generate the index:
#   ./callsite_index $APP_ROOT > callsites.txt
#
# The interpreter will load callsites.txt when RUBY_HARDEN is set to YES.

APP_ROOT="$1"

if [ "$APP_ROOT" == "" ]
then
    APP_ROOT="."
fi

pushd "$APP_ROOT" >/dev/null
rg -n -g '*.rb' '\#[ ]*muscular_allow' | awk -F: '{print $1 ":" $2+1}'
popd >/dev/null
