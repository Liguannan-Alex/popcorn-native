# Popcorn Battle - Native C++ Version

跨平台原生 C++ 版本，支持 Mac 开发，Windows 运行。

## 依赖

### macOS
```bash
# 使用 Homebrew 安装
brew install cmake sdl2 opencv

# OpenGL 由系统提供
```

### Windows
```bash
# 使用 vcpkg 安装
vcpkg install sdl2:x64-windows opencv4:x64-windows

# 或手动下载 SDL2 和 OpenCV
```

## 构建

### macOS
```bash
cd popcorn-native
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
```

### Windows (Visual Studio)
```bash
cd popcorn-native
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Windows (MinGW)
```bash
cd popcorn-native
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -j%NUMBER_OF_PROCESSORS%
```

## 运行

构建完成后，可执行文件位于 `build/bin/` 目录：
```bash
./build/bin/PopcornBattle
```

## 项目结构

```
popcorn-native/
├── CMakeLists.txt          # CMake 构建配置
├── BUILD.md                # 本文件
├── assets/
│   └── models/             # MediaPipe 模型文件
├── src/
│   ├── main.cpp            # 入口点
│   ├── core/
│   │   ├── Application.h/cpp   # 应用程序主类
│   │   ├── Window.h/cpp        # SDL2 窗口管理
│   │   └── Renderer.h/cpp      # OpenGL 渲染器
│   ├── camera/
│   │   └── CameraCapture.h/cpp # 摄像头采集
│   ├── detection/
│   │   └── PoseDetector.h/cpp  # 姿态检测（待集成 MediaPipe）
│   └── game/
│       ├── FallingItem.h       # 掉落物结构
│       ├── GameEngine.h/cpp    # 游戏逻辑
│       └── CollisionSystem.h/cpp # 碰撞检测
└── third_party/            # 第三方库（可选）
    ├── glad/               # OpenGL 加载器
    └── imgui/              # UI 库
```

## 待完成

1. **MediaPipe C++ 集成**
   - 下载 MediaPipe C++ SDK
   - 配置 CMake 查找路径
   - 实现 PoseDetector 的真实检测逻辑

2. **UI 渲染**
   - 集成 ImGui 或使用 FreeType 渲染文字
   - 显示分数、时间、FPS

3. **Windows 测试**
   - 在 Windows 上测试编译
   - 处理平台差异（OpenGL 头文件路径等）

## 性能目标

- 渲染帧率：60 FPS
- 检测延迟：< 50ms
- 内存占用：< 200MB
