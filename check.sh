#!/bin/bash
file_name=$1
parm=$2
current_path=""
check=""
code_path="/usr/src/otc_intel_next-linux"

cd "$code_path" || {
  echo "access $code_path failed"
  return 2
}

#check=$(echo "$file_name" | grep "/")
#if [[ -z "$check" ]]; then
  ./scripts/checkpatch.pl -f "$file_name"
#else
#  current_path=$(pwd)
#  ./scripts/checkpatch.pl -f "$current_path/$file_name"
#fi

if [[ "$parm" == 'a' ]]; then
  echo
  ./scripts/checkpatch.pl "$file_name"
fi
