# Git 历史整理

原本科实现有 23 条提交，时间为 2025-01-14 至 2025-04-17，原 main 为：

`fbabb29eba642c27d901f2272e18e94d2b8dbe36`

按仓库作者的要求，公开 main 整理为两层：

1. `4a3787b86a74a0ac7ef220c6715497174ee3d0ab` — 本科实现汇总快照，保留原作者与最后开发日期。除去未使用、未记录分发许可的 `assets/image.png`，代码、场景和依赖内容与原 main 相同。
2. 其后的修复提交 — HDR 与积分修复、相机控制、贴图和 PMX 静态导入、回归测试、MIT 与第三方声明。

本次从新 main 可达的提交中排除那张孤立贴图，不宣称已经擦除 GitHub 服务器中此前保存的对象。图片的本地隔离副本也不属于发布物。

改写前已保存并验证完整历史 bundle：

~~~powershell
git bundle verify .history-backups/undergraduate-before-rewrite.bundle
~~~

本地还保留 `refs/archive/undergraduate-before-rewrite`，可只读查看：

~~~powershell
git log --oneline refs/archive/undergraduate-before-rewrite
git show refs/archive/undergraduate-before-rewrite:src/main.cpp
~~~

如需独立恢复旧仓库，可在选择好恢复位置后执行 `git clone <bundle路径> <新的空目录>`。这些备份保留在本地 ignored `.history-backups/`，没有推送备份分支、标签或人物资产。

历史改写使原克隆的 main 与新 main 不再具有祖先关系。其他已有克隆应先保存本地工作，再重新克隆，或显式迁移需要保留的提交。推送使用绑定旧 main SHA 的 force-with-lease；若远程被其他人更新，会拒绝覆盖。
