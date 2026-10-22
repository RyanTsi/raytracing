# 贴图、相机与静态模型验收

日期：2026-10-22。此记录接续第一轮核心修复。代码已经统一通过 MSVC Release 构建；原模型目录只读取，未修改、复制或上传人物资产。

## 相机

默认右键观察灵敏度从 0.12 降为 **0.04°/鼠标单位**，移动倍率默认 0.5；UI 的 Camera controls 与 CLI 均可调。Ctrl 使用四分之一倍率作精调，Shift 加速移动。

- 捕获鼠标时丢弃初始位移，失焦时释放；可用时启用 raw mouse motion。
- 围绕固定世界 Y 轴偏航；俯仰限制在 ±89°，重新正交化相机基，防止翻转和累计滚转。
- 移动按经过秒数计算，斜向归一化；Q/E 保持世界竖直。
- ImGui 键盘捕获与鼠标捕获分别判断；从场景内开始的右键拖动持续到释放。

CPU 回归覆盖极点恢复、世界轴偏航、去滚转、30/144 FPS 移动一致性、斜向速度与 Q/E。原生窗口中检查过最终参数区，默认 Look speed=0.040、Move speed=0.50，控件正常显示，Esc 退出正常。本次自动化接口不能模拟持续按住右键拖动，因此不把这些检查表述为“已验证真实鼠标手感”；最终感受仍可通过 UI 的灵敏度继续调节。

## 材质与路径

新增 `TextureBank`，外部贴图通过 Unicode filesystem 路径读取字节后解码；支持 Assimp 内嵌图像。颜色以 GL_SRGB8_ALPHA8 上传并由硬件解码，normal/roughness/metallic/AO/opacity 数据以线性 RGBA8 上传。

- 插值 UV0，构造切线空间法线，采样 GGX 金属/粗糙度材质。
- OBJ 中名为 normal 的 RGB map_Bump 按法线图处理；真正的标量高度图不会冒充 RGB 法线。
- glTF 的同文件 metallic/roughness 图读取 G/B 通道。
- 支持 alpha mask 或随机覆盖率透明；透明判断用独立哈希，不推进路径 RNG，防止 BVH 遍历顺序改变结果。
- 缺失引用贴图会报错退出，避免静默丢失材质。
- 新增贴图材质 SSBO stride=48，原 Material stride=32、TriangleData=100 不变；真实驱动反射检查通过。
- 颜色与数据用独立纹理数组；按每个数组的最大宽高统一尺寸，原始同尺寸图逐字节上传，不同尺寸图最近邻重采样。当前使用显式 LOD0，无 mipmap/各向异性过滤。
- 多模型用材质索引，避免 vector 扩容使裸指针失效；内嵌纹理缓存包含模型序号，避免不同文件各自的 `*0` 混淆。

Windows argv 通过宽字符命令行转 UTF-8，模型、资源、图片和输出使用 filesystem Unicode 路径。人物 PMX 和中文贴图名实际加载成功。

## 实际模型

| 模型 | 三角形 | 材质 | 颜色/数据图 | 结果 |
|---|---:|---:|---:|---|
| 背包 | 67,907 | 2（含默认材质） | 1 /4，4096² | 颜色、normal、roughness、metallic、AO 全部加载 |
| 诺姆主体 | 45,818 | 30 | 5 /0，2048² | 全部静态网格、颜色与 alpha 加载 |
| 主体 + 独立武器 | 54,468 | 31 | 6 /0，2048² | 同时加载，按原始模型坐标展示 |

背包原包说明 metallic 被改名为 specular 供非 PBR 教程使用，本项目据此还原含义，没有将它误当普通镜面强度。`发光.png` 在该 PMX 中用作头发高光的透明颜色覆层，本项目按实际引用加载，不将文件名直接当作发光材质。

高采样输出在 ignored `captures/`：

- `backpack-final.png`：800×1000，1024 spp。
- `norma-final.png`：720×1080，1024 spp。
- `norma-with-weapon.png`：720×1080，1024 spp。

三者使用六次反弹、环境辐射亮度 0.18、曝光 0.3 EV、种子 1；对应 PFM/JSON 可检查。前两张曾并行渲染，其耗时不作为独立性能基准。含武器版本只合并模型原坐标，不自动装配到手部或绑定骨骼；原武器包中的多个部件/状态均会显示。

PMX 是**静态几何与基础材质验证**，没有恢复 MMD toon/sphere、骨骼动画、形态键、物理或完整原生效果。AO 以艺术性衰减作用于间接路径，法线基由三角形 UV 与插值法线构造，不承诺与特定烘焙器的逐顶点 MikkTSpace 完全一致。透明覆盖率不是玻璃折射。

## GPU 回归

运行 `tests/validate_textures.py` 及可选外部 `--character` 分支，报告保存在 `captures/texture-validation/` 和 `captures/texture-validation-integrated/`。

| 检查 | 实测 |
|---|---|
| 四象限 PNG 的 UV 上下左右 | 投影到已知世界点后颜色匹配 |
| sRGB 128 解码 | HDR 约 0.2158203，理论约 0.2158605，绝对差约 4.02e-5 |
| Alpha=0 | 穿透到已知背板颜色；其他象限保持不透明颜色 |
| 平坦/倾斜 RGB normal | 世界法线与理论差分别约 5.94e-8 /4.47e-8 |
| 丢失贴图 | 非零退出，指出 Missing texture 与具体文件 |
| 两个 glTF 各有内嵌 `*0` | 左红右绿均匹配；2 meshes、4 materials、2 颜色图、1 attachment |
| 真实背包 | 64² 预览有 582 个明显彩色像素、450 种量化颜色；路径追踪有限、非空 |
| 外部人物 | albedo 与路径追踪均有限、非空，目视检查完整人物 |
| GL debug | 所有成功用例 error=0 |

既有 `tests/validate_render.py --out captures/validation-textured` 也全量通过：默认场景 BVH/暴力最大绝对差 0；带贴图背包两种遍历 HDR MSE=0；glTF 变换对照 MSE 约 1.50e-17；相同种子独立进程逐值复现；多采样收敛、三种估计器能量对照、缺法线和错误参数检查通过。

`ctest --preset windows-release` 的 CPU 合约与真实 OpenGL smoke 均通过。GitHub Actions 配置只要求 Windows 编译和 CPU 合约，不声称 hosted runner 能运行硬件 OpenGL 测试。

## 发布范围

- 项目代码 MIT，背包与 vendored 依赖分别署名并保留许可。
- 未核实来源的旧孤立贴图仅本地隔离保留，不从新 main 历史引用。
- 人物原包禁止二次分发/商业用途；模型、贴图、转换副本和人物截图均未加入 Git。
- 公开 README 只附带有 CC BY 署名的背包渲染图。
- 人物本地渲染不自动获得商业化视频的授权，详见 [许可说明](LICENSING.md)。
