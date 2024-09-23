# btstack_test
准备环境:
```
git clone https://github.com/bouffalolab/bouffalo_sdk.git
cd ./bouffalo_sdk/examples
git clone https://github.com/O2C14/btstack_test.git
cd ./btstack_test
rm -rf ./btstack
git clone https://github.com/bluekitchen/btstack.git -b v1.6.1
```
然后就可以像其他例子那样编译烧录

若需要改变demo可以在CMakeLists.txt修改`set(EXAMPLE "a2dp_sink_demo")`

使用 `btstack erase` 来忘记已连接的设备