# QtiASL
开源跨平台的dsdt&ssdt分析编辑器

写这个工具的初衷是为了完善win和linux平台下的dsdt编辑器生态。

目前可用于在win或winpe下提取和编辑dsdt文件，mac或Linux下也可以使用该工具来编写和调试dsdt、ssdt。


基本特征包括但不限于：

0.流畅、高效的编辑环境，数万乃至数十万行代码均能非常流畅地编辑

1.语法高亮

2.代码自动提示

3.代码折叠

4.显示缩进编辑线

5.显示大小括弧匹配

6.多国语言支持（目前支持中文和英文）

7.可自由定义编译参数或者选择编译参数，编译参数自动保存，具体的编译参数可查看iasl的帮助

8.编辑现场在软件开启后自动还原，精确到光标的位置

9.多标签编辑文件，文件编辑的状态实时显示（标签页上的红点和绿点）

10.支持DSDT+SSDT反编译、批量反编译

11.支持双击打开文件、拖拽到软件界面打开文件，打开文件的历史记录

12.如果当前打开的文件被其他软件修改，则自动提示是否重新装入

13.人性化的搜索功能，且支持简单的正则表达式

14.无限级别的撤销和恢复

15.编译出错的地方，在行号旁边采用红点来标识

16.完善的信息显示窗口，可显示『基本信息』、『错误』、『警告』等等，点击信息窗口里面的内容，可定位到相关联的代码行

......

国内github下载文件可以尝试采用：https://toolwa.com/github/


QtiASL的诞生离不开以下第三方开源软件的支持，感谢！

ACPI：https://acpica.org/source

QSci：https://riverbankcomputing.com/software/qscintilla/download

讨论区：

pcbeta远景：http://bbs.pcbeta.com/viewthread-1867564-1-1.html

insanelymac: https://www.insanelymac.com/forum/topic/344860-open-source-cross-platform-dsdtssdt-analysis-editor/


![截图](https://github.com/ic005k/QtiASL/blob/master/qtiasl.png)

备注：此项目开始于2020年8月
