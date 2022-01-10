#include "methods.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

extern QList<QTreeWidgetItem*> tw_list;
extern MainWindow* mw_one;

Methods::Methods(QObject* parent) : QObject{parent} {}

QStringList Methods::getVoidForCpp(QString c_file) {
  QStringList listVoid;
  QFileInfo fi(c_file);

  if (fi.suffix().toLower() != "c" && fi.suffix().toLower() != "cpp")
    return listVoid;

  QTextEdit* txtEdit = new QTextEdit;
  txtEdit->setPlainText(loadText(c_file));

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
    // qDebug() << listVoid.at(i);
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
  line.trimmed();
  QStringList listKeys = QStringList() << "static"
                                       << "void"
                                       << "int"
                                       << "double"
                                       << "float"
                                       << "string"
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
