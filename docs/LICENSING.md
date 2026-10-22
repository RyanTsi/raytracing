# 开源代码、模型与公开视频的许可边界

根目录 [LICENSE](../LICENSE) 为 MIT，署名为 RyanTsi，年份为 2025–2026。
原本科提交记录的作者使用 solaRyan / solaryan，当前 Git 配置使用 RyanTsi。
MIT 授权适用于有权授权的项目自有代码、着色器、测试、脚本和文档；
第三方代码与素材继续遵循各自条款，详见
[THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)。

## 可以随源码保留的内容

- 项目自有代码及简单测试场景。
- 保留许可证和版权声明的 vendored 依赖。
- 背包模型和纹理：Berk Gedik 的 Survival Guitar Backpack，来源页标示
  CC Attribution；本仓库按 CC BY 4.0 提供署名、来源、许可链接及修改说明。
  Joey de Vries 对材质赋值和贴图命名的改动也保留署名。
- 人物模型不随代码、Release 压缩包或测试数据上传；通过本地外部路径读取。

原 `assets/image.png` 是没有记录来源的旧照片贴图，不被本次 MIT 授权覆盖，
从新的公开提交树移除。本地隔离副本和完整旧历史备份不属于发布物。

## 诺姆模型：本地读取，不再配布

本次核对的外部素材包包含 `诺姆1.pmx`、`武器.pmx`、`Texture/`
和 `readme【一定要看】.txt`。后者列出的规则包括：

- 允许完善物理、修正权重/表情问题，允许改色、适度更改衣装、添加 spa/toon。
- **禁止二次配布**，禁止拆取部件用于改造其他模型。
- **禁止商业用途**。
- 禁止用于成人作品、极端宗教宣传、血腥恐怖猎奇作品、人身攻击等列明题材。
- 模型制作：**去看流星雨**；BiliBili：**-去看流星雨-**。
- 文件注明最终解释权归属 **miHoYo**。

核对日期为 2026-10-22；以上是随包文件的摘要，不能替代原始规则，也不代表
额外获得了作者许可。转换后的 OBJ、glTF、缓存网格和模型贴图仍属于该素材，
不能因为格式改变就上传。不要把含有原始或转换模型的本地验证目录打进 Release。

核对的原始 readme 文件 SHA-256：
`37077b28a3629b5bcf6a48f9b800aa0e8e78901cd19c934d8ce0cb3fbf34e19f`。

当前是静态几何/材质渲染验证；程序支持 PMX 并不等于恢复 MMD 的骨骼动画、
形态键、物理模拟、toon/sphere 材质的全部表现。

## 发布视频时怎么处理

**代码展示与模型授权分别检查。** MIT 允许在遵守版权/许可声明要求下分发代码，
不自动许可画面中的第三方角色、纹理、配乐或商标。

- 背包画面：在简介或片尾署名 Berk Gedik，给出模型来源及 CC BY 4.0 链接，
  说明材质改动与本项目渲染；可直接采用 THIRD_PARTY_NOTICES 中的署名范例。
- 诺姆画面：保留模型制作与权利方署名，遵守随包规则。
  包内文字**不能作为商业化视频的授权依据**。广告、赞助、带货、付费展示等
  商业用途需要另行取得许可，或改用明确允许该用途的角色/自有资产。
- 不在视频附带工程、网盘、源码仓库或 Release 中再次提供该人物模型或转换副本。
- 非商业公开视频的具体题材、平台分成等适用范围，若原规则没有写明，
  应向素材权利方确认；本仓库不作扩大解释。

## 依赖与二进制发布

`lib/` 在此版本是普通 vendored 源码树，不是真正的 Git submodule。
保留上游的许可、版权与 per-file notices。GLFW 是 zlib 风格许可，
GLM 采用 MIT 选项，Dear ImGui 为 MIT，Assimp 为 BSD-3-Clause 并含独立许可的
contrib/test 内容，stb_image 采用 MIT 选项。

GLAD 生成文件的标记是 `(WTFPL OR CC0-1.0) AND Apache-2.0`，
本项目采用 CC0-1.0 选项并保留 Apache-2.0 条件；KHR 平台头保留其 MIT 风格声明。
不能仅因为 GLAD 生成器本身采用 MIT，就把生成的 Khronos 内容统称为 MIT。

发布可执行文件包时，把根 LICENSE、THIRD_PARTY_NOTICES、LICENSES 目录的
许可文本集合一起带上；分发上游源文件时还要保留其原始文件声明。
不要把 Assimp 的上游测试模型一概当作 BSD 或项目 MIT 素材打包。

LearnOpenGL 是本项目历史开发的学习参考。其当前 About 页面分别说明代码示例
采用 CC BY-NC 4.0、图片/视频采用 CC BY 4.0。上游内容如有直接复制，保留它自己的
条款；本项目的 MIT 声明不改变这些内容的许可。

## 核对来源

- 本地 Git 作者/日期与文件内许可证。
- [Berk Gedik 原模型页](https://sketchfab.com/3d-models/survival-guitar-backpack-799f8c4511f84fab8c3f12887f7e6b36)
  与本地 `assets/backpack/source_attribution.txt`。
- [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)。
- 人物素材原包 `readme【一定要看】.txt`，只在本地读取。
- [GLAD v2.0.8 许可](https://github.com/Dav1dde/glad/blob/v2.0.8/LICENSE)。
- [LearnOpenGL About](https://learnopengl.com/About)。

这些说明记录已核查的许可边界，不宣称已取得第三方未写明的授权。
