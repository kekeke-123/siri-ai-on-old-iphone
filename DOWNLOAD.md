# 下载与安装（不签名）

## 直接拿包

每次 `main` 分支构建成功，CI 都会把未签名的 `.ipa` 贴到固定的滚动 Release：

- 页面：https://github.com/kekeke-123/siri-ai-on-old-iphone/releases/tag/latest-build
- 直链：https://github.com/kekeke-123/siri-ai-on-old-iphone/releases/download/latest-build/SiriAI-unsigned.ipa

包内没有任何签名，也没有描述文件（没有 `_CodeSignature`、没有 `embedded.mobileprovision`），这是刻意为之。

## 怎么装进手机

三选一，按你机器的条件选：

1. **TrollStore**（iOS 14.0–16.6.1 / 17.0，A9–A16）：直接把 `.ipa` 拖进 TrollStore 安装，不需要证书，永久生效。
   - iPhone 15 系列（A16/A17）且系统 > 16.6.1 的不行，往下看。
2. **Sideloadly / AltStore**（任意 iOS 16+）：用你的 Apple ID 重签，免费账号 7 天掉签、最多 3 个 App。
3. **自签 + 开发者证书**：`codesign -f -s "Apple Development: ..."` 重签后 `ios-deploy` 或 Xcode 装机。

重签只改签名，不需要改代码；widget（`ai.siriplus.app.widget`）会跟着主 App 一起签。

## 装完要做什么

1. 进「设置」页导入一个 GGUF 小模型（0.5B–3B 量化q4 在小内存机型上体验最好）。
2. 想要视觉能力再加一个 `mmproj` 文件（同一模型系列的 mmproj 必须匹配）。
3. 语音：本 App 只调系统**离线**识别（`requiresOnDeviceRecognition = true`），没下离线包会说“这台设备的离线识别不可用”，不会偷偷上云。

## 系统要求

| 部件 | 最低 | 说明 |
|---|---|---|
| 主 App | iOS 16.0 | CI 里 Xcode 16.4 / iOS 18.5 SDK 能编出的最低值 |
| 小组件 / 灵动岛 | iOS 16.1 | Live Activity 从 16.1 开始可用 |
| 内核（arm64 静态库） | iOS 16.0 | `CMAKE_OSX_DEPLOYMENT_TARGET=16.0` |

想要 iOS 26 基线，本地用 Xcode 26 把 `app/project.yml` 与 `kernel/CMakeLists.txt` 里的 16.0/16.1 改成 26.0 重编即可；账号里的 ARM runner 不支持 Xcode 26，所以云端 CI 固定跑 16.0。
