cd build
cmake ..
make rmdb -j8 && echo "加载数据库文件：" && echo $1 && ./bin/rmdb $1 || { echo "构建失败"; exit 1; }
