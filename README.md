# 文字卡片生成器 (TextCardGenerator)

自定义风格的文字卡片生成工具，Qt 5.15 Widgets 项目（qmake 构建）。

输入词条，一键生成可直接打印/裁剪的文字卡片，输出 PNG 图片与 Word (.docx) 文档。

## 功能

- 输入文字（多个词条用逗号、分号或换行分隔）
- 自动按指定字数截断，超长词条自动拆成多张卡片
- 可配置字体、字体大小、图片宽度、文字颜色、背景颜色
- 实时预览第一张卡片效果
- 输出两种形式：
  1. 每张卡片独立 PNG 图片（默认黑底白字 SimSun 宋体）
  2. A4 竖向/横版排版 Word (.docx) 文件（无需安装 Word 依赖，程序内直接生成）
- 输出文件名自动取词条内容，重名自动追加序号

## 环境要求

- Qt 5.15（Widgets 模块）
- 编译器：MinGW 或 MSVC 均可
- 构建系统：qmake（项目提供 `.pro` 文件）

## 编译

### Qt Creator 方式

1. 打开 Qt Creator
2. 文件 → 打开文件或项目 → 选择 `TextCardGenerator.pro`
3. 选择 MinGW 或 MSVC 套件（Qt 5.15）
4. 点击左下角项目模式切换为 "Release"
5. 点击 ▶ 构建（或 Ctrl+B）

### 命令行方式（可选）

```bash
# 进入项目目录
cd TextCardGenerator

# 生成构建文件并编译
qmake TextCardGenerator.pro
mingw32-make        # MinGW 环境
# 或
nmake               # MSVC 环境
```

可执行文件生成在 build 目录下。

## 使用

1. 在文本框输入词条，例如：

   ```
   角的初步认识，角的特征，角的大小，角的种类
   边，顶点
   与两边张开的大小有关
   读作：角1，记作：∠1
   锐角，直角，钝角
   ```

2. 设置"字体像素大小"、"图片宽度"、"文字/背景颜色"、"输出格式"
3. 选择输出目录
4. 点击"生成"按钮

## 输出说明

- 每张卡片一张 PNG，文件名即卡片文字（非法字符替换为 `_`）
- `卡片_A4_<时间戳>.docx`：所有卡片竖向排列居中，可在 Word 中继续编辑

## 项目结构

```
TextCardGenerator/
├── main.cpp            # 程序入口
├── mainwindow.h        # 主窗口声明
├── mainwindow.cpp      # 主窗口实现（渲染 / 拆分 / 生成逻辑）
├── TextCardGenerator.pro  # qmake 工程文件
├── README.md
└── .gitignore
```

## 技术要点

- **卡片渲染**：`QPainter` 按文本宽度自适应计算画布大小，超宽时按比例缩放字体
- **Word 生成**：未使用第三方库，手写 OOXML（`word/document.xml` + 图片关系）+ 自实现 ZIP 打包（`writeZip` / `crc32Of` / 本地头、中央目录、EOCD 结构）
- **词条拆分**：支持 `,` `，` `;` `；` 换行作为分隔符

## 已知限制

- **PDF 输出暂时禁用**：`generatePdf()` 逻辑已实现，但界面中 PDF 复选框当前置灰（代码中 `m_chkPdf->setEnabled(false)`），待验证后开放
- docx 页面边距为固定值，如需自定义可在 `generateDocx()` 中调整

## 打印建议

PDF/Word 打印时选择"实际大小 / 100%"，不要勾选"适应页面"，以保证卡片尺寸统一。
