#include "methods.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

extern QList<QTreeWidgetItem*> tw_list;
extern MainWindow* mw_one;
extern bool loading;
extern bool isBreakFind;
extern QString curFindFile;

Methods::Methods(QObject* parent) : QObject{parent} {}

void Methods::init_Margin(QsciScintilla* textEdit) {
  int a;

  // 1.行号区域
  a = 0;
  // textEdit->SendScintilla(QsciScintilla::SCI_SETMARGINTYPEN, a,
  //                        QsciScintilla::SC_MARGIN_NUMBER);
  // textEdit->SendScintilla(QsciScintilla::SCI_SETMARGINWIDTHN, a, 50);
  // textEdit->SendScintilla(QsciScintilla::SCI_SETMARGINMASKN, a, 0x01);

  textEdit->setMarginType(a, QsciScintilla::NumberMargin);
  textEdit->setMarginLineNumbers(a, true);
  textEdit->setMarginSensitivity(a, true);

  // 2.书签
  a = 1;
  textEdit->setMarginType(a, QsciScintilla::SymbolMargin);
  textEdit->setMarginLineNumbers(a, false);
  textEdit->setMarginWidth(a, 12);
  textEdit->setMarginSensitivity(a, true);

  // 以下两种方法二选一
  // textEdit->setMarginMarkerMask(a, 1 << 4);
  // textEdit->markerDefine(QsciScintilla::Bookmark, 4);
  // textEdit->setMarkerBackgroundColor(QColor("#eaf593"), 5);

  textEdit->SendScintilla(QsciScintilla::SCI_SETMARGINMASKN, a, 1 << 4);
  textEdit->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, 4,
                          QsciScintilla::SC_MARK_BOOKMARK);
  textEdit->SendScintilla(QsciScintilla::SCI_MARKERSETFORE, 4,
                          QColor(Qt::blue));
  textEdit->SendScintilla(QsciScintilla::SCI_MARKERSETBACK, 4,
                          QColor(Qt::blue));

  // 3.自动折叠区域
  a = 2;
  textEdit->setMarginType(a, QsciScintilla::SymbolMargin);
  textEdit->setMarginLineNumbers(a, false);
  textEdit->setMarginWidth(a, 2);
  textEdit->setMarginSensitivity(a, true);
  textEdit->setFolding(QsciScintilla::BoxedTreeFoldStyle);  //折叠样式

  // 4.编译错误标记区域
  a = 3;
  textEdit->setMarginType(a, QsciScintilla::SymbolMargin);
  textEdit->setMarginLineNumbers(a, false);
  textEdit->setMarginWidth(a, 12);
  textEdit->setMarginSensitivity(a, true);

  // 以下两种方法二选一
  // textEdit->setMarginMarkerMask(a, 1 << 5);
  // textEdit->markerDefine(QsciScintilla::RightArrow, 5);
  // textEdit->setMarkerForegroundColor(Qt::red, 5);
  // textEdit->setMarkerBackgroundColor(Qt::red, 5);

  textEdit->SendScintilla(QsciScintilla::SCI_SETMARGINMASKN, a, 1 << 5);
  textEdit->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, 5,
                          QsciScintilla::SC_MARK_SHORTARROW);
  textEdit->SendScintilla(QsciScintilla::SCI_MARKERSETFORE, 5, QColor(Qt::red));
  textEdit->SendScintilla(QsciScintilla::SCI_MARKERSETBACK, 5, QColor(Qt::red));
}

void Methods::getAllFolds(const QString& foldPath, QStringList& folds) {
  QDirIterator it(foldPath, QDir::Dirs | QDir::NoDotAndDotDot,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    it.next();
    QFileInfo fileInfo = it.fileInfo();
    folds << fileInfo.absoluteFilePath();
  }
}

