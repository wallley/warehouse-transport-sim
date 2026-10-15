# GitHub 发布与维护

项目已公开发布至 [wallley/warehouse-transport-sim](https://github.com/wallley/warehouse-transport-sim)，默认分支为 `main`，采用 [MIT License](../LICENSE)。首次发布与核验于 2026-10-15 完成，学校课程简介、11 个 Topics、README 及真实界面图均已配置。以下操作供后续维护或迁移时参考。

首次实现提交 `1a88e9e` 的 45 个远端文件已通过匿名读取逐项比对；Windows 与 Ubuntu 的核心构建和测试均[通过 GitHub Actions](https://github.com/wallley/warehouse-transport-sim/actions/runs/38599368723)。此记录针对该提交；后续版本以对应提交的检查结果为准。

## 仓库展示

推荐仓库描述：

> 山东大学自动化《高级程序设计与实践》课程项目：C++ / EasyX 环形仓储穿梭车仿真，108 条订单、18 个站点，支持 3/5/7 车调度与三车巡游。

在仓库首页 `About` 中添加相关 Topics：

```text
cpp easyx simulation warehouse logistics scheduling state-machine
course-project shandong-university sdu automation
```

学校与课程全名放在 README 中，Topics 使用简短英文关键词。GitHub 会在仓库页面展示根目录 README；Topics 可以帮助读者按主题找到项目。参见 [GitHub README 文档](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-readmes)与 [Topics 文档](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/classifying-your-repository-with-topics)。

首页展示真实界面、运行命令、课程背景和代码阅读入口。后续增加展示素材时，使用没有桌面通知、账户信息和个人署名的程序画面，并说明图像展示的是界面还是一次实际运行结果。

## 公开内容范围

保留学校、学院、自动化专业、课程名称与题目来源。省略真实姓名、学号、具体班级、联系方式、私人账户凭据和本机绝对路径。课程原 PDF、报告、个人贡献表和历史压缩包不上传。

仓库保存源码、文档、构建和测试配置，以及 README 界面素材。编译产物、运行日志、结果 JSON 和 IDE 缓存放在 Git 忽略目录。MIT 许可使用 `2026 wallley and contributors` 作为版权署名；EasyX 等外部依赖不打包，遵循其各自的上游条款。

## 使用 GitHub 昵称与 noreply 提交

在 GitHub `Settings → Emails` 复制账户实际提供的 noreply 邮箱，将下面占位值替换为该地址。只设置当前仓库，不修改其他项目的提交身份：

```sh
git config user.name wallley
git config user.email "YOUR_GITHUB_NOREPLY_EMAIL"
git config --get user.name
git config --get user.email
```

不要把占位值当成邮箱使用。GitHub 不同账户的 noreply 地址格式可能不同，以账户设置页面显示的值为准，参见[提交邮箱说明](https://docs.github.com/en/account-and-profile/how-tos/email-preferences/setting-your-commit-email-address)。已有本地提交也应检查提交身份；改写已经共享的历史需要另行讨论。

## 首次发布

在新工程目录内先完成构建、测试与内容检查：

```sh
cmake -S . -B build -DBUILD_GUI=OFF
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
git status --short
git remote -v
```

如果 GitHub 仓库尚未创建，在账户 `wallley` 下新建名为 `warehouse-transport-sim` 的 Public 仓库。已有本地 README 和 LICENSE，创建远程时无需再次生成它们。

若当前仓库尚未配置 `origin`，执行：

```sh
git remote add origin https://github.com/wallley/warehouse-transport-sim.git
```

若 `origin` 已存在，核对它确实指向上述仓库。完成本阶段中文提交后，将 `main` 分支推送：

```sh
git push -u origin main
```

推送后检查 GitHub 首页、界面图片、文档链接、MIT 许可、提交署名和自动检查结果，再补充 `About` 描述与 Topics。网络超时或连接中断时，先检查网络及 Clash/VPN，再重试并核对远程状态。

## 后续维护

按完成并验证的阶段提交，提交标题和正文用中文说明变化、原因及验证结果。只加入本阶段涉及的文件，发布前检查待推送内容；需要调整模块职责、接口或运行参数时，同步更新架构和使用文档。

展示新性能结论时，应附可重复的命令、版本、种子和完成条件，核对未完成任务与物理约束。仅增加更漂亮的截图时，不把它写成算法性能提升。
