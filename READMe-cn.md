[简体中文](https://github.com/ic005k/QtiASL/blob/master/README-cn.md) | [English](https://github.com/ic005k/QtiASL/blob/master/README.md)
# QtiASL--开源跨平台的DSDT&SSDT集成开发环境

| [最新发布][release-link]|[下载][download-link]|[问题反馈][issues-link]|[讨论区][discourse-link]|
|-----------------|-----------------|-----------------|-----------------|
|[![release-badge](https://img.shields.io/github/release/ic005k/QtiASL.svg?style=flat-square "Release status")](https://github.com/ic005k/QtiASL/releases "Release status") | [![download-badge](https://img.shields.io/github/downloads/ic005k/QtiASL/total.svg?style=flat-square "Download status")](https://github.com/ic005k/QtiASL/releases/latest "Download status")|[![issues-badge](https://img.shields.io/badge/github-issues-red.svg?maxAge=60 "Issues")](https://github.com/ic005k/QtiASL/issues "Issues")|[![discourse](https://img.shields.io/badge/forum-discourse-orange.svg)](https://www.insanelymac.com/forum/topic/344860-open-source-cross-platform-dsdtssdt-ide/)|


[![MacOS](https://github.com/ic005k/QtiASL/actions/workflows/macos.yml/badge.svg)](https://github.com/ic005k/QtiASL/actions/workflows/macos.yml)    [![MacOS1012](https://github.com/ic005k/QtiASL/actions/workflows/macos1012.yml/badge.svg)](https://github.com/ic005k/QtiASL/actions/workflows/macos1012.yml)    [![Windows MinGW](https://github.com/ic005k/QtiASL/actions/workflows/windows-mingw.yml/badge.svg)](https://github.com/ic005k/QtiASL/actions/workflows/windows-mingw.yml)    [![Linux](https://github.com/ic005k/QtiASL/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/ic005k/QtiASL/actions/workflows/ubuntu.yml)


[download-link]: https://github.com/ic005k/QtiASL/releases/latest "Download status"
[download-badge]: https://img.shields.io/github/downloads/ic005k/QtiASL/total.svg?style=flat-square "Download status"

[release-link]: https://github.com/ic005k/QtiASL/releases "Release status"
[release-badge]: https://img.shields.io/github/release/ic005k/QtiASL.svg?style=flat-square "Release status"

[issues-link]: https://github.com/ic005k/QtiASL/issues "Issues"
[issues-badge]: https://img.shields.io/badge/github-issues-red.svg?maxAge=60 "Issues"

[discourse-link]: https://www.insanelymac.com/forum/topic/344860-open-source-cross-platform-dsdtssdt-ide/


## 基本特征包括但不限于：

* 自动加载当前使用的SSDT列表（Windows和Mac下面）

* 流畅、高效的编辑环境，数万乃至数十万行代码均能非常流畅地编辑

* 语法高亮

* 代码自动提示

* 代码折叠

* 显示缩进编辑线

* 显示大小括弧匹配

* 多国语言支持（目前支持中文和英文）

* 可自由定义编译参数或者选择编译参数，编译参数自动保存，具体的编译参数可查看iasl的帮助

* 编辑现场在软件开启后自动还原，精确到光标的位置

* 多标签编辑文件，文件编辑的状态实时显示（标签页上的红点和绿点）

* 支持DSDT+SSDT反编译、批量反编译

* 支持双击打开文件、拖拽到软件界面打开文件，打开文件的历史记录

* 如果当前打开的文件被其他软件修改，则自动提示是否重新装入

* 人性化的搜索功能，且支持简单的正则表达式

* 搜索框支持历史记录列表和输入自动完成

* 自动标记所有的搜索结果

* 搜索结果计数器

* 拖拽标签页形成一个新的窗口

* 无限级别的撤销和恢复

* 编译出错的地方，在行号旁边采用红点来标识

* 完善的信息显示窗口，可显示『基本信息』、『错误』、『警告』等等，点击信息窗口里面的内容，可定位到相关联的代码行

......

![截图](https://github.com/ic005k/QtiASL/blob/master/qtiasl.png)

## 感谢以下开源软件的支持！

[ACPI](https://acpica.org/source)&nbsp; &nbsp; &nbsp; &nbsp; [QSci](https://riverbankcomputing.com/software/qscintilla/download) &nbsp; &nbsp; &nbsp; [patchmatic](https://github.com/RehabMan/OS-X-MaciASL-patchmatic)


### 备注：此项目开始于2020年8月