void Methods::getAllFiles(const QString& foldPath, QStringList& folds,
                          const QStringList& formats) {
  QDirIterator it(foldPath, QDir::Files | QDir::NoDotAndDotDot,
                  QDirIterator::Subdirectories);
  while (it.hasNext() && !isBreakFind) {
    it.next();
    QFileInfo fileInfo = it.fileInfo();
    if (formats.contains(fileInfo.suffix())) {  //检测格式，按需保存
      folds << fileInfo.absoluteFilePath();
      curFindFile = fileInfo.absoluteFilePath();
      MainWindow::searchMain(curFindFile);
    }
  }
}

void Methods::setSearchHistory() {
  if (loading) return;

  QString findText = mw_one->ui->editFind->lineEdit()->text().trimmed();
  QStringList strList;
  for (int i = 0; i < mw_one->ui->editFind->count(); i++) {
    strList.append(mw_one->ui->editFind->itemText(i));
  }

  for (int i = 0; i < strList.count(); i++) {
    if (findText == strList.at(i)) {
      strList.removeAt(i);
    }
  }
  strList.insert(0, findText);
  mw_one->AddCboxFindItem = true;
  mw_one->ui->editFind->clear();
  mw_one->ui->editFind->addItems(strList);
  mw_one->AddCboxFindItem = false;
  mw_one->ui->editFind->lineEdit()->selectAll();
}

void Methods::setColorMatch(int red, QsciLexer* textLexer) {
  if (red < 55)  //暗模式，mac下为50
  {
    textLexer->setColor(QColor(30, 190, 30),
                        QsciLexerCPP::CommentLine);  //"//"注释颜色
    textLexer->setColor(QColor(30, 190, 30), QsciLexerCPP::Comment);

    textLexer->setColor(QColor(210, 210, 210), QsciLexerCPP::Identifier);
    textLexer->setColor(QColor(245, 150, 147), QsciLexerCPP::Number);
    textLexer->setColor(QColor(100, 100, 250), QsciLexerCPP::Keyword);
    textLexer->setColor(QColor(210, 32, 240), QsciLexerCPP::KeywordSet2);
    textLexer->setColor(QColor(245, 245, 245), QsciLexerCPP::Operator);
    textLexer->setColor(QColor(84, 235, 159),
                        QsciLexerCPP::DoubleQuotedString);  //双引号
  } else {
    textLexer->setColor(QColor(30, 190, 30),
                        QsciLexerCPP::CommentLine);  //"//"注释颜色
    textLexer->setColor(QColor(30, 190, 30), QsciLexerCPP::Comment);

    textLexer->setColor(QColor(13, 136, 91), QsciLexerCPP::Number);
    textLexer->setColor(QColor(0, 0, 255), QsciLexerCPP::Keyword);
    textLexer->setColor(QColor(0, 0, 0), QsciLexerCPP::Identifier);
    textLexer->setColor(QColor(210, 0, 210), QsciLexerCPP::KeywordSet2);
    textLexer->setColor(QColor(20, 20, 20), QsciLexerCPP::Operator);
    textLexer->setColor(QColor(163, 21, 21),
                        QsciLexerCPP::DoubleQuotedString);  //双引号
  }
}

QStringList Methods::getVoidForCpp(QsciScintilla* textEdit) {
  QStringList listVoid;

  QTextEdit* txtEdit = new QTextEdit;
  txtEdit->setPlainText(textEdit->text());

  for (int i = 0; i < txtEdit->document()->lineCount(); i++) {
    QString line = getTextEditLineText(txtEdit, i).trimmed();
    if (isSymbol(line)) {
      if (line.mid(0, 1) != "/") {
        QStringList list1 = line.split("(");
        if (list1.count() > 1) {
          QString str1 = list1.at(0) + "|" + QString::number(i);
          listVoid.append(str1.trimmed());
        }
      }
    }
  }

  tw_list.clear();
  for (int i = 0; i < listVoid.count(); i++) {
    QString str0 = listVoid.at(i);
    QStringList list0 = str0.split("|");
    QTreeWidgetItem* twItem0 =
        new QTreeWidgetItem(QStringList() << list0.at(0) << list0.at(1));
    twItem0->setIcon(0, QIcon(":/icon/m.svg"));
    tw_list.append(twItem0);
  }

  return listVoid;
}

