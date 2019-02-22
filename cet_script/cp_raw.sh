#!/bin/bash

cet_file="/home/simics/Simics_Farm/craff/clear/simics_fedora28_CET_5.0-rc6_intel-next_0213.raw"
origin_file="/home/simics/cet_0912.raw"
result=""


diff $cet_file $origin_file
result=$?
echo "result:$result"

[[ "$result" -eq 0 ]] || {
        echo "$origin_file and $cet_file are not same, so update"
        echo "rm -rf $cet_file"
        rm -rf $cet_file
        echo "cp -rf $origin_file $cet_file"
        cp -rf $origin_file $cet_file
        }

result=""
diff $cet_file $origin_file
result=$?
echo "result:$result"
if [[ "$result" -eq 0 ]]; then
        echo "copy finished, and check files same"
else
        echo "copy stopped! Check files not same!"
fi
echo "chmod 755 $cet_file"
chmod 755 $cet_file
echo "touch -d 1 year ago $cet_file"
touch -d "1 year ago" $cet_file
