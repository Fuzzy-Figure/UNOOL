# 第三方许可声明 / Third Party Notices

本项目（UNOOL）在 `dep/` 中随仓库携带以下第三方依赖。它们的版权与许可证归各自作者所有，
本文件的 MIT 授权（见根目录 `LICENSE`）不覆盖这些第三方组件。

## 1. SFML 3.1.0

- 用途：窗口、图形、音频、网络。
- 版本：`dep/SFML/include/SFML/Config.hpp` 中为 3.1.0。
- 许可证：zlib/libpng License（SFML 采用 zlib 许可）。
- 上游：https://www.sfml-dev.org/ · https://github.com/SFML/SFML
- 说明：`dep/SFML/` 内未随附独立的 LICENSE 文本，许可以其上游发布为准；
  下列静态库为 SFML 官方 Windows 发行版捆绑/链接的第三方组件，各自适用其上游许可。

### SFML 捆绑/链接的下游组件（依据 `dep/SFML/lib/*.lib` 与 `dep/SFML/include/` 实际存在项）

| 组件 | 随附产物 | 上游许可（以其官方声明为准） |
|---|---|---|
| FreeType | `freetype.lib` / `freetyped.lib` | FreeType License / GPLv2 双许可 |
| HarfBuzz | `harfbuzz.lib` / `harfbuzzd.lib` | MIT |
| libogg | `ogg.lib` / `oggd.lib` | BSD-3-Clause |
| libvorbis | `vorbis*.lib` | BSD-3-Clause |
| libFLAC | `FLAC.lib` / `FLACd.lib` | BSD-3-Clause（部分文件为公共领域） |
| libssh2 | `libssh2.lib` | BSD-3-Clause |
| Mbed TLS | `mbedtls.lib` / `mbedcrypto.lib` / `mbedx509.lib`（头在 `include/mbedtls`、`include/psa`） | Apache-2.0 |

注：以上均为 SFML 官方预编译包内包含的组件。发行二进制时请一并遵守各组件许可，
并保留相应版权声明。未在 `dep/` 中发现 SheenBidi、cpp-unicodelib、stb、miniaudio、qoi
的独立库或头文件，故不列入；若后续引入须在此补充。

## 2. nlohmann/json 3.12.0

- 用途：账号存档、商城数据、配置解析等 JSON 序列化。
- 版本：`dep/nlohmann/json.hpp` 中为 3.12.0。
- 许可证：MIT License。
- 版权：Copyright (c) 2013-2025 Niels Lohmann。
- 上游：https://github.com/nlohmann/json
- 说明：`json.hpp` 头部含 SPDX-License-Identifier: MIT 声明；`dep/nlohmann/` 内无独立 LICENSE 文件。

## 3. 其他素材

- 角色名称、角色图片、卡牌图片（见 `images/`）版权归各自原作者所有，不在本 MIT 授权范围。
- 卡牌玩法规则灵感来源于经典 UNO，本项目为课程/个人学习用途的非官方实现。
