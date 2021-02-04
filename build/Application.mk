
APP_ABI := armeabi-v7a arm64-v8a x86 x86_64

APP_MODULES = MediaService

# 指定目标Android平台的名称
APP_PLATFORM = android-26

APP_STL := c++_static

# 为项目中的所有C++编译传递的标记
APP_CPPFLAGS := -frtti -fexceptions