#!/bin/sh
# 在开发板的新实验目录中运行。仅测试临时文件，不触碰其他目录。
set -eu
cd "$(dirname "$0")"
mkdir -p evidence
date -Iseconds > evidence/environment.txt
uname -a >> evidence/environment.txt
getconf LONG_BIT >> evidence/environment.txt
cc --version >> evidence/environment.txt
cat evidence/environment.txt
make > evidence/build.log 2>&1
cat evidence/build.log
# 不通过 tee 管道启动测试，以免隐藏 make/test 的非零退出状态。
make test > evidence/test.log 2>&1
cat evidence/test.log
printf '\n板端编译与测试通过。请按 README 的演示命令运行并截取真实终端画面。\n'
