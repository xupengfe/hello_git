#!/bin/bash

path=$1
author=""
author_list=""
commit_num=""
ddt_summary="/tmp/ddt_summary.log"
ddt_log="/tmp/ddt.log"

usage() {
  echo "usage: ${0##*/} git_folder_path"
  exit 0
}

git_check() {
  cd $path
  [[ $? -eq 0 ]] || {
    echo "No  $path for git summary"
    usage
    return 1
  }

  cat /dev/null > "$ddt_summary"

  author_list=$(git log \
                | grep  "^Author: " \
                | grep intel \
                | uniq \
                | head -n 200 \
                | sort \
                | uniq)

  #echo "$author_list" | while read -r author; do echo $a; done
  echo '-------------------'
  printf "%8s %s\n" "Commit_NB" "Author"
  IFS=$'\n'
  for author in ${author_list}; do
    au=$(echo "$author" | cut -d "<" -f 1 | cut -d ":" -f 2)
    commit_num=$(git log | grep $author | wc -l)
    printf "%8s %s\n" "$commit_num" "$au" >> "$ddt_summary"
  done

  cat "$ddt_summary" | sort -frn
  cat "$ddt_summary" | sort -frn > "$ddt_log"

  auth=$(cat "$ddt_log" | awk -F "  " '{print $NF}')

  IFS=$'\n'
  for a in $auth
  do
      echo '-------------------'
      echo "Statistics for: $a"
      echo -n "Number of files changed: "
      git log --all --numstat --format="%n" --author=$a | grep -v -e "^$" | cut -f3 | sort -iu | wc -l
      echo -n "Number of lines added: "
      git log --all --numstat --format="%n" --author=$a | cut -f1 | awk '{s+=$1} END {print s}'
      echo -n "Number of lines deleted: "
      git log --all --numstat --format="%n" --author=$a | cut -f2 | awk '{s+=$1} END {print s}'
      echo -n "Number of merges: "
      git log --all --merges --author=$a | grep -c '^commit'
  done
}

[[ -z "$path" ]] && path="./otc_kernel_qa-ts_ltp_ddt"
git_check
