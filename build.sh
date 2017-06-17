export ECCE_HOME=`pwd`
cd build/
./build_ecce
./build_ecce|tee xerxes.log && ./build_ecce |tee mesa.log && ./build_ecce |tee wxwidgets.log
./build_ecce|tee wxpython.log
./build_ecce|tee httpd.log && ./build_ecce|tee ecce.log
