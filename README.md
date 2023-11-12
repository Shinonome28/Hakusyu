# Hakusyu

Hakusyu是一个简单的游戏demo，
玩法是用拍掌或其他声音控制一个小人依次跳过柱子。

暂时不支持中文，因为我没找到适合的中文字体。

推荐运行前关闭输入法。

## 如何编译

需要以下工具链：
1. MSVC编译器（只在MSVC上测试过，但是应该可以跨平台）
2. CMake
3. 库 SDL2、SDL2 TTF、SDL2 Image

请使用CMake的Debug配置编译。