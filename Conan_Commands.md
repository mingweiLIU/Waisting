## 安装conan
pip install conan
## 检查环境设置
conan profile detect --force
## 查看环境配置文件的位置
conan config home 获取的位置中/profile/default为默认的配置文件 这个时候可以自己修改一个来使用 比如修改为
```
[settings]
arch=x86_64
build_type=Debug
compiler=msvc
compiler.cppstd=14
compiler.runtime=dynamic
compiler.version=193
os=Windows
```
并命名为build_Debug

## 包配置文件获取第三方库
conan install . --output-folder=build --build=missing --profile=build_Debug  //如果不指定profile则为调用defualt其为release版本
或者不创建新的配置文件 直接使用
```
conan install . --output-folder=build --build=missing --settings=build_type=Debug
```