bool Methods::isSymbol(QString line) {
  line = line.trimmed();
  QStringList listKeys = QStringList() << "static"
                                       << "void"
                                       << "int"
                                       << "bool"
                                       << "const"
                                       << "virtual"
                                       << "class"
                                       << "long"
                                       << "signed"
                                       << "unsigned"
                                       << "typedef"
                                       << "friend"
                                       << "template"
                                       << "enum"
                                       << "inline"
                                       << "short"
                                       << "struct"
                                       << "union"
                                       << "double"
                                       << "float"
                                       << "string"
                                       << "char"
                                       << "wchar_t"
                                       << "QString"
                                       << "StringList";
  for (int i = 0; i < listKeys.count(); i++) {
    QString str0 = listKeys.at(i);
    if (line.mid(0, str0.length()) == str0 && !line.contains("=") &&
        !line.contains("<<") && !line.contains("+") && !line.contains("%") &&
        !line.contains("//") && !line.contains("number"))
      return true;
  }
  return false;
}

QString Methods::loadText(QString textFile) {
  QFileInfo fi(textFile);
  if (fi.exists()) {
    QFile file(textFile);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
      QMessageBox::warning(
          NULL, tr("Application"),
          tr("Cannot read file %1:\n%2.")
              .arg(QDir::toNativeSeparators(textFile), file.errorString()));

    } else {
      QTextStream in(&file);
      in.setCodec("UTF-8");
      QString text = in.readAll();
      return text;
    }
  }

  return "";
}

QString Methods::getTextEditLineText(QTextEdit* txtEdit, int i) {
  QTextBlock block = txtEdit->document()->findBlockByNumber(i);
  txtEdit->setTextCursor(QTextCursor(block));
  QString lineText = txtEdit->document()->findBlockByNumber(i).text().trimmed();
  return lineText;
}

#ifdef Q_OS_MAC
void Methods::init_MacVerInfo(QString ver) {
  QString str1 = qApp->applicationDirPath();
  QString infoFile = str1.replace("MacOS", "Info.plist");

  QTextEdit* edit = new QTextEdit;
  edit->setPlainText(loadText(infoFile));

  bool write = false;

  for (int i = 0; i < edit->document()->lineCount(); i++) {
    QString lineTxt = getTextEditLineText(edit, i).trimmed();

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    if (lineTxt == "<key>CFBundleShortVersionString</key>" ||
        lineTxt == "<key>CFBundleVersion</key>") {
      QString nextTxt = getTextEditLineText(edit, i + 1).trimmed();
      if (nextTxt != "<string>" + ver + "</string>") {
        QTextBlock block = edit->document()->findBlockByNumber(i + 1);
        QTextCursor cursor(block);
        block = block.next();
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.removeSelectedText();
        cursor.insertText("\n        <string>" + ver + "</string>");

        write = true;
      }
    }
#endif

#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    if (lineTxt == "<key>CFBundleGetInfoString</key>") {
      QString nextTxt = getTextEditLineText(edit, i + 1).trimmed();
      if (nextTxt != "<string>" + ver + "</string>") {
        QTextBlock block = edit->document()->findBlockByNumber(i + 1);
        QTextCursor cursor(block);
        block = block.next();
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.removeSelectedText();
        cursor.insertText("\n        <string>" + ver + "</string>");

        write = true;
      }
    }

#endif
  }

  if (write) {
    TextEditToFile(edit, infoFile);
  }
}
#endif

void Methods::TextEditToFile(QTextEdit* txtEdit, QString fileName) {
  QFile* file;
  file = new QFile;
  file->setFileName(fileName);
  bool ok = file->open(QIODevice::WriteOnly);
  if (ok) {
    QTextStream out(file);
    out << txtEdit->toPlainText();
    file->close();
    delete file;
  }
}
